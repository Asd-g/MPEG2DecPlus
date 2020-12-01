# D2VSource

This is a project (previously named as MPEG2DecPlus) to modify DGDecode.dll for AviSynth+.

# Usage

 ```
 D2VSource(string "d2v", int "idct", bool "showQ", int "info", int "upConv", bool "i420", bool "iCC")
 ```

## Parameters:

- d2v\
    The path of the dv2 file.
    
- idct\
    The iDCT algorithm to use.\
    0: Follow the d2v specification.\
    1,2,3,6,7: AP922 integer (same as SSE2/MMX).\
    4: SSE2/AVX2 LLM (single precision floating point, SSE2/AVX2 determination is automatic).\
    5: IEEE 1180 reference (double precision floating point).\
    Default: -1.
    
- showQ\
    It displays macroblock quantizers..\
    Default: False.
    
- info\
    It prints debug information.\
    0: Do not display.\
    1: Overlay on video frame.\
    2: Output with OutputDebugString(). (The contents are confirmed by DebugView.exe).\
    3: Embed hints in 64 bytes of the frame upper left corner.\
    Default: 0.
    
- upConv\
    The output format.\
    0: No conversion. YUV420 output is YV12, YUV422 output is YV16.\
    1: Output YV16.\
    2: Output YV24.\
    Default: 0.
    
- i420\
    It determinates what is the output of YUV420.\
    True: The output is i410.\
    False: The output is YV12.\
    Default: False.
    
- iCC\
    It determinates how YUV420 is upscaled when upConv=true.\
    True: Force field-based upsampling.\
    False: Forse progressive upsampling.\
    Default: Auto determination based on the frame flag.
    
## Exported variables:

FFSAR_NUM, FFSAR_DEN, FFSAR.
