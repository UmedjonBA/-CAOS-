#include <fcntl.h>

#include "storage.h"




void print_errors_for_mkdir(const char *file_path) {
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
  }
  if (answer.error_code < 0 && errno != ENOENT) {
    return answer;
  }
  answer.return_value = true;
  answer.error_code = 0;
  return answer;
}

int mkdir_one_no_mode_p(const char *dir_path, int mode, bool print_error) {
  if (mkdir(dir_path, mode) < 0) {
    if (print_error) {
      print_errors_for_mkdir(dir_path);
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

  char parent_dir[last_slash + 1];
  memcpy(parent_dir, dir_path, last_slash);
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
  if (mkdir_one_no_mode_p(dir_path, mode,  false) < 0) {
    print_errors_for_mkdir(dir_path);
    return -1;
  }
  return 0;
}


static const uint64_t MAGIC_CHAR = 'p';
static const int64_t SIZE_UINT64 = 8;  // in byte

size_t len_str(const char* str) {
  size_t len = 0;
  while (str[len] != '\0') {
    ++len;
  }
  return len;
}

bool is_empty(const char* file_path) {
  struct stat file_info;
  int check_error = stat(file_path, &file_info);
  if (check_error < 0) {
    printf("is_empty: stat couldn't work out\n");
    exit(EXIT_FAILURE);
  }
  return file_info.st_size == 0;
}

typedef struct {
  char* dir_path;
  char* file_path;
} value_path_t;

/* file: [value][size][version] ... [value][size][version]
         4byte  size byte  4byte | 4byte  size byte  4byte */
typedef struct {
  version_t version;   // value version
  uint64_t len_value;  // value length
  char* value;         // value :)
} block_in_file_t;

void storage_init(storage_t* storage, const char* root_path) {
  if (mkdir_one_mode_p(root_path, 0777) < 0) {
    exit(EXIT_FAILURE);
  }
  storage->root_path = root_path;
}

void storage_destroy(storage_t* storage) {
}

/* key = longerkey -> lo/ng/er/ke/y [file_path]
   key = longerkey -> lo/ng/er/ke [dir_path]  */
void parser_key(storage_key_t key, char* file_path, char* dir_path) {
  size_t len_key = len_str(key);
  size_t index_in_value_path = 0;
  size_t index_in_key = 0;

  while (index_in_key + SUBDIR_NAME_SIZE <= len_key) {
    memcpy(file_path + index_in_value_path, key + index_in_key,
           SUBDIR_NAME_SIZE);
    memcpy(dir_path + index_in_value_path, key + index_in_key,
           SUBDIR_NAME_SIZE);

    index_in_value_path += SUBDIR_NAME_SIZE;
    index_in_key += SUBDIR_NAME_SIZE;

    file_path[index_in_value_path] = '/';
    if (index_in_key + SUBDIR_NAME_SIZE <= len_key) {
      dir_path[index_in_value_path] = '/';
    }
    ++index_in_value_path;
  }

  if (len_key % SUBDIR_NAME_SIZE == 0) {
    file_path[index_in_value_path] = MAGIC_CHAR;
    return;
  }
  memcpy(file_path + index_in_value_path, key + index_in_key,
         len_key - index_in_key);
}

void get_value_path(storage_t* storage, storage_key_t key,
                    value_path_t* key_file_path) {
  size_t len_key = len_str(key);
  size_t len_root = len_str(storage->root_path) +
                    1;  // root_path + '/' + (dir_path or file_path)

  key_file_path->file_path =
      calloc(len_root + len_key + len_key / SUBDIR_NAME_SIZE +
                 (len_key % SUBDIR_NAME_SIZE == 0) + 1,
             sizeof(char));
  key_file_path->dir_path =
      calloc(len_root + len_key + len_key / SUBDIR_NAME_SIZE +
                 (len_key % SUBDIR_NAME_SIZE == 0),
             sizeof(char));

  if (key_file_path->dir_path == NULL || key_file_path->file_path == NULL) {
    printf("get_value_path: calloc couldn't work out\n");
    exit(EXIT_FAILURE);
  }

  memcpy(key_file_path->file_path, storage->root_path, len_root - 1);
  memcpy(key_file_path->dir_path, storage->root_path, len_root - 1);

  key_file_path->dir_path[len_root - 1] = '/';
  key_file_path->file_path[len_root - 1] = '/';

  parser_key(key, key_file_path->file_path + len_root,
             key_file_path->dir_path + len_root);
}

int open_key_file(value_path_t key_file_path) {
  if (is_exists_dir(key_file_path.dir_path).error_code < 0) {
    printf("open_key_file: stat in is_exists_dir couldn't work out\n");
    exit(EXIT_FAILURE);
  }
  if (!is_exists_dir(key_file_path.dir_path).return_value) {
    int error_check = mkdir_one_mode_p(key_file_path.dir_path, 0777);
    if (error_check < 0) {
      printf("open_key_file: can't create a directory\n");
      exit(EXIT_FAILURE);
    }
  }

  int fd = open(key_file_path.file_path, O_CREAT | O_RDWR, 0777);
  if (fd < 0) {
    printf("open_key_file: can't open the file\n");
    exit(EXIT_FAILURE);
  }
  return fd;
}

version_t storage_set(storage_t* storage, storage_key_t key,
                      storage_value_t value) {
  value_path_t key_file_path;
  block_in_file_t block_in_file;

  block_in_file.len_value = len_str(value) + 1;  // with '\0
  block_in_file.value = value;
  block_in_file.version = 1;

  get_value_path(storage, key, &key_file_path);

  int fd = open_key_file(key_file_path);
  uint64_t old_version = 0;
  if (!is_empty(key_file_path.file_path)) {
    if (lseek(fd, -SIZE_UINT64, SEEK_END) < 0) {
      printf("storage_set: can't call lseek\n");
      exit(EXIT_FAILURE);
    }
    if (read(fd, &old_version, SIZE_UINT64) < 0) {
      printf("storage_set: can't call read\n");
      exit(EXIT_FAILURE);
    }
    block_in_file.version = old_version + 1;
  }  // get old version

  if (lseek(fd, 0, SEEK_END) < 0) {
    printf("storage_set: can't call lseek\n");
    exit(EXIT_FAILURE);
  }
  int error_check1 = write(fd, block_in_file.value, block_in_file.len_value);
  int error_check2 = write(fd, &block_in_file.len_value, SIZE_UINT64);
  int error_check3 = write(fd, &block_in_file.version, SIZE_UINT64);
  if ((error_check1 < 0 || error_check2 < 0) || error_check3 < 0) {
    printf("storage_set: can't call write\n");
    exit(EXIT_FAILURE);
  }  // push block in file

  close(fd);
  free(key_file_path.dir_path);
  free(key_file_path.file_path);
  return block_in_file.version;
}

version_t storage_get(storage_t* storage, storage_key_t key,
                      returned_value_t returned_value) {
  value_path_t key_file_path;
  block_in_file_t block_in_file;

  get_value_path(storage, key, &key_file_path);

  if ((is_exists_dir(key_file_path.dir_path).error_code < 0 ||
       !is_exists_dir(key_file_path.dir_path).return_value) ||
      is_empty(key_file_path.file_path)) {
    free(key_file_path.dir_path);
    free(key_file_path.file_path);
    return 0;
  }  // by key no value

  int fd = open_key_file(key_file_path);

  if (lseek(fd, -(2 * SIZE_UINT64), SEEK_END) < 0) {
    printf("storage_get: can't call lseek\n");
    exit(EXIT_FAILURE);
  }
  int error_check1 = read(fd, &block_in_file.len_value, SIZE_UINT64);
  int error_check2 = read(fd, &block_in_file.version, SIZE_UINT64);
  if (error_check1 < 0 || error_check2 < 0) {
    printf("storage_get: can't call read\n");
    exit(EXIT_FAILURE);
  }  // read size and version

  if (lseek(fd, -(2 * SIZE_UINT64 + (int64_t)block_in_file.len_value),
            SEEK_END) < 0) {
    printf("storage_get: can't call lseek\n");
    exit(EXIT_FAILURE);
  }
  int error_check = read(fd, returned_value, block_in_file.len_value);
  if (error_check < 0) {
    printf("storage_get: can't call read\n");
    exit(EXIT_FAILURE);
  }  // read value

  close(fd);
  free(key_file_path.dir_path);
  free(key_file_path.file_path);
  return block_in_file.version;
}

version_t storage_get_by_version(storage_t* storage, storage_key_t key,
                                 version_t version,
                                 returned_value_t returned_value) {
  value_path_t key_file_path;
  block_in_file_t block_in_file;

  get_value_path(storage, key, &key_file_path);

  if (is_exists_dir(key_file_path.dir_path).error_code < 0 ||
      !is_exists_dir(key_file_path.dir_path).return_value) {
    printf("storage_get_by_version: by key no value\n");
    exit(EXIT_FAILURE);
  }
  if (is_empty(key_file_path.file_path)) {
    printf("storage_get_by_version: by key no value\n");
    exit(EXIT_FAILURE);
  }

  int fd = open_key_file(key_file_path);

  version_t now_version = 0;
  uint64_t now_len_value = 0;
  if (lseek(fd, 0, SEEK_END) < 0) {
    printf("storage_get_by_version: can't call lseek\n");
    exit(EXIT_FAILURE);
  }

  int64_t shift = 2 * SIZE_UINT64;
  while (version != now_version) {
    int count = lseek(fd, -shift, SEEK_CUR);
    if (count < 0) {
      printf("storage_get_by_version: can't call lseek\n");
      exit(EXIT_FAILURE);
    } else if (count == now_len_value + 2 * SIZE_UINT64) {
      printf(
          "storage_get_by_version: the value with this version does not "
          "exist\n");
      exit(EXIT_FAILURE);
    }
    int error_check1 = read(fd, &now_len_value, SIZE_UINT64);
    int error_check2 = read(fd, &now_version, SIZE_UINT64);
    if (error_check1 < 0 || error_check2 < 0) {
      printf("storage_get_by_version: can't call read\n");
      exit(EXIT_FAILURE);
    }  // read size and version
    shift = now_len_value + 2 * SIZE_UINT64 + 2 * SIZE_UINT64;
  }

  if (lseek(fd, -2 * SIZE_UINT64 - (int64_t)now_len_value, SEEK_CUR) < 0) {
    printf("storage_get_by_version: can't call lseek\n");
    exit(EXIT_FAILURE);
  }
  int error_check = read(fd, returned_value, now_len_value);
  if (error_check < 0) {
    printf("storage_get_by_version: can't call read\n");
    exit(EXIT_FAILURE);
  }  // read value

  close(fd);
  free(key_file_path.dir_path);
  free(key_file_path.file_path);
  return now_version;
}