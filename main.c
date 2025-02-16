#include <stdio.h>
#include <stdlib.h>
#include "binReader.h"
#include "background.h"
#include "main.h"
#include "data_unit.h"
#include "binWriter.h"

ushort *quant_tables[NUM_OF_TABLES]; //Массив таблиц квантования
huff_table *DC_tables[NUM_OF_TABLES]; //Массив таблиц Хаффмана для DC
huff_table *AC_tables[NUM_OF_TABLES]; //Массив таблиц Хаффмана для АС
ushort sample_precision; //precision of bits
ushort image_width; //Ширина
ushort image_height; //Высота
uchar max_h; //Максимальное значение горизонтального прореживания
uchar max_v; //Максимальное значения вертикального прореживания
uchar mcu_height; //Высота MCU
uchar mcu_width; //Ширина MCU
ushort num_of_comps; //Кол-во компонент
component **comps; //Массив ссылок на данные о компонентах
uchar *data_unit_by_comp;//Количество блоков для каждой компоненты
uchar sum_units;//Количество блоков в каждом MCU
ushort restart_interval = 0; //Перезапуск MCU, по умолчанию 0
ushort start_spectral = 0; //Для прогрессивного
ushort end_spectral = 63; //Для прогрессивного
bit4 ahal;

//uchar const cur_restarts_num = 19;
//uint const cur_image_width = 16 * 5 * cur_restarts_num;
//uint const cur_image_height = 16 * 10;

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
    print_huff_table(temp);
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
    image_height = get_word();
    image_width = get_word();
    num_of_comps = get_byte();
    comps = calloc(num_of_comps, sizeof(component*));
    printf("sample_pricision: %d\n", sample_precision);
    printf("image_width: %d\n", image_width);
    printf("image_height: %d\n", image_height);
    printf("num of components: %d\n", num_of_comps);
    for (int i = 0; i < num_of_comps; ++i)
    {
        ushort c = get_byte();
        bit4 hv = get_4bit();
        if (hv.first > max_h)
            max_h = hv.first;
        if (hv.second > max_v)
            max_v = hv.second;
        ushort tq = get_byte();
        comps[c - 1] = make_component(hv.first, hv.second, tq);
        printf("component %d\n", c);
        print_component(comps[c - 1]);
    }
    //Вычисление данных для декодирования MCU
    data_unit_by_comp = calloc(num_of_comps, sizeof(uchar));
    for (int i = 0; i < num_of_comps; ++i)
    {
        uchar temp = comps[i]->h * comps[i]->v;
        data_unit_by_comp[i] = temp;
        sum_units += temp;
    }
    mcu_height = ROW_COUNT * max_v;
    mcu_width = ROW_COUNT * max_h;
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
        comps[cs - 1]->dc_table_id = tdta.first;
        comps[cs - 1]->ac_table_id = tdta.second;
        printf("component %d:\tDC: %d\tAC: %d\n", cs, tdta.first, tdta.second);
    }
    start_spectral = get_byte();
    printf("start-spectral-selection: %d\n", start_spectral);
    end_spectral = get_byte();
    printf("end-spectral-selection: %d\n", end_spectral);
    ahal = get_4bit();
    printf("approximation-high: %d\tapproximation-low: %d\n\n", ahal.first, ahal.second);
}

//Проверка в диапазоне 0-255
static inline uchar clamp(int n) {
    return n < 0 ? 0 : n > 255 ? 255 : n;
}

//Декодирование одного MCU (только JFIF)
pixel **decode_mcu()
{
    /*printf("num_of_Y = %d\n", data_unit_by_comp[0]);
    printf("num_of_Cb = %d\n", data_unit_by_comp[1]);
    printf("num_of_Cr = %d\n", data_unit_by_comp[2]);*/
    pixel **im = calloc(mcu_width * mcu_height, sizeof(pixel*));
    for (int i = 0; i < mcu_width; ++i)
        for (int j = 0; j < mcu_height; ++j)
            im[i * mcu_width + j] = make_pixel(0, 0, 0);
    for (int i = 0; i < num_of_comps; ++i)
    {
        uchar x_padding = 0;
        uchar y_padding = 0;
        for (int k = 0; k < data_unit_by_comp[i]; ++k)
        {
            //printf("k: %d\n", k);
            int scaling_x = (max_v / comps[i]->v);
            int scaling_y = (max_h / comps[i]->h);
            short *unit = decode_data_unit(i, DC_tables[comps[i]->dc_table_id], AC_tables[comps[i]->ac_table_id], quant_tables[comps[i]->tq]);
            for (int x = x_padding; x < x_padding + ROW_COUNT * scaling_x; ++x)
                for (int y = y_padding; y < y_padding + ROW_COUNT * scaling_y; ++y)
                {
                    int i_mcu = x % (ROW_COUNT * scaling_x);
                    int j_mcu = y % (ROW_COUNT * scaling_y);
                    if (i == 0)
                        im[x * mcu_width + y]->YCbCr.Y = unit[(i_mcu / scaling_x) * ROW_COUNT + (j_mcu / scaling_y)];
                    else if (i == 1)
                        im[x * mcu_width + y]->YCbCr.Cb = unit[(i_mcu / scaling_x) * ROW_COUNT + (j_mcu / scaling_y)];
                    else if (i == 2)
                        im[x * mcu_width + y]->YCbCr.Cr = unit[(i_mcu / scaling_x) * ROW_COUNT + (j_mcu / scaling_y)];
                }

            free(unit);
            if (comps[i]->h > 1 && y_padding == 0 && k != 3)
            {
                y_padding += ROW_COUNT;
            }
            else if (comps[i]->v > 1 && x_padding == 0 && k != 3)
            {
                y_padding = 0;
                x_padding += ROW_COUNT;
            }
            else if (comps[i]->h == 2 && comps[i]->v == 2)
            {
                y_padding += ROW_COUNT;
            }
            else if (comps[i]->h != 1 && comps[i]->v != 1)
            {
                printf("k = %d\n", k);
                printf("decode_mcu -> Error: incorrect (h . v) values for JFIF (%d . %d)\n", comps[i]->h, comps[i]->v);
                return im;
            }
        }
    }

    //Перевод в RGB
    for (int i = 0; i < mcu_height; ++i)
    for (int j= 0; j < mcu_width; ++j)
        {
            pixel *prev = im[i * mcu_width + j];
            prev->YCbCr.Y += 128;
            prev->YCbCr.Cb += 128;
            prev->YCbCr.Cr += 128;
            im[i * mcu_width + j] = make_pixel(clamp(prev->YCbCr.Y + 1.402 * (prev->YCbCr.Cr - 128)),
                                clamp(prev->YCbCr.Y - 0.34414 * (prev->YCbCr.Cb - 128) - 0.71414 * (prev->YCbCr.Cr - 128)),
                                clamp(prev->YCbCr.Y + 1.772 * (prev->YCbCr.Cb - 128)));
            free(prev);
            /*printf("x%x%x%x ", im[i * mcu_width + j]->RGB.R, im[i * mcu_width + j]->RGB.G, im[i * mcu_width + j]->RGB.B);
            if (j == 15)
                printf("\n");*/
        }
    return im;
}

//Выполнение рестарта
uchar make_restart()
{
    ushort marker = get_word();
    bits_align();
    //printf("marker: %x\n", marker);
    if (marker >= RST0 && marker <= RST7)
    {
        data_unit_init(num_of_comps);
        return 1;
    }
    printf("marker: %x\n", marker);
    return 0;
}

//Чтение сегмента скана
void read_scan(pixel **im)
{
    ushort marker = read_tables();
    read_scan_header();
    printf("scan_header -> complete\n");
    //Example-----
    data_unit_init(num_of_comps);
    //ushort num_of_xmcu = 0; //Кол-во прочитанных mcu по x
    //ushort num_of_ymcu = 0; //Кол-во прочитанных mcu по y
    uint mcu_count = 0; //Общее кол-во прочитанных mcu
    for (int row = 0; row < (image_height + (mcu_height - 1)) / mcu_height; ++row)
    {   
        for (int col = 0; col < (image_width + (mcu_width - 1)) / mcu_width; ++col)
        {
            //printf("decode_mcu -> interval:%d  mcu%d\n", c);
            pixel **mcu = decode_mcu();
            for (int i = row * mcu_height; i < mcu_height * (row + 1); ++i)
                for (int j = col * mcu_width; j < mcu_width * (col + 1) && j < image_width; ++j)
                {
                    int mcu_i = i % mcu_width;
                    int mcu_j = j % mcu_height;
                    im[i * image_width + j] = mcu[mcu_i * mcu_width + mcu_j];
                }
                free(mcu);
                //++num_of_ymcu;
                ++mcu_count;
            //printf("next_byte: %x\n", get_next_byte());
            if (mcu_count % restart_interval == 0 && !make_restart())
            {
                printf("make_restart -> Error: wrong marker\n");
                return;
            }
            //printf("num_of_mcu: %d\n", mcu_count);
        }
        //++num_of_xmcu;
        //printf("num_of_xmcu: %d\n", num_of_xmcu);
    }
}

//Кодирование в .bmp
void encode_bmp(pixel **im)
{
    char *name = "Aqours.bmp";
    set_bin_output(name);
    uint height = image_height;//temporary
    uint width = image_width;
    uint padding_size = width % 4;
    uint size = 14 + 12 + height * width * 3 + padding_size * height;
    put_char('B');
    put_char('M');
    put_int(size);
    put_int(0);
    put_int(0x1A);
    put_int(12);
    put_short(width);
    put_short(height);
    put_short(1);
    put_short(24);

    for (int i = height - 1; i >= 0; --i)
    {
        for (int j = 0; j < width; ++j)
        {
            put_char(im[i * width + j]->RGB.B);
            put_char(im[i * width + j]->RGB.G);
            put_char(im[i * width + j]->RGB.R);
        }
        for (int j = 0; j < padding_size; ++j)
            put_char(0);
    }
    close_bin_output();
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
    pixel **image = calloc(image_height * image_width, sizeof(pixel*));
    for (int i = 0; i < image_height; ++i)
        for (int j = 0; j < image_width; ++j)
            image[i * image_width + j] = make_pixel(0, 0, 0);
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
    if (!read_marker(EOI)) //Проверка конца файла
        return;
}

int main(void)
{
    read_jpeg("Aqours.jpg");
}