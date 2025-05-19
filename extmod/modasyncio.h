/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MICROPY_INCLUDED_MODASYNCIO_H
#define MICROPY_INCLUDED_MODASYNCIO_H

#include "py/bc.h"
#include "py/pairheap.h"
#include "py/objgenerator.h"

#define TASK_STATE_RUNNING_NOT_WAITED_ON (mp_const_true)
#define TASK_STATE_DONE_NOT_WAITED_ON (mp_const_none)
#define TASK_STATE_DONE_WAS_WAITED_ON (mp_const_false)

#define TASK_IS_DONE(task) ( \
    (task)->state == TASK_STATE_DONE_NOT_WAITED_ON \
    || (task)->state == TASK_STATE_DONE_WAS_WAITED_ON)

#define TASK_GENERATOR(task) ((const mp_obj_gen_instance_t *)MP_OBJ_TO_PTR((task)->coro))
#define TASK_IS_EXECUTING(task) (TASK_GENERATOR(task)->pend_exc == MP_OBJ_NULL)
#define TASK_CODE_STATE_PTR(task) ((const mp_code_state_t *)&TASK_GENERATOR(task)->code_state)

// MICROPY_PY_ASYNCIO_TASK_HOOK support
#ifndef MICROPY_PY_ASYNCIO_TASK_HOOK
#define MICROPY_PY_ASYNCIO_TASK_HOOK(task, event)
#endif
#define TASK_HOOK_NEW_TASK          1
#define TASK_HOOK_TASK_DONE         0
#define TASK_HOOK_TASK_CANCELLED    -1

#if !defined(VIRTUAL_HUB) && !defined(TEXAS_API)
#define mpy_obj_id(obj) ((mp_int_t)(obj))
#else
// User-provided function to return a user-friendly ID for a micropython object
// allocated dynamically (used to identify tasks in the print helper)
// Must return 0 if obj is not a valid pointer to a micropython object, either
// allocated within micropython stack or within micropython heap.
mp_int_t mpy_obj_id(mp_const_obj_t obj);
#endif

typedef struct _mp_obj_task_t {
    mp_pairheap_t pairheap;
    mp_obj_t coro;              // Coroutine of this Task
    mp_obj_t data;              // General data for queue it is waiting on
    mp_obj_t state;             // None, False, True, a callable, or a TaskQueue instance
    mp_obj_t ph_key;
} mp_obj_task_t;

typedef struct _mp_obj_task_queue_t {
    mp_obj_base_t base;
    mp_obj_task_t *heap;
    #if MICROPY_PY_ASYNCIO_TASK_QUEUE_PUSH_CALLBACK
    mp_obj_t push_callback;
    #endif
} mp_obj_task_queue_t;

typedef struct _mp_ioqueue_entry_t {
    union {
        mp_obj_t as_array[2];
        struct {
            mp_obj_t read;
            mp_obj_t write;
        } _for;
    } task_waiting;
    mp_obj_t stream;
} mp_ioqueue_entry_t;

typedef struct _mp_obj_ioqueue_t {
    mp_obj_base_t base;
    mp_obj_t poller;
    mp_uint_t alloc_items : 16;
    mp_uint_t used_items : 16;
    mp_ioqueue_entry_t *items;
} mp_obj_ioqueue_t;

extern const mp_obj_type_t mp_type_task_queue;
extern const mp_obj_type_t mp_type_task;
extern const mp_obj_type_t mp_type_ioqueue;

extern mp_obj_t mp_asyncio_context;

#endif // MICROPY_INCLUDED_MODASYNCIO_H
