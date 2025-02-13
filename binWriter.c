//#include <stdlib.h>
#include <stdio.h>
#include "binWriter.h"

//Источник для записи
FILE *write_src;

//Устанавливает источник для записи (little-endian)
void set_bin_output(char *source)
{
    write_src = fopen(source, "wb");
}

//Закрыть записываемый файл
void close_bin_output()
{
    fclose(write_src);
}

//Записать в write_src 4 байта num
void put_int(uint num)
{
    putc((uchar)(num >> 0), write_src);
    putc((uchar)(num >> 8), write_src);
    putc((uchar)(num >> 16), write_src);
    putc((uchar)(num >> 24), write_src);
}

//Записать в write_src 2 байта num
void put_short(uint num)
{
    putc((uchar)(num >> 0), write_src);
    putc((uchar)(num >> 8), write_src);
}

//Записать 1 байт num
void put_char(uchar num)
{
    putc(num, write_src);
}