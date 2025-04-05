#pragma once

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "wait.h"

#define CONDITION_WAIT 0
#define CONDITION_WORK 1
#define CONDITION_END  2

typedef double field_t;
typedef field_t func_t(field_t);

static const field_t INTEGRATION_EPS = 1e-7;

typedef struct worker_context {
    _Atomic(uint32_t) condition;
    field_t           left_bound;
    field_t           right_bound;
    func_t*           func;
    field_t           result;
} worker_context_t;

typedef struct par_integrator {
    worker_context_t* thread_ctx_array;
    size_t            threads_count;
    pthread_t*        thread_ids;
} par_integrator_t;

static void* thread_func_calc_segment(void* arg) {
    worker_context_t* ctx = (worker_context_t*)arg;

    while (true) {
        atomic_wait(&ctx->condition, CONDITION_WAIT);

        if (atomic_load(&ctx->condition) == CONDITION_END) {
            return NULL;
        }

        field_t pos = ctx->left_bound;
        for (; pos + INTEGRATION_EPS < ctx->right_bound; pos += INTEGRATION_EPS) {
            ctx->result += INTEGRATION_EPS * ctx->func(pos);
        }

        if (pos < ctx->right_bound) {
            ctx->result += (ctx->right_bound - pos) * ctx->func(pos);
        }

        atomic_store(&ctx->condition, CONDITION_WAIT);
        atomic_notify_one(&ctx->condition);
    }
}

int par_integrator_init(par_integrator_t* self, size_t threads_num) {
    self->threads_count   = threads_num;
    self->thread_ids      = (pthread_t*)malloc(threads_num * sizeof(pthread_t));
    self->thread_ctx_array = (worker_context_t*)malloc(threads_num * sizeof(worker_context_t));

    for (size_t i = 0; i < threads_num; ++i) {
        self->thread_ctx_array[i].condition   = CONDITION_WAIT;
        self->thread_ctx_array[i].func        = NULL;
        self->thread_ctx_array[i].left_bound  = 0.0;
        self->thread_ctx_array[i].right_bound = 0.0;
        self->thread_ctx_array[i].result      = 0.0;

        pthread_create(&self->thread_ids[i], NULL, thread_func_calc_segment, (void*)&self->thread_ctx_array[i]);
    }
    return 0;
}

int par_integrator_start_calc(par_integrator_t* self, func_t* func, field_t left_bound, field_t right_bound) {
    field_t segment_length = (right_bound - left_bound) / self->threads_count;

    for (size_t i = 0; i < self->threads_count; ++i) {
        worker_context_t* ctx = &self->thread_ctx_array[i];
        ctx->func        = func;
        ctx->left_bound  = left_bound + i * segment_length;
        ctx->right_bound = left_bound + (i + 1) * segment_length;
        ctx->result      = 0.0;

        atomic_store(&ctx->condition, CONDITION_WORK);
        atomic_notify_one(&ctx->condition);
    }
    return 0;
}

int par_integrator_get_result(par_integrator_t* self, field_t* result) {
    *result = 0.0;
    for (size_t i = 0; i < self->threads_count; ++i) {
        worker_context_t* ctx = &self->thread_ctx_array[i];

        atomic_wait(&ctx->condition, CONDITION_WORK);

        *result += ctx->result;
    }
    return 0;
}

int par_integrator_destroy(par_integrator_t* self) {
    for (size_t i = 0; i < self->threads_count; ++i) {
        self->thread_ctx_array[i].condition = CONDITION_END;
        atomic_notify_one(&self->thread_ctx_array[i].condition);
        pthread_join(self->thread_ids[i], NULL);
    }

    free(self->thread_ids);
    free(self->thread_ctx_array);

    self->threads_count    = 0;
    self->thread_ids       = NULL;
    self->thread_ctx_array = NULL;

    return 0;
}
