#include "bloom_filter.h"
#include <stddef.h>
#include <stdlib.h>
uint64_t
calc_hash(const char* str, uint64_t modulus, uint64_t seed) {
    size_t i = 0;
    uint64_t hash = 0;
    uint64_t seed_pow = 1;
    while (str[i] != '\0')
    {
        hash = (hash + (str[i] - 'a' + 1) * seed_pow);
        seed_pow = (seed_pow * seed);
        ++i;
    }
    hash %= modulus;
    
    return hash;
}

void
bloom_init(struct BloomFilter* bloom_filter, uint64_t set_size, hash_fn_t hash_fn, uint64_t hash_fn_count) {
    bloom_filter->set_size = set_size;
    bloom_filter->set = (uint64_t*)calloc(bloom_filter->set_size, sizeof(uint64_t));
    bloom_filter->hash_fn = hash_fn;
    bloom_filter->hash_fn_count = hash_fn_count;
}

void
bloom_destroy(struct BloomFilter* bloom_filter) {
    free(bloom_filter->set);
    bloom_filter->set = NULL;
}

void
bloom_insert(struct BloomFilter* bloom_filter, Key key) {
    uint64_t hash;
    for(size_t i = 0; i < bloom_filter->hash_fn_count; ++i)
    {
        hash = bloom_filter->hash_fn(key,bloom_filter->set_size * 64, i);
        uint64_t bit = 1;
        bit << (hash % 64);
        bloom_filter->set[hash / 64] |= bit;
    }
}

bool
bloom_check(struct BloomFilter* bloom_filter, Key key) {
    uint64_t hash;
    for(size_t i = 0; i < bloom_filter->hash_fn_count; ++i)
    {
        hash = bloom_filter->hash_fn(key,bloom_filter->set_size * 64, i);
        uint64_t bit = 1;
        bit << (hash % 64);
        bit &= bloom_filter->set[hash / 64];
        if(bit == 0) 
        {
            return false;
        }
    }
    return true;
}
