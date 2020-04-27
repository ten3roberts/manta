#include "math/math.h"
#include "magpie.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Converts a signed integer to string
int itos(int num, char* buf, int base, int upper)
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
int utos(unsigned int num, char* buf, int base, int upper)
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
		if (num < 0)
			*buf++ = '-';
		*buf++ = 'i';
		*buf++ = 'n';
		*buf++ = 'f';
		*buf++ = '\0';
		return num < 0 ? 4 : 3;
	}

	// Save the sign and remove it from num
	int neg = num < 0;
	if (neg)
		num *= -1;
	// Shift decimal to precision places to an int
	uint64_t a = num * pow(10, precision + 1);

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
	char numerals[17] = {"0123456789ABCDEF"};

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

int ftos_fixed(double num, char* buf, int length)
{
	if (isinf(num))
	{
		memset(buf, '0', length);
		if (num < 0)
			*buf++ = '-';

		*buf++ = 'i';
		*buf++ = 'n';
		*buf++ = 'f';
		*buf++ = '\0';
		return length;
	}

	if (fabs(num) < pow(10, -length))
	{
		memset(buf, ' ', length);
		buf[length - 1] = '0';
		buf[length] = 0;
		return length;
	}

	// Save the sign and remove it from num
	int neg = num < 0;
	if (neg)
		num *= -1;
	// Shift decimal to precision places to an int
	uint64_t a = num * pow(10, length - (int64_t)log10(num) - 1 - neg);
	if (a % 10 >= 5)
		a += 10;
	a /= 10;

	uint64_t dec_pos = length - max((int64_t)log10(num), 0) - 1 - neg;

	char numerals[17] = {"0123456789ABCDEF"};

	// Return and write one character if float == 0 to precision accuracy
	if (a == 0)
	{
		*buf++ = '0';
		*buf = '\0';
		return 1;
	}

	int return_value = length;

	buf[length] = '\0';
	while (length > 0)
	{
		if (dec_pos == 1)
		{
			buf[--length] = '.';
		}
		buf[--length] = numerals[a % 10];
		dec_pos--;
		a /= 10;
	}

	if (neg)
		*buf = '-';
	return return_value;
}

int ftos_pad(double num, char* buf, int precision, int pad_length, char pad_char)
{
	if (isinf(num))
	{
		if (num < 0)
			*buf++ = '-';
		*buf++ = 'i';
		*buf++ = 'n';
		*buf++ = 'f';
		*buf++ = '\0';
		return num < 0 ? 4 : 3;
	}

	// Save the sign and remove it from num
	int neg = num < 0;
	if (neg)
		num *= -1;
	// Shift decimal to precision places to an int
	uint64_t a = num * pow(10, precision + 1);

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
	char numerals[17] = {"0123456789ABCDEF"};

	// Return and write one character if float == 0 to precision accuracy
	if (a == 0)
	{
		memset(buf, pad_char, pad_length);
		buf[pad_length-1] = '0';
		buf[pad_length] = '\0';
		return pad_length;
	}
	size_t buf_index = log10(a) + (dec_pos ? 2 : 1) + max(dec_pos - log10(a), 0);
	int pad = max(pad_length - (int)buf_index - neg, 0);
	int return_value = buf_index + neg + pad;
	buf += buf_index + neg + pad;
	*buf-- = '\0';
	while (buf_index)
	{
		*buf-- = numerals[a % base];
		buf_index--;
		if (dec_pos == 1)
		{
			*buf-- = '.';
			buf_index--;
		}
		dec_pos--;
		a /= base;
	}
	if (neg)
		*buf-- = '-';
	for(int i = 0; i < pad; i++)
	{
		*buf-- = pad_char;
	}
	return return_value;
}