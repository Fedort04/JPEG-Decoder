#include <stdio.h>
#include <stdlib.h>
#include "background.h"

#define TABLE_SIZE 64
#define ROW_COUNT 8
#define COL_COUNT 8

//Печать таблицы
void print_table(ushort *table)
{
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        if (i % COL_COUNT == 0 && i != 0)
            printf("\n");
        printf("%d\t", table[i]);
    }
    printf("\n\n");
}

//Вывод информации компоненты
void print_component(component *comp)
{
    printf("h: %d, v: %d\n", comp->h, comp->v);
    printf("quant-table id: %d\n\n", comp->tq);
}

//Конструктор структуры компоненты
component *make_component(ushort h, ushort v, ushort tq) 
{
    component *res = calloc(3, sizeof(ushort));
    res->h = h;
    res->v = v;
    res->tq = tq;
    return res;
}