/*
 *  Copyright (C) Chia-chen Kuo - April 2001
 *
 *  This file is part of DVD2AVI, a free MPEG-2 decoder
 *
 *  DVD2AVI is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  DVD2AVI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "MPEG2Decoder.h"

void CMPEG2Decoder::Initialize_Buffer()
{
    Rdptr = Rdbfr + BUFFER_SIZE;
    Rdmax = Rdptr;
    buffer_invalid = (uint8_t *) 0xffffffff;

    if (SystemStream_Flag)
    {
        if (Rdptr >= Rdmax)
            Next_Packet();
        CurrentBfr = *Rdptr++ << 24;

        if (Rdptr >= Rdmax)
            Next_Packet();
        CurrentBfr += *Rdptr++ << 16;

        if (Rdptr >= Rdmax)
            Next_Packet();
        CurrentBfr += *Rdptr++ << 8;

        if (Rdptr >= Rdmax)
            Next_Packet();
        CurrentBfr += *Rdptr++;

        Fill_Next();
    }
    else
    {
        Fill_Buffer();

        CurrentBfr = (*Rdptr << 24) + (*(Rdptr+1) << 16) + (*(Rdptr+2) << 8) + *(Rdptr+3);
        Rdptr += 4;

        Fill_Next();
    }

    BitsLeft = 32;
}

uint32_t CMPEG2Decoder::Get_Bits_All(uint32_t N)
{
    N -= BitsLeft;
    Val = (CurrentBfr << (32 - BitsLeft)) >> (32 - BitsLeft);

    if (N != 0)
        Val = (Val << N) + (NextBfr >> (32 - N));

    CurrentBfr = NextBfr;
    BitsLeft = 32 - N;
    Fill_Next();

    return Val;
}

void CMPEG2Decoder::Flush_Buffer_All(uint32_t N)
{
    CurrentBfr = NextBfr;
    BitsLeft = BitsLeft + 32 - N;
    Fill_Next();
}


typedef struct {
    // 1 byte
    uint8_t sync_byte; //         8   bslbf

    // 2 bytes
    uint8_t transport_error_indicator;//      1   bslbf
    uint8_t payload_unit_start_indicator;//       1   bslbf
    uint8_t transport_priority; //        1   bslbf
    uint16_t pid; //  13  uimsbf

    // 1 byte
    uint8_t transport_scrambling_control;//       2   bslbf
    uint8_t adaptation_field_control;//       2   bslbf
    uint8_t continuity_counter;//     4   uimsbf

    // VVV (only valid if adaptation_field_control != 1)
    // 1 byte
        uint8_t adaptation_field_length; // 8 uimsbf

        // VVV (only valid if adaptation_field_length != 0)
        // 1 byte
            uint8_t discontinuity_indicator; //   1   bslbf
            uint8_t random_access_indicator; //   1   bslbf
            uint8_t elementary_stream_priority_indicator; //  1   bslbf
            uint8_t PCR_flag; //  1   bslbf
            uint8_t OPCR_flag; // 1   bslbf
            uint8_t splicing_point_flag; //   1   bslbf
            uint8_t transport_private_data_flag; //   1   bslbf
            uint8_t adaptation_field_extension_flag; //   1   bslbf

    /*
    if(adaptation_field_control=='10'  || adaptation_field_control=='11'){
        adaptation_field()
    }
    if(adaptation_field_control=='01' || adaptation_field_control=='11') {
        for (i=0;i<N;i++){
            data_byte       8   bslbf
        }
    }
    */

} transport_packet;

#define SKIP_TRANSPORT_PACKET_BYTES( bytes_to_skip ) \
{   Rdptr += (bytes_to_skip); Packet_Length -= (bytes_to_skip); }

void CMPEG2Decoder::Next_Transport_Packet()
{
    int Packet_Length;  // bytes remaining in MPEG-2 transport packet
    int Packet_Header_Length;
    uint32_t code;
    transport_packet tp = {0};

    for (;;)
    {
        // 0) initialize some temp variables
        Packet_Length = TransportPacketSize; // total length of an MPEG-2 transport packet

        if (TransportPacketSize == 192)
        {
            Get_Byte();
            Get_Byte();
            Get_Byte();
            Get_Byte();
            Packet_Length -= 4;
        }

        // 1) Search for a sync byte. Gives some protection against emulation.
        for(;;)
        {
            if ((tp.sync_byte = Get_Byte()) != 0x47)
                continue;

            if (Rdptr - Rdbfr > TransportPacketSize)
            {
                if (Rdptr[-(TransportPacketSize+1)] == 0x47)
                    break;
            }
            else if (Rdbfr + Read - Rdptr > TransportPacketSize - 1)
            {
                if (Rdptr[+(TransportPacketSize-1)] == 0x47)
                    break;
            }
            else
            {
                // We can't check so just accept this sync byte.
                break;
            }
        }
        --Packet_Length; // decrement the sync_byte;

        // 2) get pid, transport_error_indicator, payload_unit_start_indicator
        code = Get_Short();
        Packet_Length = Packet_Length - 2; // decrement the two bytes we just got;
        tp.pid = code & 0x1FFF; // bits [12:0]
        tp.transport_error_indicator = (code >> 15) & 0x01;  // bit#15
        tp.payload_unit_start_indicator = (code >> 14) & 0x01; // bit#14
        tp.transport_priority = (code >> 13) & 0x01; // bit#13

        // 3) get other fields
        code = Get_Byte();
        --Packet_Length; // decrement the 1 byte we just got;
        tp.transport_scrambling_control = (code >> 6) & 0x03;//     2   bslbf
        tp.adaptation_field_control = (code >> 4 ) & 0x03;//        2   bslbf
        tp.continuity_counter = code & 0x0F;//      4   uimsbf


        // 4) check for early-exit conditions ... (possibly skip packet)
        // we don't care about the continuity counter
        // if ( tp.continuity_counter != previous_continuity_counter ) ...
        if ( tp.transport_error_indicator ||
                 (tp.adaptation_field_control == 0) )
        {
            // skip remaining bytes in current packet
            SKIP_TRANSPORT_PACKET_BYTES( Packet_Length )
            continue; // abort, and circle back to top of 'for() loop'
        }

        // 5) check
        if ( tp.adaptation_field_control == 2 || tp.adaptation_field_control == 3)
        {
            // adaptation field is present
            tp.adaptation_field_length = Get_Byte(); // 8-bits
            --Packet_Length; // decrement the 1 byte we just got;

            if ( tp.adaptation_field_length != 0 ) // end of field already?
            {
                // if we made it this far, we no longer need to decrement
                // Packet_Length.  We took care of it up there!
                code = Get_Byte();
                --Packet_Length; // decrement the 1 byte we just got;
                tp.discontinuity_indicator = (code >> 7) & 0x01; // 1   bslbf
                tp.random_access_indicator = (code >> 6) & 0x01; // 1   bslbf
                tp.elementary_stream_priority_indicator = (code >> 5) & 0x01; //    1   bslbf
                tp.PCR_flag = (code >> 4) & 0x01; //    1   bslbf
                tp.OPCR_flag = (code >> 3) & 0x01; //   1   bslbf
                tp.splicing_point_flag = (code >> 2) & 0x01; // 1   bslbf
                tp.transport_private_data_flag = (code >> 1) & 0x01; // 1   bslbf
                tp.adaptation_field_extension_flag = (code >> 0) & 0x01; // 1   bslbf

                // skip the remainder of the adaptation_field
                SKIP_TRANSPORT_PACKET_BYTES( tp.adaptation_field_length-1 )
            } // if ( tp.adaptation_field_length != 0 )
        } // if ( tp.adaptation_field_control != 1 )

        // we've processed the header, so now just the payload is left...

        // video
        if ( tp.pid == MPEG2_Transport_VideoPID && Packet_Length > 0)
        {
#if 0
            code = Get_Short();
            code = (code & 0xffff)<<16 | Get_Short();
            Packet_Length = Packet_Length - 4; // remove these two bytes

            // Packet start?
            if (code < 0x000001E0 || code > 0x000001EF )
            if (!tp.payload_unit_start_indicator)
            {
                // No, move the buffer-pointer back.
                Rdptr -= 4;
                Packet_Length = Packet_Length + 4; // restore these four bytes
            }
            else
#endif
            if (tp.payload_unit_start_indicator)
            {
                // YES, pull out PTS
                Get_Short();
                Get_Short();
                Get_Short(); // MPEG2-PES total Packet_Length
                Get_Byte(); // skip a byte
                code = Get_Byte();
                Packet_Header_Length = Get_Byte();
                Packet_Length = Packet_Length - 9; // compensate the bytes we extracted

                // get PTS, and skip rest of PES-header
                if (code>=0x80 && Packet_Header_Length > 4 ) // Extension_flag ?
                {
                    // Skip PES_PTS
                    Get_Byte();
                    Get_Short();
                    Get_Short();
                    Packet_Length = Packet_Length - 5;
                    SKIP_TRANSPORT_PACKET_BYTES( Packet_Header_Length-5 )
                }
                else
                    SKIP_TRANSPORT_PACKET_BYTES( Packet_Header_Length )
            }
            Rdmax = Rdptr + Packet_Length;
            if (TransportPacketSize == 204)
                Rdmax -= 16;
            return;
        }

        // fall through case
        // skip the remainder of the adaptation_field
        SKIP_TRANSPORT_PACKET_BYTES( Packet_Length )
    } // for
}

// PVA packet data structure.
struct pva_packet {
    uint16_t sync_byte;
    uint8_t stream_id;
    uint8_t counter;
    uint8_t reserved;
    uint8_t flags;
    uint16_t length;
};

// PVA transport stream parser.
void CMPEG2Decoder::Next_PVA_Packet()
{
    uint32_t Packet_Length;
    pva_packet pva;
    uint32_t PTS;

    for (;;)
    {
        // Search for a good sync.
        while (true)
        {
            // Sync word is 0x4156.
            if (Get_Byte() != 0x41) continue;
            if (Get_Byte() != 0x56)
            {
                // This byte might be a 0x41, so back up by one.
                Rdptr--;
                continue;
            }
            // To protect against emulation of the sync word,
            // also check that the stream says audio or video.
            pva.stream_id = Get_Byte();
            if (pva.stream_id != 0x01 && pva.stream_id != 0x02)
            {
                // This byte might be a 0x41, so back up by one.
                Rdptr--;
                continue;
            }
            break;
        }

        // Pick up the remaining packet header fields.
        pva.counter = Get_Byte();
        pva.reserved = Get_Byte();
        pva.flags = Get_Byte();
        pva.length = Get_Byte() << 8;
        pva.length |= Get_Byte();
        Packet_Length = pva.length;

        // Any payload?
        if (Packet_Length == 0 || pva.reserved != 0x55)
            continue;  // No, try the next packet.

        // Check stream id for video.
        if (pva.stream_id == 1)
        {
            // This is a video packet.
            // Extract the PTS if it exists.
            if (pva.flags & 0x10)
            {
                // The spec is unclear about the significance of the prebytes field.
                // It appears to be safe to ignore it.
                PTS = (int) ((Get_Byte() << 24) | (Get_Byte() << 16) | (Get_Byte() << 8) | Get_Byte());
                Packet_Length -= 4;
            }

            // Deliver the video to the ES parsing layer.
            Rdmax = Rdptr + Packet_Length;
            return;
        }

        // Not an video packet or an audio packet to be demultiplexed. Keep looking.
        SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
    }
}

void CMPEG2Decoder::Next_Packet()
{
    uint32_t code, Packet_Length, Packet_Header_Length;
    static int stream_type;

    if ( SystemStream_Flag == 2 )  // MPEG-2 transport packet?
    {
        Next_Transport_Packet();
        return;
    }
    else if ( SystemStream_Flag == 3 )  // PVA packet?
    {
        Next_PVA_Packet();
        return;
    }

    for (;;)
    {
        code = Get_Short();
        code = (code<<16) + Get_Short();

        // remove system layer byte stuffing
        while ((code & 0xffffff00) != 0x00000100)
        {
            if (Fault_Flag == OUT_OF_BITS)
                return;
            code = (code<<8) + Get_Byte();
        }

        if (code == PACK_START_CODE)
        {
            if ((Get_Byte() & 0xf0) == 0x20)
            {
                Rdptr += 7; // MPEG1 program stream
                stream_type = MPEG1_PROGRAM_STREAM;
            }
            else
            {
                Rdptr += 8; // MPEG2 program stream
                stream_type = MPEG2_PROGRAM_STREAM;
            }
        }
        else if ((code & 0xfffffff0) == VIDEO_ELEMENTARY_STREAM)
        {
            Packet_Length = Get_Short();
            Rdmax = Rdptr + Packet_Length;

            if (stream_type == MPEG1_PROGRAM_STREAM)
            {
                // MPEG1 program stream.
                Packet_Header_Length = 0;
                // Stuffing bytes.
                do
                {
                    code = Get_Byte();
                    Packet_Header_Length += 1;
                } while (code == 0xff);
                if ((code & 0xc0) == 0x40)
                {
                    // STD bytes.
                    Get_Byte();
                    code = Get_Byte();
                    Packet_Header_Length += 2;
                }
                if ((code & 0xf0) == 0x20)
                {
                    // PTS bytes.
                    Get_Short();
                    Get_Short();
                    Packet_Header_Length += 4;
                }
                else if ((code & 0xf0) == 0x30)
                {
                    // PTS/DTS bytes.
                    Get_Short();
                    Get_Short();
                    Get_Short();
                    Get_Short();
                    Get_Byte();
                    Packet_Header_Length += 9;
                }
                return;
            }
            else
            {
                // MPEG2 program stream.
                code = Get_Byte();
                if ((code & 0xc0)==0x80)
                {
                    code = Get_Byte();
                    Packet_Header_Length = Get_Byte();

                    Rdptr += Packet_Header_Length;
                    return;
                }
                else
                    Rdptr += Packet_Length-1;
            }
        }
        else if (code>=SYSTEM_START_CODE)
        {
            code = Get_Short();
            Rdptr += code;
        }
    }
}

void CMPEG2Decoder::Fill_Buffer()
{
    Read = _read(Infile[File_Flag], Rdbfr, BUFFER_SIZE);

    if (Read < BUFFER_SIZE)
        Next_File();

    Rdptr = Rdbfr;

    if (SystemStream_Flag)
        Rdmax -= BUFFER_SIZE;
}

void CMPEG2Decoder::Next_File()
{
    int bytes;

    if (File_Flag < File_Limit-1)
    {
        File_Flag ++;
    }
    else
    {
        // This mechanism is not yet working.
#if 0
        Fault_Flag = OUT_OF_BITS;
#endif
        File_Flag = 0;
    }
    // Even if we ran out of files, we reread the first one, just so
    // the decoder at least processes valid data until it detects the
    // fault flag and exits.
    _lseeki64(Infile[File_Flag], 0, SEEK_SET);
    bytes = _read(Infile[File_Flag], Rdbfr + Read, BUFFER_SIZE - Read);
    if (Read + bytes == BUFFER_SIZE)
        // The whole buffer has valid data.
        buffer_invalid = (uint8_t *) 0xffffffff;
    else
        // Point to the first invalid buffer location.
        buffer_invalid = Rdbfr + Read + bytes;
}

uint32_t CMPEG2Decoder::Show_Bits(uint32_t N)
{
    if (N <= BitsLeft) {
        return (CurrentBfr << (32 - BitsLeft)) >> (32 - N);;
    }
    else
    {
        N -= BitsLeft;
        return (((CurrentBfr << (32 - BitsLeft)) >> (32 - BitsLeft)) << N) + (NextBfr >> (32 - N));;
    }
}

uint32_t CMPEG2Decoder::Get_Bits(uint32_t N)
{
    if (N < BitsLeft)
    {
        Val = (CurrentBfr << (32 - BitsLeft)) >> (32 - N);
        BitsLeft -= N;
        return Val;
    }
    else
        return Get_Bits_All(N);
}

void CMPEG2Decoder::Flush_Buffer(uint32_t N)
{
    if (N < BitsLeft)
        BitsLeft -= N;
    else
        Flush_Buffer_All(N);
}

void CMPEG2Decoder::Fill_Next()
{
    // This mechanism is not yet working.
#if 0
    if (Rdptr >= buffer_invalid)
    {
        Fault_Flag = OUT_OF_BITS;
        return;
    }
#endif

    if (SystemStream_Flag && Rdptr > Rdmax - 4)
    {
        if (Rdptr >= Rdmax)
            Next_Packet();
        NextBfr = Get_Byte() << 24;

        if (Rdptr >= Rdmax)
            Next_Packet();
        NextBfr += Get_Byte() << 16;

        if (Rdptr >= Rdmax)
            Next_Packet();
        NextBfr += Get_Byte() << 8;

        if (Rdptr >= Rdmax)
            Next_Packet();
        NextBfr += Get_Byte();
    }
    else if (Rdptr <= Rdbfr + BUFFER_SIZE - 4)
    {
        NextBfr = (*Rdptr << 24) + (*(Rdptr+1) << 16) + (*(Rdptr+2) << 8) + *(Rdptr+3);
        Rdptr += 4;
    }
    else
    {
        if (Rdptr >= Rdbfr+BUFFER_SIZE)
            Fill_Buffer();
        NextBfr = *Rdptr++ << 24;

        if (Rdptr >= Rdbfr+BUFFER_SIZE)
            Fill_Buffer();
        NextBfr += *Rdptr++ << 16;

        if (Rdptr >= Rdbfr+BUFFER_SIZE)
            Fill_Buffer();
        NextBfr += *Rdptr++ << 8;

        if (Rdptr >= Rdbfr+BUFFER_SIZE)
            Fill_Buffer();
        NextBfr += *Rdptr++;
    }
}

uint32_t CMPEG2Decoder::Get_Byte()
{
    // This mechanism is not yet working.
#if 0
    if (Rdptr >= buffer_invalid)
    {
        Fault_Flag = OUT_OF_BITS;
        return Rdptr[-1];
    }
#endif

    while (Rdptr >= (Rdbfr + BUFFER_SIZE))
    {
        Read = _read(Infile[File_Flag], Rdbfr, BUFFER_SIZE);

        if (Read < BUFFER_SIZE)
            Next_File();

        Rdptr -= BUFFER_SIZE;
        Rdmax -= BUFFER_SIZE;
    }

    return *Rdptr++;
}

uint32_t CMPEG2Decoder::Get_Short()
{
    uint32_t i = Get_Byte();
    return (i<<8) + Get_Byte();
}

void CMPEG2Decoder::next_start_code()
{
    uint32_t show;

    // This is contrary to the spec but is more resilient to some
    // stream corruption scenarios.
    BitsLeft = ((BitsLeft + 7) / 8) * 8;

    while (1)
    {
        show = Show_Bits(24);
        if (Fault_Flag == OUT_OF_BITS)
            return;
        if (show == 0x000001)
            return;
        Flush_Buffer(8);
    }
}
