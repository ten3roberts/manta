#include "log.h"
#include "cr_time.h"
#include "math/math.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "math/mat4.h"

#include "math/vec.h"

static void ftoa_sci(char* buffer, double value);

#if PL_LINUX
void set_print_color(int color)
{
	printf("\x1b[%dm", color);
}
#elif PL_WINDOWS
#include <windows.h>
void set_print_color(int color)
{
	static HANDLE hConsole;
	if (!hConsole)
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
}
#endif

int normalize(double* val)
{
	int exponent = 0;
	double value = *val;

	while (value >= 1.0)
	{
		value /= 10.0;
		++exponent;
	}

	while (value < 0.1)
	{
		value *= 10.0;
		--exponent;
	}
	*val = value;
	return exponent;
}

// Converts a float to scientific notation
void ftoa_sci(char* buffer, double value)
{
	int exponent = 0;
	static const int width = 4;

	// If value is 0,
	if (value == 0.0)
	{
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	if (value < 0.0)
	{
		*buffer++ = '-';
		value = -value;
	}

	exponent = normalize(&value);

	int digit = value * 10.0;
	*buffer++ = digit + '0';
	value = value * 10.0 - digit;
	--exponent;

	*buffer++ = '.';

	for (int i = 0; i < width; i++)
	{
		int digit = value * 10.0;
		*buffer++ = digit + '0';
		value = value * 10.0 - digit;
	}

	*buffer++ = 'e';
	itos(exponent, buffer, 10, 1);
}

FILE* log_file = NULL;
size_t last_log_length = 0;

// Specifies the frame number the previous log was on, to indicate if a new message is on a new frame
size_t last_log_frame = 0;
int last_color = CONSOLE_WHITE;

#ifdef DEBUG
#define WRITE(s)                                                                                                       \
	fputs(s, stdout), fputs(s, log_file);                                                                              \
	fflush(log_file);                                                                                                  \
	last_log_length += strlen(s);
#else
#define WRITE(s)                                                                                                       \
	fputs(s, stdout);                                                                                                  \
	fputs(s, log_file);                                                                                                \
	last_log_length += strlen(s);

#endif
#define FLUSH_BUFCH                                                                                                    \
	buffer[buffer_index] = '\0';                                                                                       \
	WRITE(buffer);                                                                                                     \
	buffer_index = 0;
#define WRITECH(c)                                                                                                     \
	buffer[buffer_index] = c;                                                                                          \
	buffer_index++;                                                                                                    \
	if (buffer_index == 510)                                                                                           \
	{                                                                                                                  \
		FLUSH_BUFCH                                                                                                    \
	};

int log_init()
{
	if (log_file)
		return 1;

	last_log_length = 0;
	last_log_frame = 0;

	char fname[256];
	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	create_dirs("./logs");
	strftime(fname, sizeof fname, "./logs/%F_%H.%M", timeinfo);
	strcat(fname, ".log");
	log_file = fopen(fname, "w");
	if (log_file == NULL)
		return -1;
	return 0;
}

void log_terminate()
{
	if (log_file == NULL)
		return;
	fclose(log_file);
	log_file = NULL;
}

int log_call(int color, const char* name, const char* fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	if (color == -1)
		color = last_color;
	set_print_color(color);
	last_color = color;
	// When chars in format are written they are saved to buf
	// After 512 chars or a flag is hit, buffer is flushed
	size_t buffer_index = 0;
	// Write the header
	char buffer[512];

	if (last_log_frame != time_framecount())
	{
		last_log_frame = time_framecount();
		// Write a divider with the length of last_log_length capped at 64 characters
		last_log_length = min(64, last_log_length);
		memset(buffer, '-', last_log_length);
		buffer[last_log_length] = '\n';
		buffer[last_log_length + 1] = '\0';
		WRITE(buffer);
	}

	last_log_length = 0;
	if (name)
	{
		strcpy(buffer, "[ ");
		strcat(buffer, name);
		strcat(buffer, " @ ");

		time_t rawtime;
		struct tm* timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buffer + strlen(buffer), sizeof buffer, "%H.%M.%S", timeinfo);
		strcat(buffer, " ]");
		if (color == CONSOLE_RED)
		{
			strcat(buffer, " ERROR ");
		}
		else if (color == CONSOLE_YELLOW)
		{
			strcat(buffer, " WARNING ");
		}
		strcat(buffer, ": ");
		WRITE(buffer);
	}

	char ch;
	long long int_tmp;
	char char_tmp;
	char* string_tmp;
	double double_tmp;

	// Some specifiers require a length_mod before them, e.g; vectors %v
	unsigned int length_mod = 0;
	int precision = 4;
	while ((ch = *fmt++))
	{
		if (ch == '%' || length_mod)
		{
			FLUSH_BUFCH;
			switch (ch = *fmt++)
			{
				// %% precent sign
			case '%':
				WRITECH('%');
				length_mod = 0;

				break;

				// %c character
			case 'c':
				char_tmp = va_arg(arg, int);
				WRITECH(char_tmp);
				length_mod = 0;

				break;

				// %s string
			case 's':
				string_tmp = va_arg(arg, char*);
				WRITE(string_tmp);
				length_mod = 0;

				break;

				// %o int
			case 'b':
				int_tmp = va_arg(arg, int);
				utos(int_tmp, buffer, 2, 0);
				WRITE(buffer);
				length_mod = 0;

				break;

				// %o int
			case 'o':
				int_tmp = va_arg(arg, int);
				utos(int_tmp, buffer, 8, 0);
				WRITE(buffer);
				length_mod = 0;

				break;

				// %d int
			case 'd':
				int_tmp = va_arg(arg, int);
				itos(int_tmp, buffer, 10, 0);
				WRITE(buffer);
				length_mod = 0;

				break;

				// %u int
			case 'u':
				int_tmp = va_arg(arg, int);
				utos(int_tmp, buffer, 16, 0);
				WRITE(buffer);
				length_mod = 0;

				break;

				// %x int hex
			case 'x':
				int_tmp = va_arg(arg, int);
				utos(int_tmp, buffer, 16, 0);
				WRITE(buffer);
				length_mod = 0;

				break;

				// %x int hex
			case 'X':
				int_tmp = va_arg(arg, int);
				utos(int_tmp, buffer, 16, 1);
				WRITE(buffer);
				length_mod = 0;

				break;

				// %f float
			case 'f':
				double_tmp = va_arg(arg, double);
				ftos(double_tmp, buffer, precision);
				WRITE(buffer);
				length_mod = 0;
				break;
				// %e float in scientific form
			case 'e':
				double_tmp = va_arg(arg, double);
				ftoa_sci(buffer, double_tmp);
				WRITE(buffer);
				length_mod = 0;

				break;

			case 'p':
				int_tmp = (size_t)va_arg(arg, void*);
				utos(int_tmp, buffer, 16, 0);
				WRITE("b");
				WRITE(buffer);
				length_mod = 0;

				break;

			case 'v':
				if (length_mod == 1)
				{
					ftos(va_arg(arg, double), buffer, precision);
					WRITE(buffer)
				}
				else if (length_mod == 2)
				{
					vec2_string(va_arg(arg, vec2), buffer, precision);
					WRITE(buffer)
				}
				else if (length_mod == 3)
				{
					vec3_string(va_arg(arg, vec3), buffer, precision);
					WRITE(buffer);
				}
				else if (length_mod == 4)
				{
					vec4_string(va_arg(arg, vec4), buffer, precision);
					WRITE(buffer)
				}
				length_mod = 0;
				break;
			case 'm': {
				mat4 mat = va_arg(arg, mat4);
				WRITECH('\n');
				FLUSH_BUFCH;
				mat4_string(&mat, buffer, precision);
				WRITE(buffer);
				length_mod = 0;
				break;
			}
			default:
				length_mod = atoi(--fmt);
				if (length_mod)
					continue;
				WRITECH('%');
				WRITECH(ch);
				length_mod = 0;
			}
		}
		else
		{
			WRITECH(ch);
		}
	}
	WRITECH('\n');
	FLUSH_BUFCH;
	set_print_color(CONSOLE_WHITE);
	va_end(arg);
	return last_log_length;
}