#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    bool return_value;
    int error_code;
} return_type;

return_type
is_exists(const char *file_path)
{
    return_type answer;
    struct stat checker;
    answer.error_code = stat(file_path, &checker);
    if (answer.error_code < 0 || errno == ENOENT) {
      answer.error_code = 0;
      answer.return_value = false;
      return answer;
    }
    answer.return_value = true;
    answer.error_code = 0;
    return answer;
}

int
tailf(const char *file_path)
{
    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }
    int shift = lseek(fd, 0, SEEK_SET);
    if (shift < 0)
    {
        return -1;
    }
    struct stat file_info;
    off_t old_size = 0;

    while (true)
    {
    int error_checking = stat(file_path, &file_info);
    if (error_checking < 0)
    {
        return -1;
    }
    off_t new_size = file_info.st_size;

    if (old_size < new_size)
    {
        char *buf = (char *)malloc(new_size - old_size);
        int we_have_read = read(fd, buf, new_size - old_size);
        if (we_have_read < 0)
        {
          return -1;
        }
        int we_have_write = write(1, buf, new_size - old_size);
        if (we_have_write < 0)
        {
          return -1;
        }
        old_size = new_size;
        free(buf);
        buf = NULL;
    }
    usleep(1000);
    }
    return 0;
}

int
main(int argc, char **argv)
{
    if (argc != 2)
    {
        return -1;
    }
    if (is_exists(argv[1]).error_code < 0)
    {
        return -1;
    }
    else if (is_exists(argv[1]).return_value == false) 
    {
        printf("%s", "file does not exist");
        return -1;
    }
    return tailf(argv[1]);
}