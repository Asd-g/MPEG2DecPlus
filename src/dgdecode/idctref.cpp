/* idctref_miha.c, Inverse Discrete Fourier Transform, double precision */

/*************************************************************/
/*                                                           */
/* x87 hand-optimized assembly by Miha Peternel              */
/*                                        27.11. - 20.1.2001 */
/*                                                           */
/* You are free to use this code in your project if:         */
/* - no changes are made to this message                     */
/* - any changes to this code are publicly available         */
/* - your project documentation contains the following text: */
/*   "This software contains fast high-quality IDCT decoder  */
/*    by Miha Peternel."                                     */
/*                                                           */
/*************************************************************/

// revised by Loli.J: bug fix together with rounding mode optimization
// clipping is unnecessary since motion compensation will saturate thereafter

//  Perform IEEE 1180 reference (64-bit floating point, separable 8x1
//  direct matrix multiply) Inverse Discrete Cosine Transform

#include "idct.h"

static const double r[8][8] = {
  {
     3.5355339059327379e-001,  3.5355339059327379e-001,
     3.5355339059327379e-001,  3.5355339059327379e-001,
     3.5355339059327379e-001,  3.5355339059327379e-001,
     3.5355339059327379e-001,  3.5355339059327379e-001,
  }, {
     4.9039264020161522e-001,  4.1573480615127262e-001,
     2.7778511650980114e-001,  9.7545161008064166e-002,
    -9.7545161008064096e-002, -2.7778511650980098e-001,
    -4.1573480615127267e-001, -4.9039264020161522e-001,
  }, {
     4.6193976625564337e-001,  1.9134171618254492e-001,
    -1.9134171618254486e-001, -4.6193976625564337e-001,
    -4.6193976625564342e-001, -1.9134171618254517e-001,
     1.9134171618254500e-001,  4.6193976625564326e-001,
  }, {
     4.1573480615127262e-001, -9.7545161008064096e-002,
    -4.9039264020161522e-001, -2.7778511650980109e-001,
     2.7778511650980092e-001,  4.9039264020161522e-001,
     9.7545161008064388e-002, -4.1573480615127256e-001,
  }, {
     3.5355339059327379e-001, -3.5355339059327373e-001,
    -3.5355339059327384e-001,  3.5355339059327368e-001,
     3.5355339059327384e-001, -3.5355339059327334e-001,
    -3.5355339059327356e-001,  3.5355339059327329e-001,
  }, {
     2.7778511650980114e-001, -4.9039264020161522e-001,
     9.7545161008064152e-002,  4.1573480615127273e-001,
    -4.1573480615127256e-001, -9.7545161008064013e-002,
     4.9039264020161533e-001, -2.7778511650980076e-001,
  }, {
     1.9134171618254492e-001, -4.6193976625564342e-001,
     4.6193976625564326e-001, -1.9134171618254495e-001,
    -1.9134171618254528e-001,  4.6193976625564337e-001,
    -4.6193976625564320e-001,  1.9134171618254478e-001,
  }, {
     9.7545161008064166e-002, -2.7778511650980109e-001,
     4.1573480615127273e-001, -4.9039264020161533e-001,
     4.9039264020161522e-001, -4.1573480615127251e-001,
     2.7778511650980076e-001, -9.7545161008064291e-002,
  },
};

void __fastcall REF_IDCT(int16_t* block) noexcept
{
  int *b = (int *)block;
  double tmp[64], rnd[64];

  if ( !(b[0]|(b[31]&~0x10000)) )
  {
    if( b[ 1]|b[ 2]|b[ 3]|b[ 4]|b[ 5]|b[ 6] )
      goto __normal;
    if( b[ 7]|b[ 8]|b[ 9]|b[10]|b[11]|b[12] )
      goto __normal;
    if( b[13]|b[14]|b[15]|b[16]|b[17]|b[18] )
      goto __normal;
    if( b[19]|b[20]|b[21]|b[22]|b[23]|b[24] )
      goto __normal;
    if( b[25]|b[26]|b[27]|b[28]|b[29]|b[30] )
      goto __normal;
    b[31] = 0;
    return;
  }

__normal:

  __asm
  {
    mov  esi, [block]
    lea  eax, [r]
    lea  edi, [tmp]
    mov  ebx, 8

__col1:
      mov  edx, [esi+0*2]
      mov  ecx, [esi+2*2]
      or   edx, [esi+4*2]
      or   ecx, [esi+6*2]
      or   edx, ecx
      mov  ecx, 4
      jnz  __row1

        fild  word ptr [esi+0*2]
        fmul  qword ptr [eax+0*8*8]
        fst   qword ptr [edi+0*8]
        fst   qword ptr [edi+1*8]
        fst   qword ptr [edi+2*8]
        fst   qword ptr [edi+3*8]
        fst   qword ptr [edi+4*8]
        fst   qword ptr [edi+5*8]
        fst   qword ptr [edi+6*8]
        fstp  qword ptr [edi+7*8]
        add   edi, 64
        jmp   __next1

__row1:
        fild  word ptr [esi+0*2]
        fmul  qword ptr [eax+0*8*8]
        fild  word ptr [esi+1*2]
        fmul  qword ptr [eax+1*8*8]
        fadd
        fild  word ptr [esi+2*2]
        fmul  qword ptr [eax+2*8*8]
        fadd
        fild  word ptr [esi+3*2]
        fmul  qword ptr [eax+3*8*8]
        fadd
        fild  word ptr [esi+4*2]
        fmul  qword ptr [eax+4*8*8]
        fadd
        fild  word ptr [esi+5*2]
        fmul  qword ptr [eax+5*8*8]
        fadd
        fild  word ptr [esi+6*2]
        fmul  qword ptr [eax+6*8*8]
        fadd
        fild  word ptr [esi+7*2]
        fmul  qword ptr [eax+7*8*8]
        fadd

        fild  word ptr [esi+0*2]
        fmul  qword ptr [eax+0*8*8+8]
        fild  word ptr [esi+1*2]
        fmul  qword ptr [eax+1*8*8+8]
        fadd
        fild  word ptr [esi+2*2]
        fmul  qword ptr [eax+2*8*8+8]
        fadd
        fild  word ptr [esi+3*2]
        fmul  qword ptr [eax+3*8*8+8]
        fadd
        fild  word ptr [esi+4*2]
        fmul  qword ptr [eax+4*8*8+8]
        fadd
        fild  word ptr [esi+5*2]
        fmul  qword ptr [eax+5*8*8+8]
        fadd
        fild  word ptr [esi+6*2]
        fmul  qword ptr [eax+6*8*8+8]
        fadd
        fild  word ptr [esi+7*2]
        fmul  qword ptr [eax+7*8*8+8]
        fadd
        add   eax, 16
        fxch  st(1)
        fstp  qword ptr [edi]
        fstp  qword ptr [edi+8]
        add   edi, 16

      sub  ecx, 1
      jnz  __row1
      sub  eax, 64

__next1:
      add  esi, 16

    sub  ebx, 1
    jnz  __col1

    lea  esi, [tmp]
    lea  eax, [r]
    lea  edi, [rnd]

    mov  ebx, 8
__row2:
      mov  ecx, 4

__col2:
        fld   qword ptr [esi+0*8*8]
        fmul  qword ptr [eax+0*8*8]
        fld   qword ptr [esi+1*8*8]
        fmul  qword ptr [eax+1*8*8]
        fadd
        fld   qword ptr [esi+2*8*8]
        fmul  qword ptr [eax+2*8*8]
        fadd
        fld   qword ptr [esi+3*8*8]
        fmul  qword ptr [eax+3*8*8]
        fadd
        fld   qword ptr [esi+4*8*8]
        fmul  qword ptr [eax+4*8*8]
        fadd
        fld   qword ptr [esi+5*8*8]
        fmul  qword ptr [eax+5*8*8]
        fadd
        fld   qword ptr [esi+6*8*8]
        fmul  qword ptr [eax+6*8*8]
        fadd
        fld   qword ptr [esi+7*8*8]
        fmul  qword ptr [eax+7*8*8]
        fadd

        fld   qword ptr [esi+0*8*8]
        fmul  qword ptr [eax+0*8*8+8]
        fld   qword ptr [esi+1*8*8]
        fmul  qword ptr [eax+1*8*8+8]
        fadd
        fld   qword ptr [esi+2*8*8]
        fmul  qword ptr [eax+2*8*8+8]
        fadd
        fld   qword ptr [esi+3*8*8]
        fmul  qword ptr [eax+3*8*8+8]
        fadd
        fld   qword ptr [esi+4*8*8]
        fmul  qword ptr [eax+4*8*8+8]
        fadd
        fld   qword ptr [esi+5*8*8]
        fmul  qword ptr [eax+5*8*8+8]
        fadd
        fld   qword ptr [esi+6*8*8]
        fmul  qword ptr [eax+6*8*8+8]
        fadd
        fld   qword ptr [esi+7*8*8]
        fmul  qword ptr [eax+7*8*8+8]
        fadd
        add   eax, 16

        fxch  st(1)
        fstp  qword ptr [edi]
        fstp  qword ptr [edi+8*8]
        add   edi, 128

      sub  ecx, 1
      jnz  __col2
      sub  eax, 64
      add  esi, 8
      sub  edi, 504

    sub  ebx, 1
    jnz  __row2

    lea  esi, [rnd]
    mov  edi, [block]
    mov  ecx, 8

__round:
      fld    qword ptr [esi+0*8]
      fistp  word ptr [edi+0*2]
      fld    qword ptr [esi+1*8]
      fistp  word ptr [edi+1*2]
      fld    qword ptr [esi+2*8]
      fistp  word ptr [edi+2*2]
      fld    qword ptr [esi+3*8]
      fistp  word ptr [edi+3*2]
      fld    qword ptr [esi+4*8]
      fistp  word ptr [edi+4*2]
      fld    qword ptr [esi+5*8]
      fistp  word ptr [edi+5*2]
      fld    qword ptr [esi+6*8]
      fistp  word ptr [edi+6*2]
      fld    qword ptr [esi+7*8]
      add    esi, 64
      fistp  word ptr [edi+7*2]
      add    edi, 16
    sub  ecx,1
    jnz  __round
  }
}
