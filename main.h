#define NUM_OF_TABLES 4 //Максимальное количество таблиц Хаффмана или квантования
#define QUANT_TABLE_SIZE 64 //Количество элементов в таблице квантования

typedef unsigned short ushort;
typedef unsigned char uchar;

const ushort SOI = 0xFFD8; //Начало изображения
const ushort EOI = 0xFFD9; //Конец изображения
const ushort SOS = 0xFFDA; //Начало скана
const ushort DQT = 0xFFDB; //Таблица квантования
const ushort DRI = 0xFFDD; //Restart interval
const ushort APP0 = 0xFFE0; //Сегменты приложений от 0xFFE0 до 0xFFEF
const ushort APP15 = 0xFFEF;
const ushort COM = 0xFFFE; //Комментарий
const ushort SOF = 0xFFC0; //Начало основного кадра
const ushort DHT = 0xFFC4; //Таблица Хаффмана
const ushort RST = 0xFFD0; //Интервал перезапуска

void read_jpeg(char *source);