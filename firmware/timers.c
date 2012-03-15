/* Copyright (c) 2011 Wojciech Bober
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. 
*/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include "timers.h"

void	timer_interrupt();

static volatile timer_t timers[TIMERS_MAX];

volatile clock_t clock;
volatile uint8_t irq;

ISR(TIMER1_COMPA_vect) 
{
	timer_interrupt();
}

/*!
	Initalize timers to default values and registeres
	BH_TIMER bottom half handler
 */
void timer_init()
{
	ptimer_t t;

    TCCR1B = _BV(WGM12) | _BV(CS10) | _BV(CS11);
	OCR1A = 125;

	TIMSK |= _BV(OCIE1A);
    	
	for (t = (ptimer_t)timers; t < timers + TIMERS_MAX; ++t) {
        t->flags = 0;
		t->ticks = 0;
        t->counter = 0;
        t->pdata = NULL;
		t->callback = NULL;
	}

    clock.s = 0;
    clock.ms = 0;
}

/*!
	Timers module interrupt handler.

	Decrease all timers' tick counter by one. Activates BH_TIMER bottom half handler
 */
void timer_interrupt()
{
    ptimer_t t;

    for (t = (ptimer_t)timers; t < timers + TIMERS_MAX; t++) {
		if ((t->flags & _BV(TIMER_ENABLED)) && t->counter)
			t->counter--;
    }

    if (clock.ms++ == 1000) {
        clock.s++;
        clock.ms = 0;
    }

	irq = 1;	
}

/*!
	Bottom half handler for timer interrupt.

	If timer's tick count is zero then function calls 
    callback function associated with timer. 
    
    Function disables timer after it reaches zero.
 */
void timer_update(void) 
{
	ptimer_t t;

	if (irq > 0) {
		for (t = (ptimer_t)timers; t < timers + TIMERS_MAX; t++) {
			if ((t->flags & _BV(TIMER_ENABLED)) && !t->counter) {
	            if (t->flags & _BV(TIMER_AUTO))
	                t->counter = t->ticks;
	            else
	                t->flags &= ~_BV(TIMER_ENABLED);
				t->callback((uint8_t)(t - timers));
			}
	    }
		irq = 0;
	}
}

/*!
	Add new timer.

	The timer is automatically enabled.

	\param ticks ticks ticks to trigger timer
	\param callback function which should be called when timer reaches zero

	\return id of timer
	\return TIMERS_NO_FREE when no free timers present
 */
uint8_t timer_add(timer_callback_t callback) 
{
	ptimer_t t;

	for (t = (ptimer_t)timers; t < timers + TIMERS_MAX; ++t) {
		if (!(t->flags & _BV(TIMER_VALID))) {
            t->flags |= _BV(TIMER_VALID);
			t->ticks = 0;
            t->counter = 0;
			t->callback = callback;
            t->pdata = NULL;
			return (uint8_t)(t - timers);
		}
	}

	return t < timers + TIMERS_MAX ? TIMERS_OK : TIMERS_NO_FREE;
	
}

/*!
	Set timer ticks value.

	Timer is automatically enabled.

	\param ticks to trigger the timer
	\id timer id

	\return TIMERS_OK value set and timer enabled
	\return TIMERS_ERROR no timer with given id
 */
uint8_t timer_set(uint8_t id, uint16_t ticks, uint8_t flags, void *pdata) 
{
	if (id < TIMERS_MAX && (timers[id].flags & _BV(TIMER_VALID))) {
		timers[id].ticks = ticks;
		timers[id].counter = ticks;
        timers[id].flags = flags | _BV(TIMER_VALID);
        timers[id].pdata = pdata;
 		return TIMERS_OK;
	}
	return TIMERS_ERROR;
}

/*!
	Disables timer.

	\param id timer id
	
	\return TIMERS_OK timer disabled
	\return TIMERS_ERROR no timer with given id
 */
uint8_t timer_remove(uint8_t id) 
{
	if (id < TIMERS_MAX) {
        timers[id].flags &= ~(_BV(TIMER_VALID) | _BV(TIMER_ENABLED));
		timers[id].ticks = 0;
        timers[id].counter = 0;
		return TIMERS_OK;
	}
	return TIMERS_ERROR;
}

/*!
    Enable timer.

    \param id timer id

    \return TIMERS_OK timer enabled
    \retunr TIMERS_ERROR no timer with given id
*/

uint8_t timer_enable(uint8_t id)
{
    if (id < TIMERS_MAX && timers[id].ticks) {
        timers[id].flags |= _BV(TIMER_ENABLED);
        timers[id].counter = timers[id].ticks;
        return TIMERS_OK;
    }
    return TIMERS_ERROR;
}

uint8_t timer_disable(uint8_t id)
{
    if (id < TIMERS_MAX) {
        timers[id].flags &= ~_BV(TIMER_ENABLED);
        return TIMERS_OK;
    }
    return TIMERS_ERROR;
}


/*!
    Return user data associated with the timer.

    /param id timer id

    /return pointer to user data
 */
void*   timer_get_pdata(uint8_t id)
{
    if (id < TIMERS_MAX) {
        return timers[id].pdata;
    }
    
    return NULL;
}

