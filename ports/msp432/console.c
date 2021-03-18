/*
 * Copyright (c) 2016-2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== console.c ========
 */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* POSIX Header files */
#include <pthread.h>
#include <semaphore.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#ifdef CC32XX
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>
#endif

/* Example/Board Header files */
#include "Board.h"

#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/stackctrl.h"

/* Used to determine whether to have the thread block */
volatile bool uartEnabled = true;
sem_t semConsole;


/*
 *  ======== gpioButtonFxn ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON1.
 *  There is no debounce logic here since we are just looking for
 *  a button push. The uartEnabled variable protects use against any
 *  additional interrupts cased by the bouncing of the button.
 */
void gpioButtonFxn(uint_least8_t index)
{

    /* If disabled, enable and post the semaphore */
    if (uartEnabled == false) {
        uartEnabled = true;
        sem_post(&semConsole);
    }
}

#if 0

/*
 *  ======== simpleConsole ========
 *  Handle the user input. Currently this console does not handle
 *  user back-spaces or other "hard" characters.
 */
void simpleConsole(UART_Handle uart)
{
    char cmd;
    int status;
    char tempStr[8];
    int localTemperatureC;
    int localTemperatureF;

    UART_write(uart, consoleDisplay, sizeof(consoleDisplay));

    /* Loop until read fails or user quits */
    while (1) {
        UART_write(uart, userPrompt, sizeof(userPrompt));
        status = UART_read(uart, &cmd, sizeof(cmd));
        if (status == 0) {
            UART_write(uart, readErrDisplay, sizeof(readErrDisplay));
            cmd = 'q';
        }

        switch (cmd) {
            case 't':
                UART_write(uart, tempStartDisplay, sizeof(tempStartDisplay));
                /*
                 *  Make sure we are accessing the global float temperature variables
                 *  in a thread-safe manner.
                 */
                pthread_mutex_lock(&temperatureMutex);
                localTemperatureC = (int)temperatureC;
                localTemperatureF = (int)temperatureF;
                pthread_mutex_unlock(&temperatureMutex);

                itoa((int)localTemperatureC, tempStr);
                UART_write(uart, tempStr, strlen(tempStr));
                UART_write(uart, tempMidDisplay, sizeof(tempMidDisplay));
                itoa((int)localTemperatureF, tempStr);
                UART_write(uart, tempStr, strlen(tempStr));
                UART_write(uart, tempEndDisplay, sizeof(tempEndDisplay));
                break;
            case 'c':
                UART_write(uart, cleanDisplay, sizeof(cleanDisplay));
                break;
            case 'q':
                UART_write(uart, byeDisplay, sizeof(byeDisplay));
                return;
            case 'h':
            default:
                UART_write(uart, helpPrompt, sizeof(helpPrompt));
                break;
        }
    }
}

#endif


#if MICROPY_ENABLE_COMPILER
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}
#endif

UART_Handle uart;
void debug_log(const char * line)
{
      UART_write(uart, line, strlen(line));
}


static char heap[16384];

mp_obj_t execute_from_str(const char *str) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        qstr src_name = 1/*MP_QSTR_*/;
        mp_lexer_t *lex = mp_lexer_new_from_str_len(src_name, str, strlen(str), false);
        mp_parse_tree_t pt = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&pt, src_name, MP_EMIT_OPT_NONE, false);
        mp_call_function_0(module_fun);
        nlr_pop();
        return 0;
    } else {
        // uncaught exception
        return (mp_obj_t)nlr.ret_val;
    }
}

static void micro_python(UART_Handle uart) {
    // Initialized stack limit
    mp_stack_set_limit(40000 * (BYTES_PER_WORD / 4));
    // Initialize heap
    gc_init(heap, heap + sizeof(heap));
    // Initialize interpreter
    mp_init();

    const char str[] = "print('Hello world of easy embedding!')";
    if (execute_from_str(str)) {
        debug_log("Error\n");
    }
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

void nlr_jump_fail(void *val) {
    //printf("FATAL: uncaught NLR %p\n", val);
    //exit(1);
    debug_log("FATAL: uncaught NLR\n");
    while(1);
}






