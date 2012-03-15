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

#ifndef _TIMERS_H_
#define _TIMERS_H_

#include <inttypes.h>

#ifndef TIMERS_MAX
#define TIMERS_MAX 		0x0A
#endif

#define TIMERS_OK		0xFF
#define TIMERS_NO_FREE	0xFE
#define TIMERS_ERROR	0xFD

#define TIMER_ENABLED   0
#define TIMER_AUTO      1
#define TIMER_VALID     2

typedef void (*timer_callback_t)(uint8_t) ;

typedef void * ptr_t;

typedef struct {
    uint8_t flags;
    // max timer value
    uint16_t ticks;
    // current timer value
	uint16_t counter;
    // pointer to user data
    void*   pdata;
    // callback function
	timer_callback_t callback;
} timer_t, *ptimer_t;

typedef struct {
    uint32_t s;
    uint16_t ms;
} clock_t;

void	    timer_init();
void    	timer_interrupt()  __attribute__((always_inline));
void 	    timer_update();
uint8_t     timer_add(timer_callback_t callback);
uint8_t     timer_set(uint8_t id, uint16_t ticks, uint8_t flags, void *pdata);
uint8_t     timer_enable(uint8_t id);
uint8_t     timer_disable(uint8_t id);
void*       timer_get_pdata(uint8_t id);
uint8_t     timer_remove(uint8_t id);

extern volatile clock_t clock;


#endif
