typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

#ifndef _COMPONENT_
#define _COMPONENT_
typedef struct component_s{
    ushort h;
    ushort v;
    ushort tq;
    uchar dc_table;
    uchar ac_table;
} component;
#endif

#ifndef _HUFF_TABLE_
#define _HUFF_TABLE_
typedef struct huff_table_s{
    uchar *offset;
    uchar *symbols;
    uint *codes;
} huff_table;
#endif

void print_table(ushort *table);
void print_component(component *comp);
component *make_component(ushort h, ushort v, ushort tq);
huff_table *make_huff_table(uchar *offset, uchar *symbol, uint *codes);
void print_huff_table(huff_table *huff);
void get_huff_table(huff_table *huff);