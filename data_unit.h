#include <math.h>
#include "background.h"
#include "binReader.h"

#define UNIT_LEN 64
#define ROW_COUNT 8

typedef unsigned short ushort;
typedef unsigned char uchar;

#ifndef _PIXEL_
#define _PIXEL_
typedef struct pixel_s{
    union {
        struct {
            uchar R;
            uchar G;
            uchar B;
        } RGB;
        struct {
            uchar Y;
            uchar Cb;
            uchar Cr;
        } YCbCr;
    };
} pixel;
#endif

#ifndef _DATA_UNIT_H_
#define _DATA_UNIT_H_
//Последовательность зиг-зага
const static uchar ZIGZAG[64] = {
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

static const double IDCT_TABLE[64] = {
    0.707107,  0.707107,  0.707107,  0.707107,  0.707107,  0.707107,  0.707107,  0.707107,
    0.980785,  0.831470,  0.555570,  0.195090, -0.195090, -0.555570, -0.831470, -0.980785,
    0.923880,  0.382683, -0.382683, -0.923880, -0.923880, -0.382683,  0.382683,  0.923880,
    0.831470, -0.195090, -0.980785, -0.555570,  0.555570,  0.980785,  0.195090, -0.831470,
    0.707107, -0.707107, -0.707107,  0.707107,  0.707107, -0.707107, -0.707107,  0.707107,
    0.555570, -0.980785,  0.195090,  0.831470, -0.831470, -0.195090,  0.980785, -0.555570,
    0.382683, -0.923880,  0.923880, -0.382683, -0.382683,  0.923880, -0.923880,  0.382683,
    0.195090, -0.555570,  0.831470, -0.980785,  0.980785, -0.831470,  0.555570, -0.195090
};

//Константы для ОДКП
const static float m0 = 2.0 * cos(1.0 / 16.0 * 2.0 * M_PI);
static const float m1 = 2.0 * cos(2.0 / 16.0 * 2.0 * M_PI);
static const float m3 = 2.0 * cos(2.0 / 16.0 * 2.0 * M_PI);
static const float m5 = 2.0 * cos(3.0 / 16.0 * 2.0 * M_PI);
static const float m2 = m0 - m5;
static const float m4 = m0 + m5;

static const float s0 = cos(0.0 / 16.0 * M_PI) / sqrt(8);
static const float s1 = cos(1.0 / 16.0 * M_PI) / 2.0;
static const float s2 = cos(2.0 / 16.0 * M_PI) / 2.0;
static const float s3 = cos(3.0 / 16.0 * M_PI) / 2.0;
static const float s4 = cos(4.0 / 16.0 * M_PI) / 2.0;
static const float s5 = cos(5.0 / 16.0 * M_PI) / 2.0;
static const float s6 = cos(6.0 / 16.0 * M_PI) / 2.0;
static const float s7 = cos(7.0 / 16.0 * M_PI) / 2.0;
#endif

void data_unit_init(uchar num_of_elem);
void print_data_unit(short *unit);
short *decode_data_unit(uchar elem_id, huff_table *dc, huff_table *ac, ushort *quant_table);
pixel *make_pixel(uchar r, uchar g, uchar b);