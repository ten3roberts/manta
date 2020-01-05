#include <stdio.h>
#include "utils.h"

int main(int argc, char** argv)
{
    char buf[100];
    dir_up(argv[0], buf, sizeof buf, 2);
    set_workingdir(buf);


    char* files[16];
    for(size_t i = 0; i < 10; i++)
    {
        files[i] = malloc(512);
    }

    printf("%zu\n", listdir("/home/ten3roberts/lastmod", files, 10, 3));
    find_file("/home/ten3roberts", buf, sizeof buf, "Tools.h");
    puts(buf);
}