#include <stdio.h>
#include <stdlib.h>
#include "binReader.h"
#include "background.h"
#include "main.h"
#include "data_unit.h"

ushort *quant_tables[NUM_OF_TABLES]; //Массив таблиц квантования
huff_table *DC_tables[NUM_OF_TABLES]; //Массив таблиц Хаффмана для DC
huff_table *AC_tables[NUM_OF_TABLES]; //Массив таблиц Хаффмана для АС
ushort sample_precision; //precision of bits
ushort x; //Ширина
ushort y; //Высота
ushort nf; //Кол-во компонент
component **comps; //Массив ссылок на данные о компонентах
ushort restart_interval = 0; //Перезапуск MCU, по умолчанию 0
ushort start_spectral = 0; //Для прогрессивного
ushort end_spectral = 63; //Для прогрессивного
bit4 ahal;

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
    uint *codes = calloc(sum_elem, sizeof(uint));
    for (int i = 0; i < sum_elem; ++i)
    {
        symbols[i] = get_byte();
    }
    huff_table *temp = make_huff_table(offset, symbols, codes);
    get_huff_table(temp);
    if (tcth.second > 3)
    {
        printf("read_huff_table -> Error: incorrect table destination");
        return;
    }
    char sym = 'e';
    if (tcth.first == 0)
    {
        DC_tables[tcth.second] = temp;
        sym = 'D';
    }
    else if (tcth.first == 1)
    {
        AC_tables[tcth.second] = temp;
        sym = 'A';
    }
    else 
    {
        printf("read_huff_table -> Error: incorrect table ID");
        return;
    }
    printf("DHT\n");
    printf("%cC-table %d\n\n", sym, tcth.second);
}

//Чтение сегмента с перезапуском
void read_restart_interval()
{
    printf("DRI\n");
    ushort lr = get_word();
    restart_interval = get_word();
    printf("restart-interval: %d\n\n", restart_interval);
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
    else if (temp == DRI)
    {
        read_restart_interval();
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
    comps = calloc(nf, sizeof(component*));
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

//Чтение заголовка скана
void read_scan_header()
{
    printf("SOS\n");
    ushort ls = get_word();
    uchar ns = get_byte();
    for (int i = 0; i < ns; ++i)
    {
        uchar cs = get_byte();
        bit4 tdta = get_4bit();
        comps[cs - 1]->dc_table = tdta.first;
        comps[cs - 1]->ac_table = tdta.second;
        printf("component %d:\tDC: %d\tAC: %d\n", cs, tdta.first, tdta.second);
    }
    start_spectral = get_byte();
    printf("start-spectral-selection: %d\n", start_spectral);
    end_spectral = get_byte();
    printf("end-spectral-selection: %d\n", end_spectral);
    ahal = get_4bit();
    printf("approximation-high: %d\tapproximation-low: %d\n\n", ahal.first, ahal.second);
}

//Декодирование одного MCU
short **decode_mcu()
{
    uchar num_of_Y = comps[0]->h * comps[0]->v;
    uchar num_of_Cb = comps[1]->h * comps[1]->v;
    uchar num_of_Cr = comps[2]->h * comps[2]->v;
    short **data[num_of_Y + num_of_Cb + num_of_Cr];
    for (int i = 0; i < num_of_Y; ++i)
        data[i] = decode_data_unit(0, comps[0]->dc_table, comps[0]->ac_table, quant_tables[0]);
    data[4] = decode_data_unit(1, comps[1]->dc_table, comps[1]->ac_table, quant_tables[1]);
    data[5] = decode_data_unit(2, comps[2]->dc_table, comps[2]->ac_table, quant_tables[2]);
    return data;
}

//Чтение сегмента скана
void read_scan(pixel ***im)
{
    ushort marker = read_tables();
    read_scan_header();
    //Example-----
    data_unit_init(3);
    short **mcu = decode_mcu();
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            {
                im[i][j]->YCbCr.Y = mcu[0][i * 8 + j];
                im[i][j]->YCbCr.Cb = mcu[4][(i / 2) * 8 + (j / 2)];
                im[i][j]->YCbCr.Cr = mcu[5][(i / 2) * 8 + (j / 2)];
            }
    for (int i = 8; i < 16; ++i)
        for (int j = 0; j < 8; ++j)
            {
                im[i][j]->YCbCr.Y = mcu[1][i * 8 + j];
                im[i][j]->YCbCr.Cb = mcu[4][(i / 2) * 8 + (j / 2)];
                im[i][j]->YCbCr.Cr = mcu[5][(i / 2) * 8 + (j / 2)];
            }
    for (int i = 0; i < 8; ++i)
        for (int j = 8; j < 16; ++j)
            {
                im[i][j]->YCbCr.Y = mcu[2][i * 8 + j];
                im[i][j]->YCbCr.Cb = mcu[4][(i / 2) * 8 + (j / 2)];
                im[i][j]->YCbCr.Cr = mcu[5][(i / 2) * 8 + (j / 2)];
            }
    for (int i = 8; i < 16; ++i)
        for (int j = 8; j < 16; ++j)
            {
                im[i][j]->YCbCr.Y = mcu[3][i * 8 + j];
                im[i][j]->YCbCr.Cb = mcu[4][(i / 2) * 8 + (j / 2)];
                im[i][j]->YCbCr.Cr = mcu[5][(i / 2) * 8 + (j / 2)];
            }
    for (int i = 0; i < 16; ++i)
        for (int j= 0; j < 16; ++j)
            {
                pixel *prev = im[i][j];
                prev->YCbCr.Y += 128;
                prev->YCbCr.Cb += 128;
                prev->YCbCr.Cr += 128;
                im[i][j] = make_pixel(prev->YCbCr.Y + 1.402 * (prev->YCbCr.Cr - 128),
                                    prev->YCbCr.Y - 0.34414 * (prev->YCbCr.Cb - 128) - 0.71414 * (prev->YCbCr.Cr - 128),
                                    prev->YCbCr.Y + 1.772 * (prev->YCbCr.Cb - 128));
                free(prev);
            }
}

//Кодирование в .bmp
void encode_bmp(const pixel ***im)
{
       
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
    pixel *image[16][16];
    for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j)
            image[i][j] = make_pixel(0, 0, 0);
    read_scan(image);
    encode_bmp(image);
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
    printf("allGood\n");
    if (!read_marker(EOI)) //Проверка конца файла
        return;
}

int main(void)
{
    read_jpeg(NULL);
}