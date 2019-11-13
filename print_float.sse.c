#include "print_float.h"

void print_float(char* buf, float value)
{
    int digits;
    int i;
    int k;

    digits = 1;
    while (value >= 10) value /= 10, digits++;

    Assert(digits <= FLOAT_LEN - 2);

    i = 0;
    for (k = 0; k < FLOAT_LEN - 2; k++)
    {
        buf[i] = '0';
        while (value >= 1 && buf[i] < '9')
        {
            buf[i]++;
            value -= 1;
        }
        i++;
        digits--;
        if (digits == 0)
        {
            buf[i] = '.';
            i++;
        }
        value *= 10;
    }
    Assert(i == FLOAT_LEN - 1);
    buf[i] = 0;
}
