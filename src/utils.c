#include "utils.h"
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if PL_LINUX
#include <unistd.h>
#elif PL_WINDOWS
#include <Windows.h>
#include <pathect.h>
#include <shlobj_core.h>
#include <stdio.h>
#include <sys/types.h>
#endif

int is_regular_file(const char * path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int is_dir(const char * path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

size_t listdir(const char * dir, char ** result, size_t size, size_t depth)
{
    DIR * dp = opendir(dir);

    struct dirent * ep;
    size_t file_count = 0;
    if (dp != NULL)
    {
        while ((ep = readdir(dp)))
        {
            char full_path[PATH_MAX];
            strcpy(full_path, dir);
            strcpy(full_path, dir);
            if (dir[strlen(dir) - 1] != '/')
                strcat(full_path, ep->d_name);

            if (!strcmp(ep->d_name, "..") || !strcmp(ep->d_name, "."))
                continue;

            if (is_dir(full_path))
            {
                if (depth == 1)
                    continue;
                size_t tmp = listdir(full_path, result, size, depth - 1);
                result += tmp;
                size -= tmp;
            }
            if (result && size)
            {
                strcpy(*result, full_path);
                result++;
                size--;
                file_count++;
            }
        }
        (void)closedir(dp);
    }
    else
    {
        printf("Unable to open directory %s\n", dir);
        return 0;
    }
    return file_count;
}

int find_file(const char * dir, char * result, size_t size, const char * filename)
{
    DIR * dp = opendir(dir);

    struct dirent * ep;
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

void get_filename(const char * path, char * result, size_t size)
{
    for (size_t i = strlen(path); i != 0; i--)
    {
        // If it is at a path delimeter
        if (path[i] == '\\' || path[i] == '/')
        {
            memmove(result, path + min(i + 1, size), min(i + 1, strlen(path) - size));
            result[min(i + 1, size)] = '\0';
            break;
        }
    }
}

void get_dir(const char * path, char * result, size_t size)
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

#if PL_LINUX
void set_workingdir(const char * dir)
{
    if (chdir(dir))
    {
        printf("Invalid working directory %s\n", dir);
    }
}
#elif PL_WINDOWS
void set_workingdir(const char * dir)
{
    _chdir(dir);
}
#endif

void dir_up(const char * path, char * result, size_t size, size_t steps)
{
    for (size_t i = strlen(path); i != 0; i--)
    {
        if (steps == 0)
        {
            memmove(result, path, min(i + 1, size));
            result[min(i + 1, size)] = '\0';
            break;
        }

        // If it is at a path delimeter
        if (path[i] == '\\' || path[i] == '/')
        {
            steps--;
        }
    }
}

void posix_path(const char * path, char * result, size_t size)
{
    if (path != result)
    {
        memmove(result, path, min(strlen(path), size));
        result[min(strlen(path), size)] = '\0';
    }

    for (char * p = result; *p != '\0'; p++)
    {
        if (*p == '\\')
            *p = '/';
    }
}

void replace_string(const char * src, char * result, size_t size, char find, char replace)
{
    if (src != result)
    {
        memmove(result, src, min(strlen(src), size));
        result[min(strlen(src), size)] = '\0';
    }

    for (char * p = result; *p != '\0'; p++)
    {
        if (*p == find)
            *p = replace;
    }
}
