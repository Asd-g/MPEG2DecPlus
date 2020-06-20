# MPEG2DecPlus
これはDGDecode.dllをAvisynth+用に改造するプロジェクトです。

###やりたいこと:
    - 改築を重ねた温泉旅館のようなコードをきれいにする。
    - VFAPI用コード、YUY2用コード等、現在では必要ないコードの排除。
    - アセンブラの排除による64bitへの対応、及びSSE2/AVX2でのintrinsicによる最適化。等

###必要なもの:
    - Windows Vista SP2 以降の Windows OS
    - SSE3が使えるCPU(Intel Pentium4(prescott) または AMD Athlon64x2 以降)
    - Avisynth+ r2172以降 またはAvisynth 2.60以降
    - Microsoft VisualC++ Redistributable Package 2019.

 ###使い方:
 ```
 MPEG2Source(string "d2v", int "cpu", int "idct", bool "iPP", int "moderate_h", int "moderate_v",
             bool "showQ", bool "fastMC", string "cpu2", int "info", int "upConv", bool "i420", bool "iCC")
 ```
    d2v: dv2ファイルのパス

    cpu: 現在使用不可。設定しても何も起こらない。iPP, moderate_h, moderate_v, fastMC, cpu2も同様。

    idct: 使用するiDCTアルゴリズム。
        0: d2vの指定に従う。
        1,2,3,6,7: AP922整数(SSE2MMXと同じもの)。
        4: SSE2/AVX2 LLM(単精度浮動小数点、SSE2/AVX2の判定は自動)。
        5: IEEE 1180 reference(倍精度浮動小数点)。

    showQ: マクロブロックの量子化器を表示する。

    info: デバッグ情報を出力する。
        0: 表示しない。(デフォルト)
        1: 動画フレームにオーバーレイで表示。
        2: OutputDebugString()で出力。(内容はDebugView.exeで確認)
        3: hintsをフレーム左上隅の64バイトに埋め込む。

    upConv: フレームを出力するフォーマット。
        0: YUV420なソースはYV12で出力、YUV422なソースはYV16で出力。
        1: YV16で出力。
        2: YV24で出力。

    i420: trueであればYUV420をi420として出力する。現在ではどちらでもほぼ変わりはない。

    iCC: upConvにおけるYUV420の取扱いの設定。
        未設定: フレームフラグに従ってinterlaced/progressiveを切り替える。
        true: 全フレームをinterlacedとして処理する。
        false: 全フレームをprogressiveとして処理する。


```
LumaYUV(clip c, int "lumoff", int "lumgain")
```
入力クリップの輝度をlumoffとlumgainの値によって変更する。出力Y = (入力y * lumgain) + lumoff

    clip: Y8, YV12, YV16, YV411, YV24をサポート。

    lumoff: -255 ～ 255 (デフォルト0)

    lumgain: 0.0 ～ 2.0 (デフォルト1.0)

###ソースコード
	https://github.com/chikuzen/MPEG2DecPlus/


