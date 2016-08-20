#include "AvisynthAPI.h"
#include "PostProcess.h"
#include "color_convert.h"




BlindPP::BlindPP(AVSValue args, IScriptEnvironment* env)  : GenericVideoFilter(args[0].AsClip())
{
    int quant = args[1].AsInt(2);
    if (vi.width%16!=0)
        env->ThrowError("BlindPP : Need mod16 width");
    if (vi.height%16!=0)
        env->ThrowError("BlindPP : Need mod16 height");
    if (!vi.IsYUV() || !vi.IsPlanar())
        env->ThrowError("BlindPP : Only planar YUV colorspace supported");

    QP = nullptr;
    QP = new int[vi.width*vi.height/256];
    if (QP == nullptr)
        env->ThrowError("BlindPP:  malloc failure!");
    for (int i=0;i<vi.width*vi.height/256;i++)
        QP[i] = quant;

    switch (args[2].AsInt(6)) {
    case 0 : PP_MODE = 0; break;
    case 1 : PP_MODE = PP_DEBLOCK_Y_H; break;
    case 2 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V; break;
    case 3 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H; break;
    case 4 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V; break;
    case 5 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y; break;
    case 6 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y | PP_DERING_C; break;
    default : env->ThrowError("BlindPP : cpu level invalid (0 to 6)");
    }

    const char* cpu2 = args[3].AsString("");
    if (strlen(cpu2)==6) {
        PP_MODE = 0;
        if (cpu2[0]=='x' || cpu2[0]=='X') { PP_MODE |= PP_DEBLOCK_Y_H; }
        if (cpu2[1]=='x' || cpu2[1]=='X') { PP_MODE |= PP_DEBLOCK_Y_V; }
        if (cpu2[2]=='x' || cpu2[2]=='X') { PP_MODE |= PP_DEBLOCK_C_H; }
        if (cpu2[3]=='x' || cpu2[3]=='X') { PP_MODE |= PP_DEBLOCK_C_V; }
        if (cpu2[4]=='x' || cpu2[4]=='X') { PP_MODE |= PP_DERING_Y; }
        if (cpu2[5]=='x' || cpu2[5]=='X') { PP_MODE |= PP_DERING_C; }
    }

    iPP = args[4].AsBool(false);
    moderate_h = args[5].AsInt(20);
    moderate_v = args[6].AsInt(40);

    if (vi.IsYUY2())
    {
        out = create_YV12PICT(vi.height,vi.width,2);
        PP_MODE |= PP_DONT_COPY;
    }
    else out = nullptr;
}

BlindPP::~BlindPP()
{
    if (QP != nullptr) delete[] QP;
    if (out != nullptr) destroy_YV12PICT(out);
}


PVideoFrame __stdcall BlindPP::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame dstf = env->NewVideoFrame(vi);
    PVideoFrame cf = child->GetFrame(n,env);

    if (vi.IsYV12())
    {
        uint8_t* src[3];
        uint8_t* dst[3];
        src[0] = const_cast<uint8_t*>(cf->GetReadPtr(PLANAR_Y));
        src[1] = const_cast<uint8_t*>(cf->GetReadPtr(PLANAR_U));
        src[2] = const_cast<uint8_t*>(cf->GetReadPtr(PLANAR_V));
        dst[0] = dstf->GetWritePtr(PLANAR_Y);
        dst[1] = dstf->GetWritePtr(PLANAR_U);
        dst[2] = dstf->GetWritePtr(PLANAR_V);
        postprocess(src, cf->GetPitch(PLANAR_Y), cf->GetPitch(PLANAR_U),
            dst, dstf->GetPitch(PLANAR_Y), dstf->GetPitch(PLANAR_U),
            vi.width, vi.height, QP, vi.width/16,
            PP_MODE, moderate_h, moderate_v, false, iPP);
    }
    else
    {
        uint8_t* dst[3];
        convYUY2to422P(cf->GetReadPtr(),out->y,out->u,out->v,cf->GetPitch(),out->ypitch,
            out->uvpitch,vi.width,vi.height); // 4:2:2 packed to 4:2:2 planar
        dst[0] = out->y;
        dst[1] = out->u;
        dst[2] = out->v;
        postprocess(dst, out->ypitch, out->uvpitch,
            dst, out->ypitch, out->uvpitch,
            vi.width, vi.height, QP, vi.width/16, PP_MODE,
            moderate_h, moderate_v, true, iPP);
        conv422PtoYUY2(out->y,out->u,out->v,dstf->GetWritePtr(),out->ypitch,out->uvpitch,
            dstf->GetPitch(),vi.width,vi.height);  // 4:2:2 planar to 4:2:2 packed
    }

    return dstf;
}



AVSValue __cdecl BlindPP::create(AVSValue args, void*, IScriptEnvironment* env)
{
    return new BlindPP(args, env);
}
