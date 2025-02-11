#include <stdio.h>
#include <stdlib.h>
#include "binReader.h"

///Источник данных
FILE *src;
///"b" - big endianness, "l" - little endianness
char cur_endianness = 'b';
///Счетчик бит в текущем байте
ushort bit_count;
///Текущее значение байта для побитового чтения
ushort cur_byte;

//Устанавливает источник для чтения байт
void set_bin_src(char *source)
{
    char *filename;
    if (source == NULL)
        filename = "Aqours.jpg";
    else 
        filename = source;
    src = fopen(filename, "rb");
    bit_count = 0;
}

//Устанавливает порядок байт на order
void set_endiann(char order)
{
    if (order == 'b' || order == 'l')
        cur_endianness = order;
    else printf("Error: invalid endiann char");
    bit_count = 0;
}

//Возвращает очередной байт
ushort get_byte()
{
    return getc(src);
}

//Возвращает следующий байт без изменения указателя
ushort get_next_byte()
{
    ushort ans = getc(src);
    fseek(src, -1, SEEK_CUR);
    return ans;
}

//Возвращает очередные два байта
ushort get_word()
{
    ushort ans = getc(src);
    if (cur_endianness == 'b') {   
        ans = ans << 8;
        ans += getc(src);
    }
    else {
        int temp = getc(src);
        temp = temp << 8;
        ans += temp;
    }
    return ans;
}

//Возвращает структуру с парой 4 бит
bit4 get_4bit()
{
    ushort temp = getc(src);
    bit4 ans;
    ans.first = temp >> 4;
    ans.second = temp & 15;
    return ans;
}

//Возвращает очередной бит
ushort get_bit()
{
    //printf("cur_byte: %x\n", cur_byte);
    if (cur_endianness == 'b') {
        if (bit_count == 0) {
            cur_byte = get_byte();
            bit_count = 8;
        }
        if (cur_byte == 0x00)
        {
            cur_byte = get_byte();
        }
        ushort temp = cur_byte >> --bit_count;
        return temp & 1;
    }
    else if (cur_endianness == 'l') {
        if (bit_count == 7) {
            cur_byte = get_byte();
            bit_count = -1;
        }
        if (cur_byte == 0x00)
        {
            cur_byte = get_byte();
        }
        ushort temp = cur_byte >> ++bit_count;
        return temp & 1;
    }
    printf("get_bit -> Error");
    return -1;
}

//Читает n бит
ushort get_bits(ushort n)
{
    ushort ans = 0;
    for (ushort i = 0; i < n; ++i) {
        ans = ans << 1;
        ans += get_bit();
    }
    return ans;
}

//Читает n байт
ushort *get_array(ushort n)
{
    ushort *res = calloc(n, sizeof(ushort));
    for (ushort i = 0; i < n; ++i)
        res[i] = get_byte();
    return res;
}