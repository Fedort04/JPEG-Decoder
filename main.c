#include <stdio.h>
#include <stdlib.h>
#include "binReader.h"
#include "background.h"
#include "main.h"

ushort *quant_tables[NUM_OF_TABLES]; //Массив таблиц квантования
ushort sample_precision; //precision of bits
ushort x; //Ширина
ushort y; //Высота
ushort nf; //Кол-во компонент
component **comps; //Массив ссылок на данные о компонентах

//Чтение маркера marker
//return 1, если маркер прочитан корректно
char read_marker(ushort marker)
{
    if (get_word() == marker)
        return 1;
    printf("read_marker -> Error: marker %x has not been found\n", marker);
    return 0;
}

//Чтение сегмента приложения
void read_app()
{
    printf("APP\n");
    ushort ln = get_word();
    ushort *temp = get_array(ln - 2);
    free(temp);
}

//Чтение таблицы квантования
void read_quant_table()
{
    printf("DQT\n");
    ushort ln = get_word();
    ushort tq;
    ushort *table;
    while (get_next_byte() != 0xFF)
    {
        tq = get_byte();
        if (tq > 3)
        {
            printf("read_quant_table -> Error: incorrect table destination %d", tq);
            return;
        }
        table = get_array(QUANT_TABLE_SIZE);
        quant_tables[tq] = table;
        printf("destination: %d\n", tq);
        print_table(quant_tables[tq]);
    }
}

//Чтение и конструирование таблицы Хаффмана
void read_huff_table()
{
    ushort lh = get_word();
    bit4 tcth = get_4bit();
    uchar *offset = calloc(NUM_HUFF_CODE_LEN + 1, sizeof(uchar));
    offset[0] = 0;
    uchar sum_elem = 0;
    for (int i = 1; i < NUM_HUFF_CODE_LEN + 1; ++i)
    {
        sum_elem += get_byte();
        offset[i] = sum_elem;
    }
    if (sum_elem > MAX_NUM_HUFF_SYM)
    {
        printf("read_huff_table -> Error: incorrect number of symbols");
        return;
    }
    uchar *symbols = calloc(sum_elem, sizeof(uchar));
    for (int i = 0; i < sum_elem; ++i)
    {
        symbols[i] = get_byte();
    }
    //Коструирование таблицы из прочтенных данных!!
}

//Чтение таблиц и прочих мелких сегментов
//return следующие за сегментами два байта
ushort read_tables()
{
   ushort temp = get_word();
   uchar next = 0;
   if (temp >= APP0 && temp <= APP15)
    {
        read_app(); 
        next = 1;
    }
    else if (temp == DQT)
    {
        read_quant_table();
        next = 1;
    }
    else if (temp == DHT)
    {
        read_huff_table();
        next = 1;
    }
    if (next)
        temp = read_tables();
    return temp;
}

//Чтение заголовка фрейма
void read_frame_header()
{
    ushort lf = get_word();
    sample_precision = get_byte();
    y = get_word();
    x = get_word();
    nf = get_byte();
    comps = calloc(nf, sizeof(component));
    printf("sample_pricision: %d\n", sample_precision);
    printf("x: %d\n", x);
    printf("y: %d\n", y);
    printf("num of components: %d\n", nf);
    for (int i = 0; i < nf; ++i)
    {
        ushort c = get_byte();
        bit4 hv = get_4bit();
        ushort tq = get_byte();
        comps[c - 1] = make_component(hv.first, hv.second, tq);
        printf("component %d\n", c);
        print_component(comps[c - 1]);
    }
}

//Чтение сегмента скана
void read_scan()
{
    ushort marker = read_tables();
}

//Чтение кадра
void read_frame()
{
    ushort marker = read_tables();
    if (marker != SOF)
    {
        printf("read_frame -> Error: invalid marker %x", marker);
        return;
    }
    read_frame_header();
    read_scan();
}

//Чтение файла основной
void read_jpeg(char *source)
{
    set_bin_src(source);
    set_endiann('b');
    if (!read_marker(SOI)) //Проверка начала файла
        return;
    printf("SOI\n");
    read_frame();
    if (!read_marker(EOI)) //Проверка конца файла
        return;
}

int main(void)
{
    read_jpeg(NULL);
}