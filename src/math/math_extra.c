#include "math_extra.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// Converts a signed integer to string
int itos(signed long long num, char* buf, int base, int upper)
{
	// Return and write one character num == 0
	if (num == 0)
	{
		*buf++ = '0';
		*buf = '\0';
		return 1;
	}
	int neg = num < 0;
	num = abs(num);
	char* numerals;
	if (upper)
		numerals = "0123456789ABCDEF";
	else
		numerals = "0123456789abcdef";

	size_t buf_index = logn(base, num) + 1 + neg;
	int return_value = buf_index;
	buf[buf_index] = '\0';

	while (buf_index)
	{
		buf[--buf_index] = numerals[num % base];

		num /= base;
	}
	if (neg)
		*buf = '-';
	return return_value;
}

// Converts an unsigned integer to string
int utos(unsigned long long num, char* buf, int base, int upper)
{
	// Return and write one character num == 0
	if (num == 0)
	{
		*buf++ = '0';
		*buf = '\0';
		return 1;
	}
	char* numerals;
	if (upper)
		numerals = "0123456789ABCDEF";
	else
		numerals = "0123456789abcdef";

	size_t buf_index = logn(base, num) + 1;
	int return_value = buf_index;

	buf[buf_index] = '\0';

	while (buf_index)
	{
		buf[--buf_index] = numerals[num % base];

		num /= base;
	}
	return return_value;
}

int ftos(double num, char* buf, int precision)
{
	if (isinf(num))
	{
		strcpy(buf, "inf");
		return 3;
	}


	// Save the sign and remove it from num
	int neg = num < 0;
	if (neg)
		num *= -1;
	// Shift decimal to precision places to an int
	int32_t a = num * pow(10, precision + 1);

	if (a % 10 >= 5)
		a += 10;
	a /= 10;

	int dec_pos = precision;

	// Carried the one, need to round once more
	while (a % 10 == 0 && a && a > num)
	{
		if (a % 10 >= 5)
			a += 10;
		a /= 10;
		dec_pos--;
	}

	int base = 10;
	char numerals[17] = { "0123456789ABCDEF" };

	// Return and write one character if float == 0 to precision accuracy
	if (a == 0)
	{
		*buf++ = '0';
		*buf = '\0';
		return 1;
	}

	size_t buf_index = log10(a) + (dec_pos ? 2 : 1) + max(dec_pos - log10(a), 0) + neg;
	int return_value = buf_index;

	buf[buf_index] = '\0';
	while (buf_index)
	{
		buf[--buf_index] = numerals[a % base];
		if (dec_pos == 1)
			buf[--buf_index] = '.';
		dec_pos--;
		a /= base;
	}
	if (neg)
		*buf = '-';
	return return_value;
}
