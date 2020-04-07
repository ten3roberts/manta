// LICENSE
//	See the end of the file for license

#ifndef LIBJSON_H
#define LIBJSON_H
#include <stddef.h>

typedef struct JSON JSON;

#define JSON_COMPACT 0
#define JSON_FORMAT	 1

#define JSON_TINVALID 0
#define JSON_TOBJECT  1
#define JSON_TARRAY	  2
#define JSON_TSTRING  4
#define JSON_TNUMBER  8
#define JSON_TBOOL	  16
#define JSON_TNULL	  32

// Creates an empty json with invalid type
JSON* json_create_empty();
// Creates a valid json null
JSON* json_create_null();

// Creates a json string
// string is copied internally and can be freed afterwards
JSON* json_create_string(const char* str);

// Creates a json number
JSON* json_create_number(double value);

// Creates an empty json object with no members
JSON* json_create_object();

// Creates an empty json array with no elements
JSON* json_create_array();

// Will remove a json object's type and JSON_FREE all previous values and members
// Effectively resets an object while keeping it's parent structure intact
void json_set_invalid(JSON* object);
// Sets the value of the json object to a string
// Previous value is freed
void json_set_string(JSON* object, const char* str);

// Sets the value of the json object to a number
// Previous value is freed
void json_set_number(JSON* object, double num);

// Sets the value of the json object to a bool
// Previous value is freed
void json_set_bool(JSON* object, int val);

// Sets the value of the json object to null
// Previous value is freed
void json_set_null(JSON* object);

// Returns the name/key of the object, NULL if an element in array
const char* json_get_name(JSON* object);

// Short hand for getting the type
#define json_type(object) json_get_type(object)

// Returns the type of the json object
int json_get_type(JSON* object);

// Returns a pointer to the internal string
// Can be modified
// Validity of pointer is not guaranteed after json_set_string or similar call
// Returns NULL if it's not a string type
char* json_get_string(JSON* object);

// Returns the number value
// Returns 0 if it's not a number type
double json_get_number(JSON* object);

// Returns the bool value
// Returns 0 if it's not a bool type
int json_get_bool(JSON* object);

// Gets a member of an object and returns its string value
char* json_get_member_string(JSON* object, const char* name);

// Gets a member of an object and returns its number value
double json_get_member_number(JSON* object, const char* name);

// Gets a member of an object and returns its bool value
int json_get_member_bool(JSON* object, const char* name);

// Returns a linked list of the members of a json object
JSON* json_get_members(JSON* object);

// Returns the member with the specified name in a json object
JSON* json_get_member(JSON* object, const char* name);

// Returns a linked list of the elements of a json array
JSON* json_get_elements(JSON* object);

#define json_next(elem) json_get_next(elem)
// Returns the next item in the list element is a part of
JSON* json_get_next(JSON* element);

// Allocates and returns a json structure as a string
// Returned string needs to be manually freed
// If format is 0, resulting string will not contain whitespace
// If format is 1, resulting string will be pretty formatted
char* json_tostring(JSON* object, int format);

// Writes the json structure to a file
// Not that the name of the root object, if not NULL, is the file that was read
// Returns 0 on success
// Overwrites file
// Creates the directories leading up to it (JSON_USE_POSIX or JSON_USE_WINAPI need to be defined accordingly)
// If format is JSON_COMPACT (1), resulting string will not contain whitespace
// If format is JSON_FORMAT (0), resulting string will be pretty formatted
int json_writefile(JSON* object, const char* filepath, int format);

// Loads a json file recusively from a file into memory
JSON* json_loadfile(const char* filepath);

// Loads a json string recursively
JSON* json_loadstring(char* str);

// Loads a json object from a string
// Returns a pointer to the end of the object in the beginning string
// NOTE : should not be used on an existing object, object needs to be empty or destroyed
char* json_load(JSON* object, char* str);

// Removes and returns a member from the json structure
// Returns NULL if member was not found
JSON* json_remove_member(JSON* object, const char* name);

// Removes and returns an element from the json structure
// Returns NULL if element was not found
// Note: This will not free the object returned
JSON* json_remove_element(JSON* object, int pos);

// Insert a member to a json object with name
// If an object of that name already exists, it is overwritten
void json_add_member(JSON* object, const char* name, JSON* value);

// Insert an element into arbitrary position in a json array
// If index is greater than the length of the array, element will be inserted at the end
// if element is a linked list, the whole list will be inserted in order
void json_insert_element(JSON* object, int pos, JSON* element);

// Insert an element to the beginning of a json array
void json_add_element(JSON* object, JSON* element);

// Recursively destroys and frees an object
// Will NOT destroy subsequent objects in its array
// Element should be remove from parent before calling destroy
void json_destroy(JSON* object);

// End of header
// Implementation
#ifdef JSON_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#ifndef JSON_MALLOC
#define JSON_MALLOC(s) malloc(s)
#endif
#ifndef JSON_FREE
#define JSON_FREE(p) free(p)
#endif
#ifndef JSON_REALLOC
#define JSON_REALLOC(p, s) realloc(p, s)
#endif

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/stat.h>
#endif
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#endif

#define JSON_IS_WHITESPACE(c) (c == ' ' || c == '\n' || c == '\r' || c == '\t')

#ifndef JSON_MESSAGE
#define JSON_MESSAGE(m) puts(m)
#endif

// Returns a copy of str
char* strduplicate(const char* str)
{
	const size_t lstr = strlen(str);
	char* dup = JSON_MALLOC(lstr + 1);
	memcpy(dup, str, lstr);
	dup[lstr] = '\0';
	return dup;
}

struct JSONStringStream
{
	// The internal string pointer
	char* str;
	// The allocated size of the string
	size_t size;
	// How much has been written to the string
	// Does not include the null terminator
	size_t length;
};

#define WRITE_ESCAPE(c)            \
	escaped_str[escapedi] = '\\';  \
	escaped_str[escapedi + 1] = c; \
	escapedi++;

// Writes to string stream
// If escape is 1, control characters will be escaped as two characters
// Escaping decreases performance
void json_ss_write(struct JSONStringStream* ss, const char* str, int escape)
{
	if (str == NULL)
	{
		JSON_MESSAGE("Error writing invalid string");
	}

	size_t lstr = strlen(str);
	char* escaped_str = NULL;
	if (escape)
	{
		// Worst case, expect all characters to be escaped and fit \0
		escaped_str = JSON_MALLOC(2 * lstr + 1);

		// Allocated size
		// Look for control characters
		size_t stri = 0, escapedi = 0;
		for (; stri < lstr; stri++, escapedi++)
		{
			switch (str[stri])
			{
			case '"':
				WRITE_ESCAPE('"');
				break;
			case '\b':
				WRITE_ESCAPE('b');
				break;
			case '\f':
				WRITE_ESCAPE('f');
				break;
			case '\n':
				WRITE_ESCAPE('n');
				break;
			case '\r':
				WRITE_ESCAPE('r');
				break;
			case '\t':
				WRITE_ESCAPE('t');
				break;
			default:
				escaped_str[escapedi] = str[stri];
				break;
			}
		}
		lstr = escapedi;
		escaped_str[lstr] = '\0';
	}

	// Allocate first time
	if (ss->str == NULL)
	{
		ss->size = 8;
		ss->str = JSON_MALLOC(8);
	}

	// Resize
	if (ss->length + lstr + 1 > ss->size)
	{

		ss->size = ss->size << (size_t)log2(lstr + 1);
		char* tmp = JSON_REALLOC(ss->str, ss->size);
		if (tmp == NULL)
		{
			JSON_MESSAGE("Failed to allocate memory for string stream");
			return;
		}
		ss->str = tmp;
	}

	// Copy data
	memcpy(ss->str + ss->length, escape ? escaped_str : str, lstr);
	ss->length += lstr;
	// Null terminate
	ss->str[ss->length] = '\0';
	if (escape)
	{
		JSON_FREE(escaped_str);
	}
}

float json_max(float a, float b)
{
	return (a > b ? a : b);
}

// Converts a double to a string
// Precision indicates the max digits to include after the comma
// Prints up to precision digits after the comma, can write less. Can be used to print integers, where the comma is not
// written Returns how many characters were written
int json_ftos(double num, char* buf, int precision)
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
	size_t a = num * pow(10, precision + 1);

	// Round initially
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

	size_t digitcount = log10(a) + 1;
	size_t buf_index = digitcount + neg + (dec_pos >= digitcount ? dec_pos - digitcount + 1 : 0) + (dec_pos ? 1 : 0);
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

// Convert a json valid number representation from string to double
char* json_stof(char* str, double* out)
{
	double result = 0;
	// Signifies if read past period
	int isfraction = 0;
	// The value of the digit after period
	double digitval = 1;
	// Signifies if optional exponent is being read
	int isexponent = 0;
	// Value of exponent
	int exponent = 0;
	// The sign of the resulting value
	int sign = 1;
	if (*str == '-')
	{
		sign = -1;
		str++;
	}

	for (; *str != '\0'; str++)
	{
		// Decimal place
		if (*str == '.')
		{
			isfraction = 1;
			continue;
		}

		// Exponent
		if (*str == 'e' || *str == 'E')
		{
			if (isexponent)
			{
				JSON_MESSAGE("Can't have stacked exponents");
				return 0;
			}
			isexponent = 1;
			continue;
		}
		// End
		// Keep comma and return
		if (*str == ',')
		{
			break;
		}

		// Break when not a valid number
		// Triggered when next char is end of object or array wihtout comma
		if (*str < '0' || *str > '9')
		{
			break;
		}

		int digit = *str - '0';
		if (isexponent)
		{
			exponent *= 10;
			exponent += digit;
		}
		else if (isfraction)
		{
			digitval *= 0.1;
			result += digit * digitval;
		}
		// Normal digit
		else
		{
			result *= 10;
			result += digit;
		}
	}
	result *= sign;
	if (exponent != 0)
		*out = result * powf(10, exponent);
	else
		*out = result;
	return str;
}

// Reads from start quote to end quote and takes escape characters into consideration
// Allocates memory for output string; need to be freed manually
char* json_read_quote(char* str, char** out)
{
	// Skip past start quote
	while (*str != '"')
		str++;
	str++;

	// Iterator for the object stringval
	size_t lval = 8;
	*out = JSON_MALLOC(lval);
	char* result = *out;
	size_t valit = 0;
	// Loop to end of quote
	for (; *str != '\0'; str++)
	{
		char c = *str;

		// Allocate more space for string
		if (valit + 1 >= lval)
		{
			lval *= 2;
			char* tmp = JSON_REALLOC(*out, lval);
			if (tmp == NULL)
			{
				JSON_MESSAGE("Failed to allocate memory for string value");
				return str;
			}
			*out = tmp;
			result = *out;
		}

		// Escape sequence
		if (c == '\\')
		{
			switch (str[1])
			{
			case '"':
				result[valit++] = '"';
				break;
			case '\\':
				result[valit++] = '\\';
				break;
			case '/':
				result[valit++] = '/';
				break;
			case 'b':
				result[valit++] = '\b';
				break;
			case 'f':
				result[valit++] = '\f';
				break;
			case 'n':
				result[valit++] = '\n';
				break;
			case 'r':
				result[valit++] = '\r';
				break;
			case 't':
				result[valit++] = '\t';
				break;
			default:
				JSON_MESSAGE("Invalid escape sequence");
				break;
			}
			str++;
			continue;
		}

		if (c == '\\' || c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t')
		{
			char msg[512];
			snprintf(msg, sizeof msg, "Invalid character in string %10s, control characters must be escaped", str);
			JSON_MESSAGE(msg);
			return NULL;
		}

		// End quote
		if (c == '"')
		{
			result[valit++] = '\0';
			return str + 1;
		}

		// Normal character
		result[valit++] = c;
	}
	JSON_MESSAGE("Unexpected end of string");
	return str;
}

struct JSON
{
	int type;

	char* name;
	char* stringval;
	double numval;
	struct JSON* members;
	int count;
	// Linked list to the other members
	// First elements are more recent
	struct JSON* next;
};

// Constructors
JSON* json_create_empty()
{
	JSON* object = calloc(1, sizeof(JSON));
	object->type = JSON_TINVALID;
	return object;
}
JSON* json_create_null()
{
	JSON* object = calloc(1, sizeof(JSON));
	object->type = JSON_TNULL;
	return object;
}

JSON* json_create_string(const char* str)
{
	JSON* object = calloc(1, sizeof(JSON));
	object->type = JSON_TSTRING;
	object->stringval = strduplicate(str);
	return object;
}

JSON* json_create_number(double value)
{
	JSON* object = calloc(1, sizeof(JSON));
	object->type = JSON_TNUMBER;
	object->numval = value;
	return object;
}

JSON* json_create_object()
{
	JSON* object = calloc(1, sizeof(JSON));
	object->type = JSON_TOBJECT;
	return object;
}

JSON* json_create_array()
{
	JSON* object = calloc(1, sizeof(JSON));
	object->type = JSON_TARRAY;
	return object;
}

void json_set_invalid(JSON* object)
{
	// If previous type was object or array, destroy members
	if (object->members)
	{
		JSON* cur = object->members;
		while (cur)
		{
			JSON* next = cur->next;
			json_destroy(cur);
			cur = next;
		}
	}
	object->count = 0;
	object->members = NULL;
	if (object->stringval)
	{
		JSON_FREE(object->stringval);
	}
	object->type = JSON_TINVALID;
	object->numval = 0;
}

// Setters
void json_set_string(JSON* object, const char* str)
{
	json_set_invalid(object);
	object->type = JSON_TSTRING;
	object->stringval = strduplicate(str);
}

void json_set_number(JSON* object, double num)
{
	json_set_invalid(object);
	object->type = JSON_TNUMBER;
	object->numval = num;
}

void json_set_bool(JSON* object, int val)
{
	json_set_invalid(object);
	object->type = JSON_TBOOL;
	object->numval = val;
}

void json_set_null(JSON* object)
{
	json_set_invalid(object);
	object->type = JSON_TNULL;
}

const char* json_get_name(JSON* object)
{
	return object->name;
}

int json_get_type(JSON* object)
{
	return object->type;
}

char* json_get_string(JSON* object)
{
	return object->stringval;
}

double json_get_number(JSON* object)
{
	return object->numval;
}

int json_get_bool(JSON* object)
{
	return object->numval;
}

char* json_get_member_string(JSON* object, const char* name)
{
	JSON* tmp = json_get_member(object, name);
	if (tmp == NULL)
		return NULL;
	return tmp->stringval;
}

// Gets a member of an object and returns its number value
double json_get_member_number(JSON* object, const char* name)
{
	JSON* tmp = json_get_member(object, name);
	if (tmp == NULL)
		return 0;
	return tmp->numval;
}

// Gets a member of an object and returns its bool value
int json_get_member_bool(JSON* object, const char* name)
{
	JSON* tmp = json_get_member(object, name);
	if (tmp == NULL)
		return 0;
	return tmp->numval;
}

JSON* json_get_members(JSON* object)
{
	if (object->type != JSON_TOBJECT)
		return NULL;
	return object->members;
}

JSON* json_get_member(JSON* object, const char* name)
{
	if (object->type != JSON_TOBJECT)
		return NULL;
	JSON* cur = object->members;
	while (cur)
	{
		if (strcmp(cur->name, name) == 0)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

// Returns a linked list of the elements of a json array
JSON* json_get_elements(JSON* object)
{
	if (object->type != JSON_TARRAY)
		return NULL;
	return object->members;
}

// Returns the next item in the list element is a part of
JSON* json_get_next(JSON* element)
{
	return element->next;
}

#define WRITE_NAME                                     \
	if (object->name)                                  \
	{                                                  \
		json_ss_write(ss, "\"", 0);                    \
		json_ss_write(ss, object->name, 1);            \
		json_ss_write(ss, format ? "\": " : "\":", 0); \
	}

void json_tostring_internal(JSON* object, struct JSONStringStream* ss, int format, size_t depth)
{
	if (object->type == JSON_TOBJECT || object->type == JSON_TARRAY)
	{
		WRITE_NAME;
		json_ss_write(ss, (object->type == JSON_TOBJECT ? "{" : "["), 0);
		if (format)
			json_ss_write(ss, "\n", 0);

		JSON* cur = object->members;
		while (cur)
		{
			// Format with tabs
			if (format)
			{
				for (size_t i = 0; i < depth + 1; i++)
				{
					json_ss_write(ss, "\t", 0);
				}
			}
			json_tostring_internal(cur, ss, format, depth + 1);
			cur = cur->next;
			if (cur)
				json_ss_write(ss, (format ? ",\n" : ","), 0);
		}
		if (format)
		{
			json_ss_write(ss, "\n", 0);
			for (size_t i = 0; i < depth; i++)
			{
				json_ss_write(ss, "\t", 0);
			}
		}
		json_ss_write(ss, (object->type == JSON_TOBJECT ? "}" : "]"), 0);
	}
	else if (object->type == JSON_TSTRING)
	{
		WRITE_NAME;
		json_ss_write(ss, "\"", 0);

		json_ss_write(ss, object->stringval, 1);
		json_ss_write(ss, "\"", 0);
	}
	else if (object->type == JSON_TNUMBER)
	{
		WRITE_NAME;
		char buf[128];
		json_ftos(object->numval, buf, 5);
		json_ss_write(ss, buf, 0);
	}
	else if (object->type == JSON_TBOOL)
	{
		WRITE_NAME;
		json_ss_write(ss, object->numval ? "true" : "false", 0);
	}
	else if (object->type == JSON_TNULL)
	{
		WRITE_NAME;
		json_ss_write(ss, "null", 0);
	}
}

char* json_tostring(JSON* object, int format)
{
	struct JSONStringStream ss = {0};

	json_tostring_internal(object, &ss, format, 0);
	return ss.str;
}

int json_writefile(JSON* object, const char* filepath, int format)
{
// Create directories leading up
#if JSON_USE_POSIX
	const char* p = filepath;
	char tmp_path[FILENAME_MAX];
	size_t len = 0;
	struct stat st = {0};
	for (; *p != '\0'; p++)
	{
		// Dir separator hit
		if (*p == '/')
		{
			len = p - filepath;

			memcpy(tmp_path, filepath, len);
			tmp_path[len] = '\0';

			// Dir already exists
			if (stat(tmp_path, &st) == 0)
			{
				continue;
			}
			if (mkdir(tmp_path, 0777))
			{
				char msg[512];
				snprintf(msg, sizeof msg, "Failed to create directory %s", tmp_path);
				JSON_MESSAGE(msg);
				return -1;
			}
		}
	}
#elif JSON_USE_WINAPI
	char* p = filepath;
	const char tmp_path[FILENAME_MAX];
	for (; *p != '\0'; p++)
	{
		// Dir separator hit
		if (*p == '/' || *p == '\\')
		{
			size_t len = p - filepath;
			memcpy(tmp_path, filepath, p - filepath);
			tmp_path[len] = '\0';
			CreateDirectoryA(tmp_path, NULL);
		}
	}
#endif
	FILE* fp = NULL;
	fp = fopen(filepath, "w");
	if (fp == NULL)
	{

		char msg[512];
		snprintf(msg, sizeof msg, "Failed to create or open file %s", filepath);
		JSON_MESSAGE(msg);
		return -2;
	}
	struct JSONStringStream ss = {0};
	json_tostring_internal(object, &ss, format, 0);
	fwrite(ss.str, 1, ss.length, fp);

	// Exit
	JSON_FREE(ss.str);
	fclose(fp);
	return 0;
}

JSON* json_loadfile(const char* filepath)
{
	FILE* fp;
	fp = fopen(filepath, "r");
	if (fp == NULL)
	{
		char msg[512];
		snprintf(msg, sizeof msg, "Failed to open file %s", filepath);
		JSON_MESSAGE(msg);
		return NULL;
	}

	// Read the file into a string
	char* buf = NULL;
	size_t size;
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	buf = JSON_MALLOC(size);
	fseek(fp, 0L, SEEK_SET);
	fread(buf, 1, size, fp);
	fclose(fp);

	JSON* root = JSON_MALLOC(sizeof(JSON));
	if (json_load(root, buf) == NULL)
	{
		char msg[512];
		snprintf(msg, sizeof msg, "File %s contains none or invalid json data", filepath);
		JSON_MESSAGE(msg);
		JSON_FREE(root);
		JSON_FREE(buf);
		return NULL;
	}
	root->name = strduplicate(filepath);
	JSON_FREE(buf);
	return root;
}
JSON* json_loadstring(char* str)
{
	JSON* root = JSON_MALLOC(sizeof(JSON));
	if (json_load(root, str) == NULL)
	{
		JSON_MESSAGE("String contains none or invalid json data");
		JSON_FREE(root);
		return NULL;
	}
	return root;
}

char* json_load(JSON* object, char* str)
{
	object->type = JSON_TINVALID;

	object->name = NULL;
	object->stringval = NULL;
	object->numval = 0;
	object->members = NULL;
	object->count = 0;
	object->next = NULL;

	// Object
	if (str[0] == '{')
	{
		object->type = JSON_TOBJECT;

		char* tmp_name = NULL;
		str++;
		for (; *str != '\0'; str++)
		{
			// Skip whitespace
			if (JSON_IS_WHITESPACE(*str))
			{
				continue;
			}

			// Read the name
			if (*str == '"')
			{
				char* tmp = json_read_quote(str, &tmp_name);
				if (tmp == NULL)
				{
					char msg[512];
					snprintf(msg, sizeof msg, "Error reading characters in string \"%.15s\"", str);
					JSON_MESSAGE(msg);
					break;
				}
				str = tmp;
			}

			// Next side of key value pair
			// After reading the key, recursively load the value
			if (*str == ':')
			{
				// Jump over ':'
				str++;
				// Skip all whitespace after ':'
				while (*str == ' ' || *str == '\t' || *str == '\n')
					str++;

				// Load the json with what is after the ':'
				JSON* new_object = JSON_MALLOC(sizeof(JSON));

				// Load the child element from the string and skip over that string
				char* tmp_buf = json_load(new_object, str);

				if (tmp_buf == NULL || new_object->type == JSON_TINVALID)
				{
					char msg[512];
					snprintf(msg, sizeof msg, "Invalid json %.15s", str);
					JSON_MESSAGE(msg);
					json_destroy(new_object);
				}
				else
				{
					str = tmp_buf;
				}

				// Insert member
				json_add_member(object, tmp_name, new_object);
				JSON_FREE(tmp_name);

				// Skip to next comma or quit
				for (; *str != '\0'; str++)
				{
					if (*str == ',')
						break;
					if (*str == '}')
						return str + 1;
					if (JSON_IS_WHITESPACE(*str))
						continue;
					char msg[512];
					snprintf(msg, sizeof msg, "Unexpected character before comma %.15s", str);
					JSON_MESSAGE(msg);
					return NULL;
				}
				if (*str == '\0')
				{
					JSON_MESSAGE("Expected comma before end of string");
					return NULL;
				}
				continue;
			}

			char msg[512];
			snprintf(msg, sizeof msg, "Expected property before \"%.15s\"", str);
			JSON_MESSAGE(msg);
			return str;
		}
	}

	// Array
	else if (str[0] == '[')
	{
		object->type = JSON_TARRAY;

		str++;
		for (; *str != '\0'; str++)
		{
			// Skip whitespace
			if (JSON_IS_WHITESPACE(*str))
				continue;

			// The end of the array
			if (*str == ']')
				return str + 1;

			// Read elements of array

			{
				JSON* new_object = JSON_MALLOC(sizeof(JSON));

				// Load the element from the string
				char* tmp_buf = json_load(new_object, str);
				if (tmp_buf == NULL)
				{
					JSON_MESSAGE("Invalid json");
					json_destroy(new_object);
				}
				else
				{
					str = tmp_buf;
				}

				// Insert element at end
				json_add_element(object, new_object);

				// Skip to next comma or quit
				for (; *str != '\0'; str++)
				{
					if (*str == ',')
						break;
					if (*str == ']')
						return str + 1;
					if (JSON_IS_WHITESPACE(*str))
						continue;
					char msg[512];
					snprintf(msg, sizeof msg, "Unexpected character before comma \"%.15s\"\n", str);
					JSON_MESSAGE(msg);
					return str;
				}
			}

			// The end of the array
			if (*str == ']')
			{
				return str + 1;
			}
		}
	}

	// String
	else if (str[0] == '"')
	{
		object->type = JSON_TSTRING;
		return json_read_quote(str, &object->stringval);
	}
	// Number
	else if ((*str >= '0' && *str <= '9') || *str == '-' || *str == '+')
	{
		object->type = JSON_TNUMBER;
		return json_stof(str, &object->numval);
	}

	// Bool true
	else if (strncmp(str, "true", 4) == 0)
	{
		object->type = JSON_TBOOL;
		object->numval = 1;
		return str + 4;
	}

	// Bool true
	else if (strncmp(str, "false", 5) == 0)
	{
		object->type = JSON_TBOOL;
		object->numval = 0;
		return str + 5;
	}

	else if (strncmp(str, "null", 4) == 0)
	{
		object->type = JSON_TNULL;
		return str + 4;
	}

	return NULL;
}

// Removes and returns a member from the json structure
JSON* json_remove_member(JSON* object, const char* name)
{
	if (object->type != JSON_TOBJECT)
		return NULL;
	JSON* cur = object->members;
	JSON* prev = NULL;
	while (cur && strcmp(cur->name, name) != 0)
	{
		prev = cur;
		cur = cur->next;
	}
	// Name was not found
	if (cur == NULL)
	{
		return NULL;
	}

	object->count--;
	// Handle first item
	// Set to the following item or NULL
	if (prev == NULL)
	{
		object->members = cur->next;
		return cur;
	}

	// Middle and end

	prev->next = cur->next;
	return cur;
}

// Removes and returns an element from the json structure
JSON* json_remove_element(JSON* object, int pos)
{
	if (object->type != JSON_TARRAY)
		return NULL;
	JSON* cur = object->members;
	JSON* prev = NULL;
	int idx = 0;
	// Loop to either index or end of list
	while (cur != NULL && idx != pos)
	{
		prev = cur;
		cur = cur->next;
	}

	// Index was not found
	if (cur == NULL)
	{
		return NULL;
	}
	object->count--;
	// Handle first item
	// Set to the following item or NULL
	if (prev == NULL)
	{
		object->members = cur->next;
		return cur;
	}

	// Middle and end
	prev->next = cur->next;
	return cur;
}

void json_add_member(JSON* object, const char* name, JSON* value)
{
	if (object->type != JSON_TOBJECT)
	{
		json_set_invalid(object);
	}
	object->type = JSON_TOBJECT;

	value->name = strduplicate(name);

	// If list is empty
	if (object->members == NULL)
	{
		object->members = value;
		return;
	}

	// Traverse to end of array and look for duplicate
	JSON* cur = object->members;
	JSON* prev = NULL;
	while (cur && strcmp(cur->name, name) != 0)
	{
		prev = cur;
		cur = cur->next;
	}

	// No duplicate, insert at end
	if (cur == NULL)
	{
		object->count++;
		prev->next = value;
		return;
	}
	// Duplicate
	value->next = cur->next;

	if (prev)
		prev->next = value;
	// Handle beginning of list
	else
		object->members = value;

	json_destroy(cur);
	return;
}

void json_insert_element(JSON* object, int pos, JSON* element)
{
	if (object->type != JSON_TARRAY)
	{
		json_set_invalid(object);
	}
	object->type = JSON_TARRAY;
	object->count++;
	// List is empty
	if (object->members == NULL)
	{
		object->members = element;
		return;
	}

	JSON* cur = object->members;
	JSON* prev = NULL;
	int idx = 0;
	// Jump to index or end of array
	while (cur != NULL && idx != pos)
	{
		idx++;
		prev = cur;
		cur = cur->next;
	}

	// Insert
	// Beginning
	if (prev == NULL)
	{
		object->members = element;
		element->next = cur;
		return;
	}

	// End or middle
	prev->next = element;
	element->next = cur;
}

void json_add_element(JSON* object, JSON* element)
{
	json_insert_element(object, 0, element);
}

void json_destroy(JSON* object)
{
	JSON* cur = object->members;
	while (cur)
	{
		JSON* next = cur->next;
		json_destroy(cur);
		cur = next;
	}

	if (object->name)
	{
		JSON_FREE(object->name);
		object->name = NULL;
	}
	if (object->stringval)
	{
		JSON_FREE(object->stringval);
		object->stringval = NULL;
	}

	object->numval = 0;
	object->type = JSON_TINVALID;

	JSON_FREE(object);
}
#endif
#endif

// LICENSE
//
// MIT License
//
// Copyright (c) 2020 Tim Roberts
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.