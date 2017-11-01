/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 by Sergey Fetisov <fsenok@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * version: 1.0 demo (7.02.2015)
 */

#ifndef SYSTEMTIME
#define SYSTEMTIME

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* general functions */

void    time_init(void);                 /* time module initialization */
int64_t utime(void);                     /* monotonic time with 1 us precision */
int64_t mtime(void);                     /* monotonic time with 1 ms precision */
void    usleep(int us);                  /* sleep to n us */
#define msleep(ms) usleep((ms) * 1000)   /* sleep to n ms */

/* softeare timer types */

typedef struct stmr stmr_t;

typedef void (*stmr_cb_t)(stmr_t *tmr);

#define STMR_ACTIVE 1

struct stmr
{
	uint32_t  period; /* timer period, us. */
	uint32_t  event;  /* the last event, us */
	uint32_t  flags;  /* STMR_XXX */
	void     *data;   /* user data */
	stmr_cb_t proc;   /* timer proc */
	stmr_t   *next;   /* don't touch it */
};

/* softeare timer functions */

void stmr(void);             /* call it periodically */
void stmr_init(stmr_t *tmr); /* init timer and adds to list */
void stmr_add(stmr_t *tmr);  /* adds timer to a timers list */
void stmr_free(stmr_t *tmr); /* remove timer from the list */
void stmr_stop(stmr_t *tmr); /* deactivate timer */
void stmr_run(stmr_t *tmr);  /* activate timer */

#define TIMER_PROC(name, period, active, data) \
void name##_proc(stmr_t *tmr); \
static stmr_t name = \
{ \
	period, 0, active, data, \
	name##_proc, NULL \
}; \
void name##_proc(stmr_t *tmr)

#ifdef __cplusplus
}
#endif

#endif
