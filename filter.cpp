/**
 * Laplacian filter by AXI4-Stream input/output.
 * Achieves 2pixel/clk.
 */
#include <stdio.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>

#define HORIZONTAL_PIXEL_WIDTH    240
#define VERTICAL_PIXEL_WIDTH    120
#define ALL_PIXEL_VALUE    (HORIZONTAL_PIXEL_WIDTH*VERTICAL_PIXEL_WIDTH)

ap_uint<8> laplacian_fil(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2);
int conv_rgb2y(int rgb);

#define UPPER32BITS(x) (((x)>>32)&0xFFFFFFFF)
#define LOWER32BITS(x) (((x)&0xFFFFFFFF))
#define COMBINE2PIX(x,y) (((x)<<16)|(y))
#define UPPER16BITS(x) (((x)>>16)&0xFFFF)
#define LOWER16BITS(x) (((x)&0xFFFF))
#define COMBINE2Y(y1, y2) ((y1)<<48|(y1)<<40|(y1)<<32|(y2)<<16|(y2)<<8|(y2))

int lap_filter_axim(hls::stream<ap_uint<64> >& in, hls::stream<ap_uint<64> >& out)
{
#pragma HLS INTERFACE axis port=in
#pragma HLS INTERFACE axis port=out
#pragma HLS INTERFACE s_axilite port=return

        ap_uint<32> y_buf[2][HORIZONTAL_PIXEL_WIDTH/2]; // 上位16bit=左側, 下位16bit=右側 として使う
#pragma HLS array_partition variable=y_buf block factor=2 dim=1
#pragma HLS resource variable=y_buf core=RAM_2P

        ap_uint<32> window[3][2];   // 水平方向は4画素分(隣り合う3x3を2つ取れる)
#pragma HLS array_partition variable=window complete

    for (int j = 0; j < VERTICAL_PIXEL_WIDTH; j++){
                for (int i = 0; i < HORIZONTAL_PIXEL_WIDTH/2; i++){
#pragma HLS PIPELINE
                        ap_uint<64> val = in.read();
                        int y1 = conv_rgb2y(UPPER32BITS(val));
                        int y2 = conv_rgb2y(LOWER32BITS(val));

                        window[0][0] = window[0][1];    // 水平方向シフトレジスタ :-)
                        window[0][1] = y_buf[0][i];
                        window[1][0] = window[1][1];
                        window[1][1] = y_buf[1][i];
                        window[2][0] = window[2][1];
                        window[2][1] = COMBINE2PIX(y1, y2);       // 現在の入力

                        y_buf[0][i] = y_buf[1][i];      // 垂直方向シフト :-<
                        y_buf[1][i] = COMBINE2PIX(y1, y2);

                        ap_uint<64> val1 = laplacian_fil(UPPER16BITS(window[0][0]), LOWER16BITS(window[0][0]), UPPER16BITS(window[0][1]),
                                                         UPPER16BITS(window[1][0]), LOWER16BITS(window[1][0]), UPPER16BITS(window[1][1]),
                                                         UPPER16BITS(window[2][0]), LOWER16BITS(window[2][0]), UPPER16BITS(window[2][1]));
                        ap_uint<64> val2 = laplacian_fil(LOWER16BITS(window[0][0]), UPPER16BITS(window[0][1]), LOWER16BITS(window[0][1]),
                                                         LOWER16BITS(window[1][0]), UPPER16BITS(window[1][1]), LOWER16BITS(window[1][1]),
                                                         LOWER16BITS(window[2][0]), UPPER16BITS(window[2][1]), LOWER16BITS(window[2][1]));
                        if (j >= 2 && i >= 1)   // 無効の部分は出力しないので水平垂直2画素ずつ減る
                                out.write(COMBINE2Y(val1, val2));
                }
    }

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
ap_uint<8> laplacian_fil(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2)
{
    int y;

    y = -x0y0 -x1y0 -x2y0 -x0y1 +8*x1y1 -x2y1 -x0y2 -x1y2 -x2y2;
    if (y<0)
        y = 0;
    else if (y>255)
        y = 255;
    return(y);
}
