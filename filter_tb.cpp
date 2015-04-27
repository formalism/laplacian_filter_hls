// Testbench of filter.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include "image.h"
#include <iostream>
#include <fstream>

#define HORIZONTAL_PIXEL_WIDTH    240
#define VERTICAL_PIXEL_WIDTH    120
#define ALL_PIXEL_VALUE    (HORIZONTAL_PIXEL_WIDTH*VERTICAL_PIXEL_WIDTH)

int lap_filter_axim(hls::stream<ap_axiu<32,1,1,1> >& in, hls::stream<ap_axiu<32,1,1,1> >& out);

using namespace std;

int main()
{
    hls::stream<ap_axiu<32,1,1,1> > in;
    hls::stream<ap_axiu<32,1,1,1> > out;
    ap_axiu<32,1,1,1> val;
    ap_axiu<32,1,1,1> lap;
    unsigned char pix[3];
    ofstream ofs("result.data", ios::binary);
    ofstream img("image.data", ios::binary);
    char buf[3];
    
    for (int j = 0; j < VERTICAL_PIXEL_WIDTH; j++)
        for (int i = 0; i < HORIZONTAL_PIXEL_WIDTH; i++){
            HEADER_PIXEL(header_data, pix);
            img.write((char*)pix, 3);
            val.data = (pix[0]<<16)|(pix[1]<<8)|(pix[2]);
            if (i == HORIZONTAL_PIXEL_WIDTH-1){
                val.user = 1;
                if (j == VERTICAL_PIXEL_WIDTH-1)
                    val.last = 1;
                else
                    val.last = 0;
            }else{
                val.user = 0;
            }
            in.write(val);
        }

    lap_filter_axim(in, out);

    // o—Í‰æ‘f”‚Í‚’¼A…•½‚Æ‚à‚É-2‚³‚ê‚Ä‚¢‚é‚Ì‚Å238x118‚É‚È‚é
    do{
        lap = out.read();
        buf[0] = (lap.data >> 16) & 0xFF;
        buf[1] = (lap.data >> 8) & 0xFF;
        buf[2] = lap.data & 0xFF;
        ofs.write(buf, 3);
    }while (!lap.last);
    
    ofs.close();
    img.close();
    
    return 0;
}
