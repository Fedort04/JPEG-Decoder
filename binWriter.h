#ifndef _BIN_WRITER_H_
#define _BIN_WRITER_H_
typedef unsigned char uchar;
typedef unsigned int uint;

void set_bin_output(char *source);
void close_bin_output();
void put_int(uint num);
void put_short(uint num);
void put_char(uchar num);
#endif