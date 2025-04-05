#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void print_errors(const char *file_path) {
  if (errno == EEXIST) {
    printf("%s", "mkdir: cannot create directory '");
    printf("%s", file_path);
    printf("%s", "': File exists\n");

  } else if (errno == EACCES) {
    printf("%s", "mkdir: cannot create directory '");
    printf("%s", file_path);
    printf("%s", "': Permission denied\n");

  } else {
    printf("%s", "mkdir: cannot create directory '");
    printf("%s", file_path);
    printf("%s", "': happened some cringe\n");
  }
}

typedef struct {
  bool return_value;
  int error_code;
} return_type;

return_type is_exists_dir(const char *dir_path) {
  return_type answer;
  struct stat checker;
  answer.error_code = stat(dir_path, &checker);

  if (answer.error_code < 0 && errno == ENOENT) {
    answer.error_code = 0;
    answer.return_value = false;
    return answer;
  } else if (answer.error_code < 0 && errno != ENOENT) {
    return answer;
  }
  answer.return_value = true;
  answer.error_code = 0;
  return answer;
}

int mkdir_one_no_mode_p(const char *dir_path, int mode, bool print_error) {
  if (mkdir(dir_path, mode) < 0) {
    if (print_error) {
      print_errors(dir_path);
    }
    return -1;
  }
  return 0;
}

int mkdir_one_mode_p(const char *dir_path, int mode) {
  // case: dir already exists
  if (is_exists_dir(dir_path).error_code == 0 &&
      is_exists_dir(dir_path).return_value) {
    return 0;
  }

  // case: parent_dir_path = "/some_dir_name"
  if (strcmp(dir_path, "") == 0) {
    return 0;
  }

  size_t i = 0;
  int last_slash = -1;
  while (dir_path[i] != '\0') {
    if (dir_path[i] == '/') {
      last_slash = i;
    }
    ++i;
  }

  // case: dir_path without parent_dir
  if (last_slash == -1) {
    return mkdir_one_no_mode_p(dir_path, mode, false);
  }
  char *parent_dir = (char *)calloc(last_slash + 1, sizeof(char));
  if (memcpy(parent_dir, dir_path, last_slash) < 0 || parent_dir == NULL) {
    return -1;
  }
  parent_dir[last_slash] = '\0';

  // create parent_dir
  if (mkdir_one_mode_p(parent_dir, 0777) < 0) {
    return -1;
  }

  // case: dir_path = dir_name + '/'
  if (strcmp(dir_path, "") == 0) {
    return 0;
  }

  // case: dir_path = dir_name
  if (mkdir_one_no_mode_p(dir_path, mode, false) < 0) {
    print_errors(dir_path);
    return -1;
  }
  return 0;
}

int my_mkdir(int argc, char **argv, int mode, int begin, bool mode_p) {
  for (size_t i = begin; i + 1 < argc; ++i) {
    if (mode_p) {
      mkdir_one_mode_p(argv[i], mode);
      continue;
    }
    mkdir_one_no_mode_p(argv[i], mode, true);
  }
  if (mode_p) {
    return mkdir_one_mode_p(argv[argc - 1], mode);
  }
  return mkdir_one_no_mode_p(argv[argc - 1], mode, true);
}

int main(int argc, char *argv[]) {
  int opt = 0;

  bool option_p = false;
  int mode = 0777;
  struct option long_options[] = {{"mode", required_argument, NULL, 'm'},
                                  {NULL, 0, NULL, 0}};

  while ((opt = getopt_long(argc, argv, "pm:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'p':
        option_p = true;
        break;
      case 'm':
        mode = strtol(optarg, NULL, 10);
        break;
      case '?':
        printf("Error found !\n");
        break;
      default:
        printf("Error\n");
        break;
    }
  }

  // case: command = mkdir [options] [empty]
  if (argc == optind) {
    errno = EINVAL;
    printf("%s", "nothig to remove\n");
    return -1;
  }

  return my_mkdir(argc, argv, mode, optind, option_p);
}