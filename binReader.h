typedef struct {
    short first;
    short second;
} bit4;

void set_bin_src(char *source);
void set_endiann(char order);
short get_byte();
short get_next_byte();
int get_word();
bit4 get_4bit();
short get_bit();
short get_bits(short n);
short *get_array(short n);