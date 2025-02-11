#include <stdlib.h>
#include <stdio.h>
#include "data_unit.h"

short *prev;

//Инициализация декодирования потока, предыдущие значения
void data_unit_init(uchar num_of_elem)
{
    prev = calloc(num_of_elem, sizeof(short));
    for (int i = 0; i < num_of_elem; ++i)
        prev[i] = 0;
    printf("data_unit_init -> complete\n");
}

//Декодирование знака в потоке Хаффмана
short decode_sign(ushort num, short len)
{
    return num >= (1 << len - 1) ? num : num - (1 << len) + 1;
}

//Декодирование из битового потока значений Хаффмана
uchar decode_huff(huff_table *huff)
{
    ushort code = 0;
    ushort code_len = 0;
    int counter = 0;
    while (counter < 10000)
    {
        code <<= 1;
        code += get_bit();
        ++code_len;
        for (int i = huff->offset[code_len - 1]; i < huff->offset[code_len]; ++i)
        {
            if (huff->codes[i] == code)
                return huff->symbols[i];
        }
        counter++;
    }
    printf("decode_huff -> Error: code has not been found");
    return (uchar)code;
}

//Декодирование DC-коэффициента
short decode_dc(uchar elem_id, huff_table *huff)
{
    short temp = decode_huff(huff);
    short diff = decode_sign(get_bits(temp), temp);
    temp = diff + prev[elem_id];
    prev[elem_id] = temp;
    return temp;
}

//Декодирование AC-коэффициента
void decode_ac(short *unit, huff_table *huff)
{
    uchar k = 1;
    while (k < UNIT_LEN)
    {
        uchar rs = decode_huff(huff);
        uchar big = rs >> 4;
        uchar small = rs & 0x0f;
        printf("%d: %d\n", k, big);
        if (small == 0)
            if (big != 15)
                return;
            else 
            {
                k += 16;
                continue;
            }
        k += big;
        if (k >= UNIT_LEN)
        {
            printf("decode_ac -> Error: k bigger than unit length");
            return;
        }
        ushort bits = get_bits(small);
        //printf("%d: %d -> %x\n", k, small, bits);
        unit[k] = decode_sign(bits, small);
        k++;
    }
}

//Деквантование
void dequant(short *unit, ushort *table)
{
    for (int i = 0; i < UNIT_LEN; ++i)
        unit[i] *= table[i];
}

//Восстановление данных по зиг-загу 
short *zig_zag_order(short *unit)
{
    short *res = calloc(UNIT_LEN, sizeof(short));
    for (int i = 0; i < ROW_COUNT; ++i)
        for (int j = 0; j < ROW_COUNT; ++j)
            res[i * 8 + j] = unit[ZIGZAG[i * 8 + j]];
    free(unit);
    return res;
}

//Обратное дискретно-косинусное преобразование
void inverse_cosin(short *unit)
{
    float intermediate[64];

    for (uint i = 0; i < 8; ++i) {
        const float g0 = unit[0 * 8 + i] * s0;
        const float g1 = unit[4 * 8 + i] * s4;
        const float g2 = unit[2 * 8 + i] * s2;
        const float g3 = unit[6 * 8 + i] * s6;
        const float g4 = unit[5 * 8 + i] * s5;
        const float g5 = unit[1 * 8 + i] * s1;
        const float g6 = unit[7 * 8 + i] * s7;
        const float g7 = unit[3 * 8 + i] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        intermediate[0 * 8 + i] = b0 + b7;
        intermediate[1 * 8 + i] = b1 + b6;
        intermediate[2 * 8 + i] = b2 + b5;
        intermediate[3 * 8 + i] = b3 + b4;
        intermediate[4 * 8 + i] = b3 - b4;
        intermediate[5 * 8 + i] = b2 - b5;
        intermediate[6 * 8 + i] = b1 - b6;
        intermediate[7 * 8 + i] = b0 - b7;
    }
    for (uint i = 0; i < 8; ++i) {
        const float g0 = intermediate[i * 8 + 0] * s0;
        const float g1 = intermediate[i * 8 + 4] * s4;
        const float g2 = intermediate[i * 8 + 2] * s2;
        const float g3 = intermediate[i * 8 + 6] * s6;
        const float g4 = intermediate[i * 8 + 5] * s5;
        const float g5 = intermediate[i * 8 + 1] * s1;
        const float g6 = intermediate[i * 8 + 7] * s7;
        const float g7 = intermediate[i * 8 + 3] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        unit[i * 8 + 0] = b0 + b7 + 0.5f;
        unit[i * 8 + 1] = b1 + b6 + 0.5f;
        unit[i * 8 + 2] = b2 + b5 + 0.5f;
        unit[i * 8 + 3] = b3 + b4 + 0.5f;
        unit[i * 8 + 4] = b3 - b4 + 0.5f;
        unit[i * 8 + 5] = b2 - b5 + 0.5f;
        unit[i * 8 + 6] = b1 - b6 + 0.5f;
        unit[i * 8 + 7] = b0 - b7 + 0.5f;
    }
}

//Декодирование одного блока 64
short *decode_data_unit(uchar elem_id, huff_table *dc, huff_table *ac, ushort *quant_table)
{
    printf("decode_data_unit\n");
    short *unit = calloc(UNIT_LEN, sizeof(short));
    for (int i = 0; i < UNIT_LEN; ++i)
        unit[i] = 0;
    unit[0] = decode_dc(elem_id, dc);
    decode_ac(unit, ac);
    unit = zig_zag_order(unit);
    print_data_unit(unit);
    dequant(unit, quant_table);
    //unit = zig_zag_order(unit);
    inverse_cosin(unit);
    //print_data_unit(unit);
    return unit;
}

//Конструктор пикселя
pixel *make_pixel(uchar r, uchar g, uchar b)
{
    pixel *res = malloc(sizeof(pixel));
    res->RGB.R = r;
    res->RGB.G = g;
    res->RGB.B = b;
    return res;
}

//Вывод data_unit
void print_data_unit(short *unit)
{
    for (int i = 0; i < ROW_COUNT; ++i)
    {
        for (int j = 0; j < ROW_COUNT; ++j)
            printf("%d\t", unit[i * 8 + j]);
        printf("\n");
    }
    printf("\n\n");
}