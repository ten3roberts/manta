#include "math_extra.h"
#include <stdlib.h>

// Converts a signed integer to string
void itos(signed long long num, char * buf, int base, int upper)
{
	int neg = num < 0;
	num = abs(num);
	char * numerals;
	if (upper)
		numerals = "0123456789ABCDEF";
	else
		numerals = "0123456789abcdef";

	size_t buf_index = logn(base, num) + 1 + neg;
	buf[buf_index] = '\0';

	while (buf_index)
	{
		buf[--buf_index] = numerals[num % base];

		num /= base;
	}
	if (neg)
		buf[0] = '-';
}

// Converts an unsigned integer to string
void utos(unsigned long long num, char * buf, int base, int upper)
{
	char * numerals;
	if (upper)
		numerals = "0123456789ABCDEF";
	else
		numerals = "0123456789abcdef";

	size_t buf_index = logn(base, num) + 1;
	buf[buf_index] = '\0';

	while (buf_index)
	{
		buf[--buf_index] = numerals[num % base];

		num /= base;
	}
}

void ftos(double num, char * buf, int precision)
{
	// Shift decimal to ,'nul'
	int a = num * pow(10, precision+1);

	if (a % 10 >= 5)
		a += 10;
	a /= 10;

	int dec_pos = precision;

	// Carried the one, need to round once more
	while (a % 10 == 0 && a)
	{
		if (a % 10 >= 5)
			a += 10;
		a /= 10;
		dec_pos--;
	}

	int base = 10;
	char numerals[17] = {"0123456789ABCDEF"};

	size_t buf_index = logn(base, a) + (dec_pos ? 2 : 1) + (a < pow(10, dec_pos));
	buf[buf_index] = '\0';
	while (buf_index)
	{
		buf[--buf_index] = numerals[a % base];
		if (dec_pos == 1)
			buf[--buf_index] = '.';
		dec_pos--;
		a /= base;
	}
}
