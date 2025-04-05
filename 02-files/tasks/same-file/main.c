#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

bool
is_same_file(const char* lhs_path, const char* rhs_path)
{
    char l_path[1024];
    char r_path[1024];
    if (realpath(lhs_path, l_path) == NULL)
    {
        return 0;
    }
    
    if (realpath(rhs_path, r_path) == NULL)
    {
        return 0;
    }
    struct stat stat1, stat2;
    if (lstat(l_path, &stat1) == -1)
    {
        return 0;
    }

    if (lstat(r_path, &stat2) == -1)
    {
        return 0;
    }

    if (stat1.st_dev == stat2.st_dev && stat1.st_ino == stat2.st_ino) 
    {
        return 1;
    }

    return 0;
}


int
main(int argc, char *argv[]) 
{
    if (argc != 3)
    {
        return 1;
    }
    if(is_same_file(argv[1], argv[2])) printf("yes\n");
    else printf("no\n");
    return 0;
}
