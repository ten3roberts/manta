#include "math/vec3.h"
#include "utils.h"
#include <stdio.h>
#include <time.h>

int main(int argc, char ** argv)
{
    char buf[100];
    dir_up(argv[0], buf, sizeof buf, 2);
    set_workingdir(buf);
}