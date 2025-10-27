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

#ifndef MICROPY_INCLUDED_MODSELECT_H
#define MICROPY_INCLUDED_MODSELECT_H

mp_obj_t mp_select_poll(void);
mp_obj_t mp_poll_register(size_t n_args, const mp_obj_t *args);
mp_obj_t mp_poll_unregister(mp_obj_t self_in, mp_obj_t obj_in);
mp_obj_t mp_poll_modify(mp_obj_t self_in, mp_obj_t obj_in, mp_obj_t eventmask_in);
mp_obj_t mp_poll_ipoll(size_t n_args, const mp_obj_t *args);
mp_obj_t mp_poll_iternext(mp_obj_t self_in);

#endif // MICROPY_INCLUDED_MODSELECT_H
