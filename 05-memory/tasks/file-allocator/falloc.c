#include "falloc.h"
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int cnt = 0;

void set_bit_mask(file_allocator_t *allocator, uint64_t pos) {
    uint64_t id1 = pos / 64;
    uint64_t id2 = 1;
    id2 = id2 << (pos % 64);
    allocator->page_mask[id1] |= id2;
    allocator->curr_page_count++;
}

void dlt_bit_mask(file_allocator_t *allocator, uint64_t pos) {
    uint64_t id1 = pos / 64;
    uint64_t id2 = 1;
    id2 = id2 << (pos % 64);
    if ((allocator->page_mask[id1] & id2)) {
        allocator->page_mask[id1] ^= id2;
        allocator->curr_page_count--;
    }
}

bool is_free(file_allocator_t *allocator, uint64_t pos) {
    int id1 = pos / 64;
    uint64_t id2 = 1;
    id2 = id2 << (pos % 64);
    if ((allocator->page_mask[id1] & id2)) {
        return false;
    }
    return true;
}

void falloc_init(file_allocator_t* allocator, const char* filepath, uint64_t allowed_page_count) {
    int cnt_bit = allowed_page_count * PAGE_SIZE + PAGE_MASK_SIZE;
    int fd = open(filepath, O_RDWR | O_CREAT, 0777);
    if (fd == -1) {
        return;
    }
    int ft = ftruncate(fd, cnt_bit);
    allocator->fd = fd;
    allocator->allowed_page_count = allowed_page_count;

    void* addr = mmap(NULL, cnt_bit, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    allocator->page_mask = addr;
    addr += PAGE_MASK_SIZE;

    allocator->base_addr = addr;
    allocator->curr_page_count = 0;
    for (uint64_t i = 0; i < allowed_page_count; i++) {
        if (!is_free(allocator, i)) {
            allocator->curr_page_count++;
        }
    }
}

void falloc_destroy(file_allocator_t* allocator) {
    uint64_t cnt_bit = allocator->allowed_page_count * PAGE_SIZE +
                       PAGE_MASK_SIZE;
    munmap(allocator->page_mask, cnt_bit);
    allocator->base_addr = NULL;
    allocator->page_mask = NULL;
    close(allocator->fd);
}

void* falloc_acquire_page(file_allocator_t* allocator) {
    for (int i = 0; i < allocator->allowed_page_count; i++) {
        if (is_free(allocator, i)) {
            set_bit_mask(allocator, i);
            void* res = allocator->base_addr;
            res = res + i * PAGE_SIZE;
            return res;
        }
    }
    return NULL;
}

void falloc_release_page(file_allocator_t* allocator, void** addr) {
    for (int i = 0; i < allocator->allowed_page_count; i++) {
        if (*addr == (void*)(allocator->base_addr + i * PAGE_SIZE)) {
            dlt_bit_mask(allocator, i);
            *addr = NULL;
            return;
        }
    }
}