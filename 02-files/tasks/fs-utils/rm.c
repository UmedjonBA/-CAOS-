#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

    void print_errors(const char* file_path) {
  if (errno == EISDIR) {
    printf("%s", "rm: cannot remove '");
    printf("%s", file_path);
    printf("%s", "': Is a directory\n");

  } else if (errno == EPERM) {
    printf("%s", "rm: cannot remove '");
    printf("%s", file_path);
    printf("%s", "': No access rights\n");

  } else if (errno == ENOENT) {
    printf("%s", "rm: cannot remove '");
    printf("%s", file_path);
    printf("%s", "': No such file or directory\n");

  } else if (errno == EBADF) {
    printf("%s", "rm: cannot remove '");
    printf("%s", file_path);
    printf("%s", "': Invalid directory descriptor\n");

  } else {
    printf("%s", "rm: cannot remove '");
    printf("%s", file_path);
    printf("%s", "': happened some cringe\n");
  }
}
size_t len_str(char* str) {
  size_t len = 0;
  while (str[len] != '\0') {
    ++len;
  }
  return len;
}

int rm_one_file(char* file_path) {
  struct stat file_info;
  int error_checking = lstat(file_path, &file_info);
  if (error_checking < 0) {
    print_errors(file_path);
    return -1;
  }

  if (S_ISDIR(file_info.st_mode)) {
    errno = EISDIR;
    print_errors(file_path);
    return -1;
  }

  int error_checking1 = unlink(file_path);
  if (error_checking1 < 0) {
    print_errors(file_path);
    return -1;
  }
  return 0;
}

int rm_emty_dir(char* dir_path) {
  int error_checking = rmdir(dir_path);
  if (error_checking < 0) {
    print_errors(dir_path);
    return -1;
  }
  return 0;
}

int rm_no_recursive(int argc, char** argv, int begin) {
  for (size_t i = begin; i + 1 < argc; ++i) {
    rm_one_file(argv[i]);
  }

  return rm_one_file(argv[argc - 1]);
}

int rm_one_dir_recursive(char* dir_path) {
  struct stat file_info;
  int error_checking = lstat(dir_path, &file_info);
  if (error_checking < 0) {
    print_errors(dir_path);
    return -1;
  }
  if (!S_ISDIR(file_info.st_mode)) {
    return rm_one_file(dir_path);
  }  // the case when dir_name isn't dir

  DIR* dir = opendir(dir_path);
  if (dir == NULL) {
    print_errors(dir_path);
    return -1;
  }

  struct dirent* entry;
  while (true) {
    errno = 0;
    entry = readdir(dir);
    if (entry == NULL) {
      if (errno == 0) {
        break;
      }
      closedir(dir);
      return -1;
    }
    char
        new_path[len_str(dir_path) + len_str("/") + len_str(entry->d_name) + 1];
    snprintf(new_path, sizeof new_path, "%s%s%s", dir_path, "/", entry->d_name);

    struct stat file_info;
    int error_checking1 = lstat(new_path, &file_info);
    if (error_checking1 < 0) {
      print_errors(entry->d_name);
      closedir(dir);
      return -1;
    }

    if (S_ISDIR(file_info.st_mode) && ((strcmp(entry->d_name, "..") == 0) ||
                                       (strcmp(entry->d_name, ".") == 0))) {
      continue;
    }

    if (S_ISDIR(file_info.st_mode)) {
      int check_error = rm_one_dir_recursive(new_path);
      if (check_error < 0) {
        closedir(dir);
        return -1;
      }
      continue;
    }

    int error_checking2 = rm_one_file(new_path);
    if (error_checking2 < 0) {
      closedir(dir);
      return -1;
    }
  }
  if (closedir(dir) < 0) {
    printf("couldn't close the directory\n");
    return -1;
  }
  return rm_emty_dir(dir_path);
}

int rm_recursive(int argc, char** argv, int begin) {
  for (size_t i = begin; i + 1 < argc; ++i) {
    rm_one_dir_recursive(argv[i]);
  }

  return rm_one_dir_recursive(argv[argc - 1]);
}

int main(int argc, char** argv) {
  if (argc == 1) {
    errno = EINVAL;
    printf("%s", "nothig to remove\n");
    return -1;
  }

  bool is_recursive = false;
  int opt = 0;
  while ((opt = getopt(argc, argv, "r")) != -1) {
    switch (opt) {
      case 'r':
        is_recursive = true;
        break;
      default:
        break;
    }
  }

  if (is_recursive) {
    if (argc == optind) {
      errno = EINVAL;
      printf("%s", "nothig to remove\n");
      return -1;
    }
    return rm_recursive(argc, argv, optind);
  }

  return rm_no_recursive(argc, argv, optind);
}