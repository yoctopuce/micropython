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

#include "py/runtime.h"
#include "py/smallint.h"
#include "py/pairheap.h"
#include "py/mphal.h"
#include "py/stream.h"
#include "extmod/modasyncio.h"
#include "extmod/modselect.h"

#if MICROPY_PY_ASYNCIO

// Used when task cannot be guaranteed to be non-NULL.
#define TASK_PAIRHEAP(task) ((task) ? &(task)->pairheap : NULL)

static mp_obj_t task_queue_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

/******************************************************************************/
// Ticks for task ordering in pairing heap

static mp_obj_t ticks(void) {
    return MP_OBJ_NEW_SMALL_INT(mp_hal_ticks_ms() & (MICROPY_PY_TIME_TICKS_PERIOD - 1));
}

static mp_int_t ticks_diff(mp_obj_t t1_in, mp_obj_t t0_in) {
    mp_uint_t t0 = MP_OBJ_SMALL_INT_VALUE(t0_in);
    mp_uint_t t1 = MP_OBJ_SMALL_INT_VALUE(t1_in);
    mp_int_t diff = ((t1 - t0 + MICROPY_PY_TIME_TICKS_PERIOD / 2) & (MICROPY_PY_TIME_TICKS_PERIOD - 1))
        - MICROPY_PY_TIME_TICKS_PERIOD / 2;
    return diff;
}

static int task_lt(mp_pairheap_t *n1, mp_pairheap_t *n2) {
    mp_obj_task_t *t1 = (mp_obj_task_t *)n1;
    mp_obj_task_t *t2 = (mp_obj_task_t *)n2;
    return MP_OBJ_SMALL_INT_VALUE(ticks_diff(t1->ph_key, t2->ph_key)) < 0;
}

/******************************************************************************/
// TaskQueue class

static mp_obj_t task_queue_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    (void)args;
    mp_arg_check_num(n_args, n_kw, 0, MICROPY_PY_ASYNCIO_TASK_QUEUE_PUSH_CALLBACK ? 1 : 0, false);
    mp_obj_task_queue_t *self = mp_obj_malloc(mp_obj_task_queue_t, type);
    self->heap = (mp_obj_task_t *)mp_pairheap_new(task_lt);
    #if MICROPY_PY_ASYNCIO_TASK_QUEUE_PUSH_CALLBACK
    if (n_args == 1) {
        self->push_callback = args[0];
    } else {
        self->push_callback = MP_OBJ_NULL;
    }
    #endif
    return MP_OBJ_FROM_PTR(self);
}

static void task_queue_print(const mp_print_t* print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;

    mp_printf(print, "<task queue %d>", mpy_obj_id(self_in));
}

static mp_obj_t task_queue_peek(mp_obj_t self_in) {
    mp_obj_task_queue_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->heap == NULL) {
        return mp_const_none;
    } else {
        return MP_OBJ_FROM_PTR(self->heap);
    }
}
static MP_DEFINE_CONST_FUN_OBJ_1(task_queue_peek_obj, task_queue_peek);

static mp_obj_t task_queue_push(size_t n_args, const mp_obj_t *args) {
    mp_obj_task_queue_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_task_t *task = MP_OBJ_TO_PTR(args[1]);
    task->data = mp_const_none;
    if (n_args == 2) {
        task->ph_key = ticks();
    } else {
        assert(mp_obj_is_small_int(args[2]));
        task->ph_key = args[2];
    }
    self->heap = (mp_obj_task_t *)mp_pairheap_push(task_lt, TASK_PAIRHEAP(self->heap), TASK_PAIRHEAP(task));
    #if MICROPY_PY_ASYNCIO_TASK_QUEUE_PUSH_CALLBACK
    if (self->push_callback != MP_OBJ_NULL) {
        mp_call_function_1(self->push_callback, MP_OBJ_NEW_SMALL_INT(0));
    }
    #endif
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(task_queue_push_obj, 2, 3, task_queue_push);

static mp_obj_t task_queue_pop(mp_obj_t self_in) {
    mp_obj_task_queue_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_task_t *head = (mp_obj_task_t *)mp_pairheap_peek(task_lt, &self->heap->pairheap);
    if (head == NULL) {
        mp_raise_msg(&mp_type_IndexError, MP_ERROR_TEXT("empty heap"));
    }
    self->heap = (mp_obj_task_t *)mp_pairheap_pop(task_lt, &self->heap->pairheap);
    return MP_OBJ_FROM_PTR(head);
}
static MP_DEFINE_CONST_FUN_OBJ_1(task_queue_pop_obj, task_queue_pop);

static mp_obj_t task_queue_remove(mp_obj_t self_in, mp_obj_t task_in) {
    mp_obj_task_queue_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_task_t *task = MP_OBJ_TO_PTR(task_in);
    self->heap = (mp_obj_task_t *)mp_pairheap_delete(task_lt, &self->heap->pairheap, &task->pairheap);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(task_queue_remove_obj, task_queue_remove);

static const mp_rom_map_elem_t task_queue_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_peek), MP_ROM_PTR(&task_queue_peek_obj) },
    { MP_ROM_QSTR(MP_QSTR_push), MP_ROM_PTR(&task_queue_push_obj) },
    { MP_ROM_QSTR(MP_QSTR_pop), MP_ROM_PTR(&task_queue_pop_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&task_queue_remove_obj) },
};
static MP_DEFINE_CONST_DICT(task_queue_locals_dict, task_queue_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_task_queue,
    MP_QSTR_TaskQueue,
    MP_TYPE_FLAG_NONE,
    make_new, task_queue_make_new,
    print, task_queue_print,
    locals_dict, &task_queue_locals_dict
    );

/******************************************************************************/
// Task class

// This is the core asyncio context with cur_task, _task_queue and CancelledError.
mp_obj_t mp_asyncio_context = MP_OBJ_NULL;

static mp_obj_t task_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    mp_obj_task_t *self = m_new_obj(mp_obj_task_t);
    self->pairheap.base.type = type;
    mp_pairheap_init_node(task_lt, &self->pairheap);
    self->coro = args[0];
    self->data = mp_const_none;
    self->state = TASK_STATE_RUNNING_NOT_WAITED_ON;
    self->ph_key = MP_OBJ_NEW_SMALL_INT(0);
    if (n_args == 2) {
        mp_asyncio_context = args[1];
    }
    MICROPY_PY_ASYNCIO_TASK_HOOK(self, TASK_HOOK_NEW_TASK);
    return MP_OBJ_FROM_PTR(self);
}

static void task_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_obj_task_t *self = MP_OBJ_TO_PTR(self_in);
    const char *state;
    (void)kind;

    if (TASK_IS_DONE(self)) {
        state = "done";
    } else if (TASK_IS_EXECUTING(self)) {
        state = "running";
    } else {
        state = "pending";
    }
    mp_printf(print, "<task aio%d %s>", mpy_obj_id(self_in), state);
}

static mp_obj_t task_done(mp_obj_t self_in) {
    mp_obj_task_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(TASK_IS_DONE(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(task_done_obj, task_done);

static mp_obj_t task_cancel(mp_obj_t self_in) {
    mp_obj_task_t *self = MP_OBJ_TO_PTR(self_in);
    // Check if task is already finished.
    if (TASK_IS_DONE(self)) {
        return mp_const_false;
    }
    // Can't cancel self (not supported yet).
    mp_obj_t cur_task = mp_obj_dict_get(mp_asyncio_context, MP_OBJ_NEW_QSTR(MP_QSTR_cur_task));
    if (self_in == cur_task) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("can't cancel self"));
    }
    // If Task waits on another task then forward the cancel to the one it's waiting on.
    while (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(mp_obj_get_type(self->data)), MP_OBJ_FROM_PTR(&mp_type_task))) {
        MICROPY_PY_ASYNCIO_TASK_HOOK(self, TASK_HOOK_TASK_CANCELLED);
        self = MP_OBJ_TO_PTR(self->data);
    }
    MICROPY_PY_ASYNCIO_TASK_HOOK(self, TASK_HOOK_TASK_CANCELLED);

    mp_obj_t _task_queue = mp_obj_dict_get(mp_asyncio_context, MP_OBJ_NEW_QSTR(MP_QSTR__task_queue));

    // Reschedule Task as a cancelled task.
    mp_obj_t dest[3];
    mp_load_method_maybe(self->data, MP_QSTR_remove, dest);
    if (dest[0] != MP_OBJ_NULL) {
        // Not on the main running queue, remove the task from the queue it's on.
        dest[2] = MP_OBJ_FROM_PTR(self);
        mp_call_method_n_kw(1, 0, dest);
        // _task_queue.push(self)
        dest[0] = _task_queue;
        dest[1] = MP_OBJ_FROM_PTR(self);
        task_queue_push(2, dest);
    } else if (ticks_diff(self->ph_key, ticks()) > 0) {
        // On the main running queue but scheduled in the future, so bring it forward to now.
        // _task_queue.remove(self)
        task_queue_remove(_task_queue, MP_OBJ_FROM_PTR(self));
        // _task_queue.push(self)
        dest[0] = _task_queue;
        dest[1] = MP_OBJ_FROM_PTR(self);
        task_queue_push(2, dest);
    }

    self->data = mp_obj_dict_get(mp_asyncio_context, MP_OBJ_NEW_QSTR(MP_QSTR_CancelledError));

    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(task_cancel_obj, task_cancel);

static void task_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_task_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        // Load
        if (attr == MP_QSTR_coro) {
            dest[0] = self->coro;
        } else if (attr == MP_QSTR_data) {
            dest[0] = self->data;
        } else if (attr == MP_QSTR_state) {
            dest[0] = self->state;
        } else if (attr == MP_QSTR_done) {
            dest[0] = MP_OBJ_FROM_PTR(&task_done_obj);
            dest[1] = self_in;
        } else if (attr == MP_QSTR_cancel) {
            dest[0] = MP_OBJ_FROM_PTR(&task_cancel_obj);
            dest[1] = self_in;
        } else if (attr == MP_QSTR_ph_key) {
            dest[0] = self->ph_key;
        }
    } else if (dest[1] != MP_OBJ_NULL) {
        // Store
        if (attr == MP_QSTR_data) {
            self->data = dest[1];
            dest[0] = MP_OBJ_NULL;
        } else if (attr == MP_QSTR_state) {
            if (!TASK_IS_DONE(self)) {
                self->state = dest[1];
                if (TASK_IS_DONE(self)) {
                    MICROPY_PY_ASYNCIO_TASK_HOOK(self, TASK_HOOK_TASK_DONE);
                }
            } else {
                self->state = dest[1];
            }
            dest[0] = MP_OBJ_NULL;
        }
    }
}

static mp_obj_t task_getiter(mp_obj_t self_in, mp_obj_iter_buf_t *iter_buf) {
    (void)iter_buf;
    mp_obj_task_t *self = MP_OBJ_TO_PTR(self_in);
    if (TASK_IS_DONE(self)) {
        // Signal that the completed-task has been await'ed on.
        self->state = TASK_STATE_DONE_WAS_WAITED_ON;
    } else if (self->state == TASK_STATE_RUNNING_NOT_WAITED_ON) {
        // Allocate the waiting queue.
        self->state = task_queue_make_new(&mp_type_task_queue, 0, 0, NULL);
    } else if (mp_obj_get_type(self->state) != &mp_type_task_queue) {
        // Task has state used for another purpose, so can't also wait on it.
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("can't wait"));
    }
    return self_in;
}

static mp_obj_t task_iternext(mp_obj_t self_in) {
    mp_obj_task_t *self = MP_OBJ_TO_PTR(self_in);
    if (TASK_IS_DONE(self)) {
        // Task finished, raise return value to caller so it can continue.
        nlr_raise(self->data);
    } else {
        // Put calling task on waiting queue.
        mp_obj_t cur_task = mp_obj_dict_get(mp_asyncio_context, MP_OBJ_NEW_QSTR(MP_QSTR_cur_task));
        mp_obj_t args[2] = { self->state, cur_task };
        task_queue_push(2, args);
        // Set calling task's data to this task that it waits on, to double-link it.
        ((mp_obj_task_t *)MP_OBJ_TO_PTR(cur_task))->data = self_in;
    }
    return mp_const_none;
}

static const mp_getiter_iternext_custom_t task_getiter_iternext = {
    .getiter = task_getiter,
    .iternext = task_iternext,
};

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_task,
    MP_QSTR_Task,
    MP_TYPE_FLAG_ITER_IS_CUSTOM,
    make_new, task_make_new,
    print, task_print,
    attr, task_attr,
    iter, &task_getiter_iternext
    );

/******************************************************************************/
// IOQueue class

static mp_obj_t ioqueue_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    mp_obj_ioqueue_t *self = m_new_obj(mp_obj_ioqueue_t);
    mp_uint_t alloc_items = 8;
    self->base.type = type;
    self->poller = mp_select_poll();
    self->items = m_new(mp_ioqueue_entry_t, alloc_items);
    self->alloc_items = alloc_items;
    self->used_items = 0;
    return MP_OBJ_FROM_PTR(self);
}

static void ioqueue_enqueue(mp_obj_t self_in, mp_obj_t stream, mp_uint_t idx) {
    mp_obj_ioqueue_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t used_items = self->used_items;
    mp_uint_t alloc_items = self->alloc_items;
    mp_ioqueue_entry_t *end = self->items + used_items;
    mp_ioqueue_entry_t *scan = self->items;
    mp_ioqueue_entry_t *emptyEntry = NULL;
    mp_obj_t *cur_task;

    cur_task = mp_obj_dict_get(mp_asyncio_context, MP_OBJ_NEW_QSTR(MP_QSTR_cur_task));
    while (scan < end && scan->stream != stream) {
        if (!emptyEntry && !scan->stream) {
            emptyEntry = scan;
        }
        scan++;
    }
    if (scan >= end) {
        if (emptyEntry) {
            scan = emptyEntry;
        } else if (used_items < alloc_items) {
            self->used_items += 1;
        } else {
            alloc_items += 4;
            self->items = m_renew(mp_ioqueue_entry_t, self->items, used_items, alloc_items);
            self->alloc_items = alloc_items;
            mp_seq_clear(self->items, used_items, alloc_items, sizeof(mp_ioqueue_entry_t));
            scan = self->items + used_items;
            self->used_items += 1;
        }
        scan->stream = stream;
        scan->task_waiting.as_array[idx] = cur_task;
        scan->task_waiting.as_array[1 - idx] = mp_const_none;
        mp_obj_t args[3];
        args[0] = self->poller;
        args[1] = stream;
        args[2] = MP_OBJ_NEW_SMALL_INT(!idx ? MP_STREAM_POLL_RD : MP_STREAM_POLL_WR);
        mp_poll_register(3, args);
    } else {
        assert(scan->task_waiting.as_array[idx] == mp_const_none);
        assert(scan->task_waiting.as_array[1 - idx] != mp_const_none);
        scan->task_waiting.as_array[idx] = cur_task;
        mp_poll_modify(self->poller, stream, MP_OBJ_NEW_SMALL_INT(MP_STREAM_POLL_RD | MP_STREAM_POLL_WR));
    }
    // Link task to this IOQueue so it can be removed if needed
    mp_obj_task_t *cur_task_ptr = MP_OBJ_TO_PTR(cur_task);
    cur_task_ptr->data = self_in;
}

static mp_obj_t ioqueue_queue_read(mp_obj_t self_in, mp_obj_t stream) {
    ioqueue_enqueue(self_in, stream, 0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ioqueue_queue_read_obj, ioqueue_queue_read);

static mp_obj_t ioqueue_queue_write(mp_obj_t self_in, mp_obj_t stream) {
    ioqueue_enqueue(self_in, stream, 1);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ioqueue_queue_write_obj, ioqueue_queue_write);

// dequeue a stream
static mp_obj_t ioqueue_dequeue(mp_obj_t self_in, mp_obj_t stream) {
    mp_obj_ioqueue_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t used_items = self->used_items;
    mp_ioqueue_entry_t *end = self->items + used_items;
    mp_ioqueue_entry_t *scan = self->items;

    while (scan < end && scan->stream != stream) {
        scan++;
    }
    assert(scan < end);
    mp_poll_unregister(self->poller, stream);
    // blank entry
    memset(scan, 0, sizeof(mp_ioqueue_entry_t));
    if (scan == end - 1) {
        // move back end pointer
        do {
            used_items--;
        } while (used_items > 0 && !self->items[used_items - 1].stream);
        self->used_items = used_items;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ioqueue_dequeue_obj, ioqueue_dequeue);

// update poll status or remove entry from ioqueue after dequeuing a task
static void ioqueue_dequeue_ifneeded(mp_obj_t self_in, mp_ioqueue_entry_t *scan) {
    mp_obj_ioqueue_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t poller = self->poller;
    mp_obj_t stream = scan->stream;

    if (scan->task_waiting._for.read == mp_const_none) {
        if (scan->task_waiting._for.write == mp_const_none) {
            mp_poll_unregister(poller, stream);
            // blank entry
            memset(scan, 0, sizeof(mp_ioqueue_entry_t));
            mp_uint_t used_items = self->used_items;
            if (scan == self->items + used_items - 1) {
                // move back end pointer
                do {
                    used_items--;
                } while (used_items > 0 && !self->items[used_items - 1].stream);
                self->used_items = used_items;
            }
        } else {
            mp_poll_modify(poller, stream, MP_OBJ_NEW_SMALL_INT(MP_STREAM_POLL_WR));
        }
    } else {
        assert(scan->task_waiting._for.write == mp_const_none);
        mp_poll_modify(poller, stream, MP_OBJ_NEW_SMALL_INT(MP_STREAM_POLL_RD));
    }
}

static mp_obj_t ioqueue_remove(mp_obj_t self_in, mp_obj_t task) {
    mp_obj_ioqueue_t *self = MP_OBJ_TO_PTR(self_in);
    mp_ioqueue_entry_t *scan = self->items;

    while (scan < self->items + self->used_items) {
        if (scan->stream) {
            mp_uint_t changed = 0;
            if (scan->task_waiting._for.read == task) {
                scan->task_waiting._for.read = mp_const_none;
                changed = 1;
            }
            if (scan->task_waiting._for.write == task) {
                scan->task_waiting._for.write = mp_const_none;
                changed = 1;
            }
            if (changed) {
                ioqueue_dequeue_ifneeded(self_in, scan);
            }
        }
        scan++;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ioqueue_remove_obj, ioqueue_remove);

static mp_obj_t ioqueue_wait_io_event(mp_obj_t self_in, mp_obj_t dt) {
    mp_obj_ioqueue_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t poller = self->poller;
    mp_obj_t args[2];
    mp_obj_t tuple;

    args[0] = poller;
    args[1] = dt;
    poller = mp_poll_ipoll(2, args);
    args[0] = mp_obj_dict_get(mp_asyncio_context, MP_OBJ_NEW_QSTR(MP_QSTR__task_queue));
    tuple = mp_poll_iternext(poller);
    while (tuple != MP_OBJ_STOP_ITERATION) {
        mp_obj_tuple_t *t = MP_OBJ_TO_PTR(tuple);
        mp_obj_t stream = t->items[0];
        mp_uint_t flags = mp_obj_get_int(t->items[1]);
        mp_ioqueue_entry_t *end = self->items + self->used_items;
        mp_ioqueue_entry_t *scan = self->items;
        while (scan < end && scan->stream != stream) {
            scan++;
        }
        assert(scan < end);
        if ((flags & ~MP_STREAM_POLL_WR) != 0 && scan->task_waiting._for.read != mp_const_none) {
            // POLLIN or error
            args[1] = scan->task_waiting._for.read;
            scan->task_waiting._for.read = mp_const_none;
            task_queue_push(2, args);
        }
        if ((flags & ~MP_STREAM_POLL_RD) != 0 && scan->task_waiting._for.write != mp_const_none) {
            // POLLOUT or error
            args[1] = scan->task_waiting._for.write;
            scan->task_waiting._for.write = mp_const_none;
            task_queue_push(2, args);
        }
        ioqueue_dequeue_ifneeded(self_in, scan);
        tuple = mp_poll_iternext(poller);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ioqueue_wait_io_event_obj, ioqueue_wait_io_event);

static void ioqueue_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_ioqueue_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        // Load
        if (attr == MP_QSTR_poller) {
            // poller is a valid attribute, corresponding to the
            // install returned by select.poll to be used
            dest[0] = self->poller;
        } else if (attr == MP_QSTR_map) {
            // map is a valid attribute, which should evaluate
            // to True iff there is a task pending I/O
            dest[0] = (self->used_items ? mp_const_true : mp_const_false);
        } else {
            // not an attribute, trigger lookup in locals_dict
            dest[1] = MP_OBJ_SENTINEL;
        }
    } else if (dest[1] != MP_OBJ_NULL) {
        // Store
        if (attr == MP_QSTR_poller) {
            // success
            self->poller = dest[1];
            dest[0] = MP_OBJ_NULL;
        }
    }
}

static const mp_rom_map_elem_t ioqueue_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_queue_read), MP_ROM_PTR(&ioqueue_queue_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_queue_write), MP_ROM_PTR(&ioqueue_queue_write_obj) },
    { MP_ROM_QSTR(MP_QSTR__dequeue), MP_ROM_PTR(&ioqueue_dequeue_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&ioqueue_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_wait_io_event), MP_ROM_PTR(&ioqueue_wait_io_event_obj) },
};
static MP_DEFINE_CONST_DICT(ioqueue_locals_dict, ioqueue_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ioqueue,
    MP_QSTR_IOQueue,
    MP_TYPE_FLAG_NONE,
    make_new, ioqueue_make_new,
    attr, ioqueue_attr,
    locals_dict, &ioqueue_locals_dict
    );

/******************************************************************************/
// C-level asyncio module

static const mp_rom_map_elem_t mp_module_asyncio_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__asyncio) },
    { MP_ROM_QSTR(MP_QSTR_Task), MP_ROM_PTR(&mp_type_task) },
    { MP_ROM_QSTR(MP_QSTR_TaskQueue), MP_ROM_PTR(&mp_type_task_queue) },
    { MP_ROM_QSTR(MP_QSTR_IOQueue), MP_ROM_PTR(&mp_type_ioqueue) },
};
static MP_DEFINE_CONST_DICT(mp_module_asyncio_globals, mp_module_asyncio_globals_table);

const mp_obj_module_t mp_module_asyncio = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_asyncio_globals,
};

MP_REGISTER_MODULE(MP_QSTR__asyncio, mp_module_asyncio);

#endif // MICROPY_PY_ASYNCIO
