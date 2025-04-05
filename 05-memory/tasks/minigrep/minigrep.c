#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pcre.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>

typedef struct {
    void   *data;    
    size_t  readPos; 
    size_t  linePos; 
    int     fd;      
    size_t  size;    
} MappedFile;

#define MAPPED_FILE_RESET_ITER(mf) ((mf).readPos = (mf).linePos = 0)

int  openMappedFileRead(MappedFile *mf, const char *path);
int  closeMappedFile(MappedFile *mf);
bool mappedFileGetLine(MappedFile *mf, char **line);
char *buildAbsolutePath(const char *dir, const char *fname);

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <регулярное_выражение> <директория>\n", argv[0]);
        return 1;
    }

    const char *pattern = argv[1];
    const char *directory = argv[2];
    struct stat dirStat;

    if (stat(directory, &dirStat) != 0 || !S_ISDIR(dirStat.st_mode)) {
        fprintf(stderr, "Указанный путь не является директорией\n");
        return 1;
    }

    DIR *dirHandle = opendir(directory);
    if (!dirHandle) {
        fprintf(stderr, "Не удалось открыть директорию: %s\n", directory);
        return 1;
    }

    const char *errorMsg = NULL;
    int errorOffset = 0;

    pcre *regex = pcre_compile(pattern, 0, &errorMsg, &errorOffset, NULL);
    if (!regex) {
        fprintf(stderr, "Ошибка компиляции регулярного выражения на позиции %d: %s\n", errorOffset, errorMsg);
        closedir(dirHandle);
        return 1;
    }

    struct dirent *entry = NULL;
    while ((entry = readdir(dirHandle)) != NULL) {
        char *fullPath = buildAbsolutePath(directory, entry->d_name);

        struct stat fileStat;
        if (stat(fullPath, &fileStat) != 0 || S_ISDIR(fileStat.st_mode)) {
            free(fullPath);
            continue;
        }

        MappedFile mf;
        if (openMappedFileRead(&mf, fullPath) < 0) {
            free(fullPath);
            continue;
        }

        size_t lineNumber = 0;
        MAPPED_FILE_RESET_ITER(mf);

        char *currentLine = NULL;
        while (mappedFileGetLine(&mf, &currentLine)) {
            lineNumber++;
            if (!currentLine) {
                continue;
            }

            int outVec[30];
            int rc = pcre_exec(regex, NULL, currentLine, (int)strlen(currentLine), 0, 0, outVec, 30);
            if (rc > 0) {
                fprintf(stdout, "%s:%lu: %s\n", fullPath, (unsigned long)lineNumber, currentLine);
            }

            free(currentLine);
            currentLine = NULL;
        }

        closeMappedFile(&mf);
        free(fullPath);
    }

    pcre_free(regex);
    closedir(dirHandle);
    return 0;
}

int openMappedFileRead(MappedFile *mf, const char *path)
{
    mf->fd = open(path, O_RDONLY);
    if (mf->fd < 0) {
        return -1;
    }

    struct stat st;
    if (fstat(mf->fd, &st) < 0) {
        close(mf->fd);
        return -1;
    }

    mf->size = st.st_size;
    if (mf->size == 0) {
        mf->data = NULL;
        return 0;
    }

    mf->data = mmap(NULL, mf->size, PROT_READ, MAP_PRIVATE, mf->fd, 0);
    if (mf->data == MAP_FAILED) {
        close(mf->fd);
        return -1;
    }

    return 0;
}

int closeMappedFile(MappedFile *mf)
{
    int result = 0;
    if (mf->data && mf->data != MAP_FAILED) {
        result += munmap(mf->data, mf->size);
    }
    if (mf->fd >= 0) {
        result += close(mf->fd);
    }
    return result;
}

bool mappedFileGetLine(MappedFile *mf, char **line)
{
    if (*line) {
        free(*line);
        *line = NULL;
    }

    if (mf->size == 0 || mf->readPos >= mf->size) {
        return false;
    }

    size_t start = mf->linePos;
    size_t pos = start;
    while (pos < mf->size && ((char *)mf->data)[pos] != '\n') {
        pos++;
    }

    size_t length = pos - start;
    *line = (char *)malloc(length + 1);
    if (!*line) {
        fprintf(stderr, "Не удалось выделить память под строку\n");
        exit(1);
    }

    memcpy(*line, (char *)mf->data + start, length);
    (*line)[length] = '\0';

    mf->readPos = (pos < mf->size) ? (pos + 1) : mf->size;
    mf->linePos = mf->readPos;

    return true;
}

char *buildAbsolutePath(const char *root, const char *filename)
{
    size_t rootLen = strlen(root);
    size_t fileLen = strlen(filename);

    char *absPath = (char *)malloc(rootLen + fileLen + 2);
    if (!absPath) {
        fprintf(stderr, "Не удалось выделить память под путь\n");
        exit(1);
    }

    memcpy(absPath, root, rootLen);
    absPath[rootLen] = '/';
    memcpy(absPath + rootLen + 1, filename, fileLen);
    absPath[rootLen + fileLen + 1] = '\0';

    return absPath;
}
