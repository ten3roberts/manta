#pragma once

// Contains utility functions used by the engine
#include <stdlib.h>

// Returns 1 if the path is a regular file
int is_regular_file(const char * path);

// Returns 1 if the path is a directory
int is_dir(const char * path);

// Lists all files and sub directories recursively. Set depth to 0 to list unlimited
// Fills string array with relative path
size_t listdir(const char * dir, char ** result, size_t size, size_t depth);

// Finds a file and returns the relative path to it
// If the file is not found, result is not modified and EXIT_SUCCESS is returned
// If result is NULL, the function only returns the existence of the file
int find_file(const char * dir, char * result, size_t size, const char * filename);

// Will create all dirs in path
void create_dirs(const char* path);

// Gets the file name from a path and copies it to result
void get_filename(const char * path, char * result, size_t size);

// Gets the dir path from a file path and copies it to result
void get_dir(const char * path, char * result, size_t size);

// Changes the working directory
void set_workingdir(const char * dir);

// Goes back or up one directory in the file tree
// Does not include the last dir delimeter
void dir_up(const char * path, char * result, size_t size, size_t steps);

// Will convert a path with \\ to /
void posix_path(const char * path, char * result, size_t size);

// Will replace
void replace_string(const char * src, char * result, size_t size, char find, char replace);

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)
