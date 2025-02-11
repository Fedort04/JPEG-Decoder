#ifndef _BIT4_
#define _BIT4_
typedef struct bit4_s{
    unsigned char first;
    unsigned char second;
} bit4;
#endif

typedef unsigned short ushort;

void set_bin_src(char *source);
void set_endiann(char order);
ushort get_byte();
ushort get_next_byte();
ushort get_word();
bit4 get_4bit();
ushort get_bit();
ushort get_bits(ushort n);
ushort *get_array(ushort n);