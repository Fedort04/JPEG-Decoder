#include <stdio.h>
#include <stdlib.h>
#include "binReader.h"

///Источник данных
FILE *read_src;
///"b" - big endianness, "l" - little endianness
char cur_endianness = 'b';
///Счетчик бит в текущем байте
int bit_count;
///Текущее значение байта для побитового чтения
ushort cur_byte;
///Предыдущий байт для побитового чтения
ushort prev_byte;

//Устанавливает источник для чтения байт
void set_bin_src(char *source)
{
    char *filename;
    if (source == NULL)
        filename = "Aqours.jpg";
    else 
        filename = source;
    read_src = fopen(filename, "rb");
    bit_count = 0;
    cur_byte = 0;
    prev_byte = 0;
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
    return getc(read_src);
}

//Возвращает следующий байт без изменения указателя
ushort get_next_byte()
{
    ushort ans = getc(read_src);
    fseek(read_src, -1, SEEK_CUR);
    return ans;
}

//Возвращает очередные два байта
ushort get_word()
{
    ushort ans = getc(read_src);
    if (cur_endianness == 'b') {   
        ans = ans << 8;
        ans += getc(read_src);
    }
    else {
        int temp = getc(read_src);
        temp = temp << 8;
        ans += temp;
    }
    return ans;
}

//Возвращает структуру с парой 4 бит
bit4 get_4bit()
{
    ushort temp = getc(read_src);
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
            prev_byte = cur_byte;
            cur_byte = get_byte();
            bit_count = 8;
        }
        if (prev_byte == 0xFF && cur_byte == 0x00)
        {
            cur_byte = get_byte();
        }
        ushort temp = cur_byte >> --bit_count;
        return temp & 1;
    }
    else if (cur_endianness == 'l') {
        if (bit_count == 7) {
            prev_byte = cur_byte;
            cur_byte = get_byte();
            bit_count = -1;
        }
        if (prev_byte == 0xFF && cur_byte == 0x00)
        {
            cur_byte = get_byte();
        }
        ushort temp = cur_byte >> ++bit_count;
        return temp & 1;
    }
    printf("get_bit -> Error");
    return -1;
}

//Пропускает оставшиеся биты текущего байта
void bits_align()
{
    if (cur_endianness == 'b')
    {
        prev_byte = cur_byte;
        cur_byte = get_byte();
        bit_count = 8;
    }
    else if (cur_endianness == 'l')
    {
        prev_byte = cur_byte;
        cur_byte = get_byte();
        bit_count = -1;
    }
    else 
    {
        printf("bits_align -> Error: invalid endianes");
    }
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