typedef unsigned short ushort;

typedef struct component_s{
    ushort h;
    ushort v;
    ushort tq;
} component;

void print_table(ushort *table);
void print_component(component *comp);
component *make_component(ushort h, ushort v, ushort tq);