#include "utils.h"
#include "math/math.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "magpie.h"
#include "math/vec.h"
#include "math/mat4.h"
#include "math/math.h"

#if PL_LINUX
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#elif PL_WINDOWS
#include <Windows.h>
#include <shlobj_core.h>
#include <stdio.h>
#include <sys/types.h>
#endif

#if PL_LINUX
int is_regular_file(const char* path)
{
	struct stat path_stat;
	stat(path, &path_stat);
	return S_ISREG(path_stat.st_mode);
}

int is_dir(const char* path)
{
	struct stat path_stat;
	stat(path, &path_stat);
	return S_ISDIR(path_stat.st_mode);
}

size_t listdir(const char* dir, char** result, size_t size, size_t depth)
{
	DIR* dp = opendir(dir);

	struct dirent* ep;
	if (dp != NULL)
	{
		while ((ep = readdir(dp)))
		{
			char full_path[PATH_MAX];
			strcpy(full_path, dir);
			if (dir[strlen(dir) - 1] != '/')
				strcat(full_path, "/");
			strcat(full_path, ep->d_name);

			if (!strcmp(ep->d_name, "..") || !strcmp(ep->d_name, "."))
				continue;

			if (is_dir(full_path))
			{
				if (depth == 1)
					continue;
				size_t tmp = listdir(full_path, result, size, depth - 1);
				result += size - tmp;
				size = tmp;
			}
			else if (result && size)
			{
				strcpy(*result, full_path);
				result++;
				size--;
			}
			else
			{
				return 0;
			}
		}
		(void)closedir(dp);
	}
	else
	{
		printf("Unable to open directory %s\n", dir);
		return 0;
	}
	return size;
}

int find_file(const char* dir, char* result, size_t size, const char* filename)
{
	DIR* dp = opendir(dir);

	struct dirent* ep;
	if (dp != NULL)
	{
		while ((ep = readdir(dp)))
		{
			char full_path[PATH_MAX];
			strcpy(full_path, dir);
			if (dir[strlen(dir) - 1] != '/')
				strcat(full_path, "/");
			strcat(full_path, ep->d_name);

			if (!strcmp(ep->d_name, "..") || !strcmp(ep->d_name, "."))
				continue;

			if (is_dir(full_path))
			{
				// File was found in subdir
				if (find_file(full_path, result, size, filename) == EXIT_SUCCESS)
					return EXIT_SUCCESS;
			}
			else if (strcmp(ep->d_name, filename) == 0)
			{
				strncpy(result, full_path, size);
				return EXIT_SUCCESS;
			}
		}
		(void)closedir(dp);
	}
	else
	{
		printf("Unable to open directory %s\n", dir);
		return EXIT_FAILURE;
	}
	return EXIT_FAILURE;
}
void create_dirs(const char* path)
{
	char buf[2048];
	size_t len = strlen(path);
	for (size_t i = 0; i < len; i++)
	{
		if (path[i] == '/' || path[i] == '\\' || i == len - 1)
		{
			strncpy(buf, path, i + 1);
			buf[i + 1] = '\0';
			if (!strcmp(buf, "./") || !strcmp(buf, "../") || !strcmp(buf, "") || !strcmp(buf + 1, ":") || !strcmp(buf, ".\\") || !strcmp(buf, "..\\"))
				continue;
			mkdir(buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		}
	}
}
#elif PL_WINDOWS
int is_regular_file(const char* path)
{
	DWORD dwAttrib = GetFileAttributesA(path);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int is_dir(const char* path)
{
	DWORD dwAttrib = GetFileAttributesA(path);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

size_t listdir(const char* dir, char** result, size_t size, size_t depth)
{
	char pattern[2048];
	snprintf(pattern, 2048, "%s\\*", dir);

	WIN32_FIND_DATA data;
	HANDLE hFind;

	char fname[2048];
	if ((hFind = FindFirstFileA(pattern, &data)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			sprintf(fname, "%ws", data.cFileName);
			if (!strcmp(fname, ".") || !strcmp(fname, ".."))
				continue;

			char full_path[2048];
			strcpy(full_path, dir);
			if (dir[strlen(dir) - 1] != '/')
				strcat(full_path, "/");
			strcat(full_path, fname);

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (depth == 1)
					continue;
				size_t tmp = listdir(full_path, result, size, depth - 1);
				result += size - tmp;
				size = tmp;
			}
			else if (result && size)
			{
				posix_path(full_path, full_path, strlen(full_path));
				strcpy(*result, full_path);
				result++;
				size--;
			}
			else
			{
				return 0;
			}
		} while (FindNextFile(hFind, &data) != 0);
		FindClose(hFind);
	}
	return size;
}
int find_file(const char* dir, char* result, size_t size, const char* filename)
{
	char pattern[2048];
	snprintf(pattern, 2048, "%s\\*", dir);

	WIN32_FIND_DATA data;
	HANDLE hFind;

	char fname[2048];
	if ((hFind = FindFirstFileA(pattern, &data)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			sprintf(fname, "%ws", data.cFileName);
			if (!strcmp(fname, ".") || !strcmp(fname, ".."))
				continue;

			char full_path[2048];
			strcpy(full_path, dir);
			if (dir[strlen(dir) - 1] != '/')
				strcat(full_path, "/");
			strcat(full_path, fname);

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (find_file(full_path, result, size, filename) == EXIT_SUCCESS)
					return EXIT_SUCCESS;
			}
			else if (strcmp(fname, filename) == 0)
			{
				posix_path(full_path, full_path, strlen(full_path));
				strncpy(result, full_path, size);
				return EXIT_SUCCESS;
			}
			else
			{
			}
		} while (FindNextFile(hFind, &data) != 0);
		FindClose(hFind);
	}
	return EXIT_FAILURE;
}
void create_dirs(const char* path)
{
	char buf[2048];
	size_t len = strlen(path);
	for (size_t i = 0; i < len; i++)
	{
		if (path[i] == '/' || path[i] == '\\' || i == len - 1)
		{
			strncpy(buf, path, i + 1);
			buf[i + 1] = '\0';
			if (!strcmp(buf, "./") || !strcmp(buf, "../") || !strcmp(buf, "") || !strcmp(buf + 1, ":") || !strcmp(buf, ".\\") || !strcmp(buf, "..\\"))
				continue;
			CreateDirectoryA(buf, NULL);
		}
	}
}
#endif

void get_filename(const char* path, char* result, size_t size)
{
	const char* path_end = strrchr(path, '/');
	if (path_end == NULL)
		path_end = strchr(path, '\\');

	// No path delimiter before
	path_end = path_end ? path_end : path;

	// Extension is either . or \0
	const char* extension = strrchr(path, '.');
	if (extension == NULL)
		extension = path + strlen(path);

	size_t len = extension - path_end;
	if (len > size)
	{
		len = size;
	}
	memcpy(result, path_end + 1, len - 1);
	result[len - 1] = '\0';
}

void get_dir(const char* path, char* result, size_t size)
{
	for (size_t i = strlen(path); i != 0; i--)
	{
		// If it is at a path delimeter
		if (path[i] == '\\' || path[i] == '/')
		{
			memmove(result, path, min(i + 1, size));
			result[min(i + 1, size)] = '\0';
			break;
		}
	}
}

size_t read_file(const char* path, char* buf)
{
	FILE* file = fopen(path, "r");
	if (!file)
		return 0;
	size_t size;
	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	if (!buf)
		return size + 1;
	fseek(file, 0L, SEEK_SET);
	fread(buf, 1, size, file);
	buf[size] = '\0';
	fclose(file);
	return size + 1;
}

char* read_file_alloc(const char* path)
{
	char* buf = NULL;
	FILE* file = fopen(path, "r");
	if (!file)
		return NULL;
	size_t size;
	fseek(file, 0L, SEEK_END);
	size = ftell(file);

	// Allocate enough memory for file contents and null terminator
	buf = malloc(size + 1);

	fseek(file, 0L, SEEK_SET);
	fread(buf, 1, size, file);
	buf[size] = '\0';
	fclose(file);
	return buf;
}

size_t read_fileb(const char* path, char* buf)
{
	FILE* file = fopen(path, "rb");
	if (!file)
		return 0;
	size_t size;
	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	if (!buf)
		return size;
	fseek(file, 0L, SEEK_SET);
	fread(buf, 1, size, file);
	fclose(file);
	return size;
}

char* read_fileb_alloc(const char* path)
{
	char* buf = NULL;
	FILE* file = fopen(path, "rb");
	if (!file)
		return NULL;
	size_t size;
	fseek(file, 0L, SEEK_END);
	size = ftell(file);

	// Allocate enough memory for file contents and null terminator
	buf = malloc(size + 1);

	fseek(file, 0L, SEEK_SET);
	fread(buf, 1, size, file);
	buf[size] = '\0';
	fclose(file);
	return buf;
}

#if PL_LINUX
void set_workingdir(const char* dir)
{
	if (chdir(dir))
	{
		printf("Invalid working directory %s\n", dir);
	}
}
#elif PL_WINDOWS
void set_workingdir(const char* dir)
{
	_chdir(dir);
}
#endif

void dir_up(const char* path, char* result, size_t size, size_t steps)
{

	size_t len = strlen(path) - 1;
	for (size_t i = len; i != 0; i--)
	{
		if (steps == 0 || i == 1)
		{
			memmove(result, path, min(i + 1, size));
			result[min(i + 1, size)] = '\0';
			break;
		}

		// If it is at a path delimeter
		if ((path[i] == '\\' || path[i] == '/') && i != len)
		{
			steps--;
		}
	}
	if (steps <= 1)
		return;
	if (strcmp(result, "./") == 0)
	{
		strcpy(result, "../");
		return;
	}
	int contains_dir = 0;
	for (char* p = result; *p != '\0'; p++)
	{
		if (*p != '/' && *p != '\\' && *p != '.')
			contains_dir = 1;
	}
	if (!contains_dir)
	{
		strncat(result, "../", size);
	}
}

void posix_path(const char* path, char* result, size_t size)
{
	if (path != result)
	{
		memmove(result, path, min(strlen(path), size));
		result[min(strlen(path), size)] = '\0';
	}

	for (char* p = result; *p != '\0'; p++)
	{
		if (*p == '\\')
			*p = '/';
	}
}

void replace_string(const char* src, char* result, size_t size, char find, char replace)
{
	if (src != result)
	{
		memmove(result, src, min(strlen(src), size));
		result[min(strlen(src), size)] = '\0';
	}

	for (char* p = result; *p != '\0'; p++)
	{
		if (*p == find)
			*p = replace;
	}
}

char* stringdup_internal(const char* s)
{
	size_t slen = strlen(s);
	char* result = malloc(slen + 1);
	if (result == NULL)
	{
		return NULL;
	}

	memcpy(result, s, slen + 1);
	return result;
}

int strcmp_s(const char* str1, const char* str2)
{
	// Either both are NULL or the same location string
	if (str1 == str2)
		return 0;
	if (str1 == NULL && str2 != NULL)
		return 1;
	if (str1 != NULL && str2 == NULL)
		return 1;

	// C stdlib strcmp
	return strcmp(str1, str2);
}

size_t string_format(char* str, size_t size, const char* fmt, ...)
{

	va_list args;
	va_start(args, fmt);
	va_end(args);
	return string_vformat(str, size, fmt, args);
}

// Writes a string into another string
// Returns written characters even if size is too small
// Does not NULL terminate
size_t string_vformat_write(char* buf, int64_t size, const char* str, size_t len)
{
	if (size > 0 && buf)
	{
		memcpy(buf, str, min(len, size));
	}
	return len;
}

//#define string_vformat_writechar(char* buf, size_t )

#define WRITE(s)                                                             \
	{                                                                        \
		size_t written = string_vformat_write(str, remaining, s, strlen(s)); \
		if (remaining > 0 && str)                                            \
			str += min(written, remaining);                                  \
		remaining -= written;                                                \
	}

#define WRITECH(c)            \
	if (remaining > 0 && str) \
	{                         \
		*str++ = c;           \
	}                         \
	--remaining;

size_t string_vformat(char* str, size_t size, const char* fmt, va_list args)
{
	char* str_start = str;
	int64_t remaining = size;

	char ch;
	long long int_tmp;
	char* string_tmp;
	double double_tmp;

	char buf_tmp[1024];

	// Some specifiers require a length_mod before them, e.g; vectors %v
	unsigned int length_mod = 0;
	int precision = 4;

	// Iterate fmt string
	while ((ch = *fmt++) != '\0')
	{
		// Format specifier
		if (ch == '%' || length_mod)
		{
			// Look at next character
			switch (ch = *fmt++)
			{
				// %% percent sign
			case '%':
				WRITECH('%');
				length_mod = 0;
				break;

				// %c character
			case 'c':
				WRITECH(va_arg(args, int));
				length_mod = 0;

				break;

				// %s string
			case 's':
				string_tmp = va_arg(args, char*);
				WRITE(string_tmp);
				length_mod = 0;

				break;

				// %o int
			case 'b':
				int_tmp = va_arg(args, int);
				utos(int_tmp, buf_tmp, 2, 0);
				WRITE(buf_tmp);
				length_mod = 0;

				break;

				// %o int
			case 'o':
				int_tmp = va_arg(args, int);
				utos(int_tmp, buf_tmp, 8, 0);
				WRITE(buf_tmp);
				length_mod = 0;

				break;

				// %d int
			case 'd':
				int_tmp = va_arg(args, int);
				itos(int_tmp, buf_tmp, 10, 0);
				WRITE(buf_tmp);
				length_mod = 0;

				break;

				// %u int
			case 'u':
				int_tmp = va_arg(args, int);
				utos(int_tmp, buf_tmp, 16, 0);
				WRITE(buf_tmp);
				length_mod = 0;

				break;

				// %x int hex
			case 'x':
				int_tmp = va_arg(args, int);
				utos(int_tmp, buf_tmp, 16, 0);
				WRITE(buf_tmp);
				length_mod = 0;

				break;

				// %x int hex
			case 'X':
				int_tmp = va_arg(args, int);
				utos(int_tmp, buf_tmp, 16, 1);
				WRITE(buf_tmp);
				length_mod = 0;

				break;

				// %f float
			case 'f':
				double_tmp = va_arg(args, double);
				ftos(double_tmp, buf_tmp, precision);
				WRITE(buf_tmp);
				length_mod = 0;
				break;
				// %e float in scientific form
			case 'e':
				double_tmp = va_arg(args, double);
				ftos_sci(double_tmp, buf_tmp, precision);
				WRITE(buf_tmp);
				length_mod = 0;

				break;
			case 'g':
				double_tmp = va_arg(args, double);
				ftos_mixed(double_tmp, buf_tmp, precision);
				WRITE(buf_tmp);
				length_mod = 0;

				break;

			case 'p':
				int_tmp = (size_t)va_arg(args, void*);
				utos(int_tmp, buf_tmp, 16, 0);
				WRITECH('b');
				WRITE(buf_tmp);
				length_mod = 0;

				break;

			case 'v':
				if (length_mod == 1)
				{
					ftos(va_arg(args, double), buf_tmp, precision);
				}
				else if (length_mod == 2)
				{
					vec2_string(va_arg(args, vec2), buf_tmp, precision);
				}
				else if (length_mod == 3)
				{
					vec3_string(va_arg(args, vec3), buf_tmp, precision);
				}
				else if (length_mod == 4)
				{
					vec4_string(va_arg(args, vec4), buf_tmp, precision);
				}
				else
				{
					WRITE("[not a valid vector length]");
					break;
					length_mod = 0;
				}
				WRITE(buf_tmp);
				length_mod = 0;
				break;
			case 'm': {
				mat4 mat = va_arg(args, mat4);
				WRITECH('\n');
				mat4_string(&mat, buf_tmp, precision);
				WRITE(buf_tmp);
				length_mod = 0;
				break;
			}

			// Not a modifier
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

	// Not enough room, step back str ptr and write NULL pointer
	if (remaining <= 0)
		str--;
	// NULL terminate
	if (str)
		*str = '\0';

	return size - remaining;
}