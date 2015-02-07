/*
 * Copyright (C) 2015 by Sergey Fetisov <fsenok@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
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

// general functions

void    time_init(void);                 // time module initialization
int64_t utime(void);                     // monotonic time with 1 us precision
int64_t mtime(void);                     // monotonic time with 1 ms precision
void    usleep(int us);                  // sleep to n us
#define msleep(ms) usleep((ms) * 1000)   // sleep to n ms

// softeare timer types

typedef struct stmr stmr_t;

typedef void (*stmr_cb_t)(stmr_t *tmr);

#define STMR_ACTIVE 1

struct stmr
{
	uint32_t  period; // timer period, us.
	uint32_t  event;  // the last event, us
	uint32_t  flags;  // STMR_XXX
	void     *data;   // user data
	stmr_cb_t proc;   // timer proc
	stmr_t   *next;   // don't touch it
};

// softeare timer functions

void stmr(void);             // call it periodically
void stmr_init(stmr_t *tmr); // init timer and adds to list
void stmr_add(stmr_t *tmr);  // adds timer to a timers list
void stmr_free(stmr_t *tmr); // remove timer from the list
void stmr_stop(stmr_t *tmr); // deactivate timer
void stmr_run(stmr_t *tmr);  // activate timer

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
