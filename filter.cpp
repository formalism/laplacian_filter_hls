/**
 * Laplacian filter by AXI4-Stream input/output.
 * Achieves 1 clk/pixel.
 */
#include <stdio.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define HORIZONTAL_PIXEL_WIDTH    240

int laplacian_fil(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2);
int conv_rgb2y(int rgb);

int lap_filter_axim(hls::stream<ap_axiu<32,1,1,1> >& in, hls::stream<ap_axiu<32,1,1,1> >& out)
{
#pragma HLS INTERFACE axis port=in
#pragma HLS INTERFACE axis port=out
#pragma HLS INTERFACE s_axilite port=return

    ap_axiu<32, 1, 1, 1> val;   // input value
    ap_axiu<32, 1, 1, 1> lap;   // output value
    int i=0, j=0;

    unsigned int y_buf[2][HORIZONTAL_PIXEL_WIDTH];
#pragma HLS array_partition variable=y_buf block factor=2 dim=1
#pragma HLS resource variable=y_buf core=RAM_2P

    unsigned int window[3][3];  // filter window
#pragma HLS array_partition variable=window complete

    do {
#pragma HLS DEPENDENCE variable=y_buf inter false
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=240*120 max=240*120
        val = in.read();
        int y = conv_rgb2y(val.data);

        window[0][0] = window[0][1];    // 水平方向シフトレジスタ :-)
        window[0][1] = window[0][2];
        window[0][2] = y_buf[0][i];
        window[1][0] = window[1][1];
        window[1][1] = window[1][2];
        window[1][2] = y_buf[1][i];
        window[2][0] = window[2][1];
        window[2][1] = window[2][2];
        window[2][2] = y;       // 現在の入力
        
        y_buf[0][i] = y_buf[1][i];      // 垂直方向シフト :-<
        y_buf[1][i] = y;
        
        int lapval = laplacian_fil(window[0][0], window[0][1], window[0][2],
                                   window[1][0], window[1][1], window[1][2],
                                   window[2][0], window[2][1], window[2][2]);
        
        lap.data = (lapval<<16)|(lapval<<8)|lapval;
        lap.last = val.last;    // copy tlast
        if (j >= 2 && i >= 2)   // 無効の部分は出力しないので水平垂直2画素ずつ減る
            out.write(lap);
        
        if (val.user){
            i = 0;
            j++;
        }else{
            i++;
        }
    }while (!val.last);

    return 0;
}

// RGBからYへの変換
// RGBのフォーマットは、{8'd0, R(8bits), G(8bits), B(8bits)}, 1pixel = 32bits
// 輝度信号Yのみに変換する。変換式は、Y =  0.299R + 0.587G + 0.114B
// "YUVフォーマット及び YUV<->RGB変換"を参考にした。http://vision.kuee.kyoto-u.ac.jp/~hiroaki/firewire/yuv.html
//　2013/09/27 : float を止めて、すべてint にした
int conv_rgb2y(int rgb){
    int r,g,b;
    int y_f;
    int y;

    r = (rgb >> 16) & 0xFF;
    g = (rgb >> 8) & 0xFF;
    b = rgb & 0xFF;
    y_f = 77*r + 150*g + 29*b; //y_f = 0.299*r + 0.587*g + 0.114*b;の係数に256倍した
    y = y_f >> 8; // 256で割る

    return(y);
}

// ラプラシアンフィルタ
// x0y0 x1y0 x2y0 -1 -1 -1
// x0y1 x1y1 x2y1 -1  8 -1
// x0y2 x1y2 x2y2 -1 -1 -1
int laplacian_fil(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2)
{
    int y;

    y = -x0y0 -x1y0 -x2y0 -x0y1 +8*x1y1 -x2y1 -x0y2 -x1y2 -x2y2;
    if (y<0)
        y = 0;
    else if (y>255)
        y = 255;
    return(y);
}
