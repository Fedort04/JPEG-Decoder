#define NUM_OF_TABLES 4 //Максимальное количество таблиц Хаффмана или квантования
#define QUANT_TABLE_SIZE 64 //Количество элементов в таблице квантования
#define NUM_HUFF_CODE_LEN 16 //Количество длин кодов Хаффмана
#define MAX_NUM_HUFF_SYM 162 //Максимальное количество символов в таблице Хаффмана

typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

static const ushort SOI = 0xFFD8; //Начало изображения
static const ushort EOI = 0xFFD9; //Конец изображения
static const ushort SOS = 0xFFDA; //Начало скана
static const ushort DQT = 0xFFDB; //Таблица квантования
static const ushort DRI = 0xFFDD; //Restart interval
static const ushort APP0 = 0xFFE0; //Сегменты приложений от 0xFFE0 до 0xFFEF
static const ushort APP15 = 0xFFEF;
static const ushort COM = 0xFFFE; //Комментарий
static const ushort SOF = 0xFFC0; //Начало основного кадра
static const ushort DHT = 0xFFC4; //Таблица Хаффмана
static const ushort RST0 = 0xFFD0; //Интервал перезапуска от 0xFFD0 до 0xFFD7
static const ushort RST7 = 0xFFD7;

void read_jpeg(char *source);