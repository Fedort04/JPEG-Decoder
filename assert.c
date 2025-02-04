#include <stdio.h>

void ASSERT(int first, int second)
{
    if (first == second)
        printf("OK\n");
    else 
        printf("FAIL\t%d\t%d\n", first, second);
}