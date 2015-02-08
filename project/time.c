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

#include "time.h"

volatile uint32_t sysTimeTicks;
volatile uint32_t sysTimeDelayCounter;

void RTC_Config(void)
{
  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  /* Allow access to RTC */
  PWR_BackupAccessCmd(ENABLE);
	RCC_LSEConfig(RCC_LSE_ON);
  /* Wait till LSE is ready */  
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {}
  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);
  /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();
	RTC_TimeTypeDef time;
	RTC_TimeStructInit(&time);
  RTC_SetTime(RTC_Format_BCD, &time);
}

void time_init(void)
{
	//RTC_Config();
  if (SysTick_Config(SystemCoreClock / 1000))
    while (1) {} // Capture error
}

void rtctime(int *h, int *m, int *s)
{
	RTC_TimeTypeDef time;
	RTC_GetTime(RTC_Format_BIN, &time);
	if (h != NULL) *h = time.RTC_Hours;
	if (m != NULL) *m = time.RTC_Minutes;
	if (s != NULL) *s = time.RTC_Seconds;
}

volatile int64_t usAddition = 0;

void SysTick_Handler(void)
{
	usAddition += 1000; // +1 ms
}

int64_t utime(void)
{
	static int64_t res;
	register uint32_t ticks;

	// „итаем значение счетчика SysTick->VAL и число 
	// добавочных микросекунд usAddition.
	// ѕоскольку во врем€ чтени€ может сработать прерывание
	// перезагрузки таймера (увеличиваетс€ usAddition), 
	// то делаем чтение в цикле. ”словие выхода из цикла -
	// значение usAddition не изменилось.

	do
	{
		res = usAddition;
		ticks = SysTick->VAL;
	} while (res != usAddition);

	#define ticksPerUs (SystemCoreClock / 1000000)
	res += 1000 - ticks / ticksPerUs;
	#undef ticksPerUs

	return res;
}

int64_t mtime(void)
{
	return utime() / 1000;
}

void usleep(int us)
{
	uint64_t t = utime();
	while (true)
	{
		uint64_t t1 = utime();
		if (t1 - t >= us) break;
		if (t1 < t) break; // overflow
	}
}

void MAKE_Alarm(int seconds)
{
	RTC_AlarmTypeDef RTC_AlarmStructure;
	RTC_TimeTypeDef RTC_TimeStruct;

	RTC_ClearFlag(RTC_FLAG_ALRAF);

	RTC_AlarmCmd(RTC_Alarm_A, DISABLE);

	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);

	RTC_AlarmStructure.RTC_AlarmTime.RTC_H12     = RTC_TimeStruct.RTC_H12;
	RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours   = RTC_TimeStruct.RTC_Hours;
	RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes = RTC_TimeStruct.RTC_Minutes;
	RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds = (RTC_TimeStruct.RTC_Seconds + seconds) % 60;
	RTC_AlarmStructure.RTC_AlarmDateWeekDay = 0x31;
	RTC_AlarmStructure.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_Date;
	RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay | RTC_AlarmMask_Hours | RTC_AlarmMask_Minutes;
	RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &RTC_AlarmStructure);

	RTC_ITConfig(RTC_IT_ALRA, ENABLE);

	RTC_AlarmCmd(RTC_Alarm_A, ENABLE);

	RTC_ClearFlag(RTC_FLAG_ALRAF);
}

void standby(int seconds)
{
	MAKE_Alarm(seconds);
	PWR_EnterSTANDBYMode();
}

static stmr_t *stmrs = NULL;

void stmr(void)
{
	stmr_t *tmr;
	uint32_t time;
	time = utime();
	tmr = stmrs;
	while (tmr != NULL)
	{
		stmr_t *t;
		uint32_t elapsed;
		t = tmr;
		tmr = tmr->next;
		if ((t->flags & STMR_ACTIVE) == 0)
			continue;
		elapsed = time;
		elapsed -= t->event;
		if (elapsed < t->period)
			continue;
		t->proc(t);
		t->event = utime();
	}
}

void stmr_init(stmr_t *tmr)
{
	tmr->period = 0;
	tmr->event = 0;
	tmr->flags = 0;
	tmr->data = NULL;
	tmr->proc = NULL;
	tmr->next = stmrs;
	stmrs = tmr;
}

void stmr_add(stmr_t *tmr)
{
	tmr->next = stmrs;
	stmrs = tmr;
}

void stmr_free(stmr_t *tmr)
{
	stmr_t *t;
	
	if (stmrs == NULL)
		return;

	if (tmr == stmrs)
	{
		stmrs = tmr->next;
		tmr->next = NULL;
		return;
	}

	t = stmrs;
	while (t->next != NULL)
	{
		if (t->next == tmr)
		{
			t->next = tmr->next;
			tmr->next = NULL;
			return;
		}
		t = t->next;
	}
}

void stmr_stop(stmr_t *tmr)
{
	tmr->flags &= ~(uint32_t)STMR_ACTIVE;
}

void stmr_run(stmr_t *tmr)
{
	tmr->flags |= STMR_ACTIVE;
	tmr->event = utime();
}
