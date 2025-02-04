#include <stdio.h>
#include "binReader.h"
#include "assert.h"

int main(void)
{
    set_bin_src(NULL);
    set_endiann('b');
    ASSERT(get_byte(), 255);
    ASSERT(get_next_byte(), 216);
    ASSERT(get_byte(), 216);
    ASSERT(get_byte(), 255);
    ASSERT(get_word(), 57344);
    bit4 temp = get_4bit();
    ASSERT(temp.first, 1);
    ASSERT(temp.second, 0);
    printf("=======bit-test=======\n");
    ASSERT(get_bit(), 0);
    ASSERT(get_bit(), 1);
    ASSERT(get_bit(), 0);
    ASSERT(get_bit(), 0);
    ASSERT(get_bits(4), 10);
    printf("=======\n");
    short *arr = get_array(3);
    ASSERT(arr[0], 70);
    ASSERT(arr[1], 73);
    ASSERT(arr[2], 70);
}