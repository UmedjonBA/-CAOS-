#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct node {
    struct node* next;
    uintptr_t    value;
} node_t;

typedef struct lfstack {
    _Atomic(node_t*) top;
} lfstack_t;

int lfstack_init(lfstack_t* stack) {
    stack->top = NULL;
    return 0;
}

int lfstack_push(lfstack_t* stack, uintptr_t value) {
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    if (!new_node) {
        return -1;
    }

    new_node->value = value;

    while (true) {
        node_t* old_top = atomic_load(&stack->top);
        new_node->next  = old_top;
        if (atomic_compare_exchange_weak(&stack->top, &old_top, new_node)) {
            break;
        }
        usleep(10000);
    }

    return 0;
}

int lfstack_pop(lfstack_t* stack, uintptr_t* value) {
    node_t* top_node = atomic_load(&stack->top);
    if (!top_node) {
        *value = 0;
        return 0;
    }

    while (top_node) {
        if (atomic_compare_exchange_weak(&stack->top, &top_node, top_node->next)) {
            atomic_store(value, top_node->value);
            free(top_node);
            break;
        }
        usleep(10000);
    }

    return 0;
}

int lfstack_destroy(lfstack_t* stack) {
    stack->top = NULL;
    return 0;
}
