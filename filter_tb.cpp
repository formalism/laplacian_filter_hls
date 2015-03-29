// Testbench of filter.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>
#include "image.h"
#include <iostream>
#include <fstream>

#define HORIZONTAL_PIXEL_WIDTH    240
#define VERTICAL_PIXEL_WIDTH    120
#define ALL_PIXEL_VALUE    (HORIZONTAL_PIXEL_WIDTH*VERTICAL_PIXEL_WIDTH)

int lap_filter_axim(hls::stream<ap_int<32> >& in, hls::stream<ap_int<32> >& out);

using namespace std;

int main()
{
    hls::stream<ap_int<32> > in;
    hls::stream<ap_int<32> > out;
    unsigned char pix[3];
    ofstream ofs("result.data", ios::binary);
    ofstream img("image.data", ios::binary);
    char buf[3];

    for (int j = 0; j < VERTICAL_PIXEL_WIDTH; j++)
        for (int i = 0; i < HORIZONTAL_PIXEL_WIDTH; i++){
            HEADER_PIXEL(header_data, pix);
            img.write((char*)pix, 3);
            in.write((pix[0]<<16)|(pix[1]<<8)|(pix[2]));
        }

    lap_filter_axim(in, out);

    // o—Í‰æ‘f”‚Í‚’¼A…•½‚Æ‚à‚É-2‚³‚ê‚Ä‚¢‚é‚Ì‚Å238x118‚É‚È‚é
    for (int j = 0; j < VERTICAL_PIXEL_WIDTH-2; j++)
        for (int i = 0; i < HORIZONTAL_PIXEL_WIDTH-2; i++){
            ap_int<32> val = out.read();
            buf[0] = (val >> 16) & 0xFF;
            buf[1] = (val >> 8) & 0xFF;
            buf[2] = val & 0xFF;
            ofs.write(buf, 3);
        }

    return 0;
}
