typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

typedef struct component_s{
    ushort h;
    ushort v;
    ushort tq;
    uchar dc_table;
    uchar ac_table;
} component;

typedef struct huff_table_s{
    uchar *offset;
    uchar *symbols;
    uint *codes;
} huff_table;

void print_table(ushort *table);
void print_component(component *comp);
component *make_component(ushort h, ushort v, ushort tq);
huff_table *make_huff_table(uchar *offset, uchar *symbol, uint *codes);
void get_huff_table(huff_table *huff);