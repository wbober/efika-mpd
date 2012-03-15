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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rc5_x.h"


#define rc5_reset_ctx() { rc5_ctx.r_state = IDLE; \
						  rc5_ctx.r_bit_cnt= 0; \
						  rc5_ctx.r_data = 0; \
						}
#define rc5_reset_timer() {	TCNT0 = RC5_TIMER_VALUE; \
							rc5_ctx.r_tick_cnt = 0; }

uint16_t rc5_data;

static volatile rc5_ctx_t rc5_ctx;
static void rc5_isr(void) __attribute__((always_inline));
static void rc5_timer_isr(void) __attribute__((always_inline));


ISR(TIMER0_OVF_vect)
{
	rc5_timer_isr();
}


ISR(INT0_vect)
{
	rc5_isr();
}

void rc5_timer_isr() {
	rc5_ctx.r_tick_cnt += 1;
	TCNT0 = RC5_TIMER_VALUE;

	// errir condition if signal toogle does not occur 
	// within a certain period of time 
	if (rc5_ctx.r_tick_cnt > RC5_TOGGLE_TIMEOUT) {
		rc5_reset_ctx();
	}
}

void rc5_isr(void) {
	uint8_t ticks = rc5_ctx.r_tick_cnt;

	switch (rc5_ctx.r_state) {
		case IDLE:
			rc5_reset_timer();
			rc5_ctx.r_state = START;
			break;

		case START:
			if (ticks >= RC5_PULSE_DURATION) {
				if (ticks > RC5_START_TRESHOLD) {
					rc5_reset_timer();
					rc5_ctx.r_data <<= 1;
					if (bit_is_clear(PIND, PD2)) {
						rc5_ctx.r_data |= 1;
					}
					rc5_ctx.r_bit_cnt += 1;
				}
				rc5_ctx.r_state = SYNC;
			} else {
				rc5_reset_ctx();
			}			
			break;

		case SYNC:
			if (ticks >= RC5_START_TRESHOLD) {

				rc5_reset_timer();

				rc5_ctx.r_data <<= 1;
				if (bit_is_clear(PIND, PD2)) {
					rc5_ctx.r_data |= 1;
				}

				rc5_ctx.r_bit_cnt += 1;

				if (rc5_ctx.r_bit_cnt == 13) {		
					rc5_data = rc5_ctx.r_data;
					rc5_data |= _BV(RC5_FLAG_VALID);
					
					if (bit_is_set(PIND, PD2)) {
						rc5_reset_ctx();
					} else {
						rc5_ctx.r_state = FINISH;
					}
				}
			}
			break;

		case FINISH:
			rc5_reset_ctx();
			break;
	}
}

void rc5_get(uint16_t data, uint8_t *addr, uint8_t *cmd) 
{
	*addr = (data >> 6) & 0x1F;
	*cmd = (data & 0x3F); 
	if (!(data & _BV(RC5_BIT_S2))) {
		*cmd |= 0x40;
	}
}

void rc5_init() 
{
	PORTD |= _BV(PD2);

    TCCR0 |= _BV(CS01);
	TCNT0 = RC5_TIMER_VALUE;

    TIMSK |= _BV(TOIE0);

    MCUCR |= _BV(ISC00);
    GICR |= _BV(INT0);

	rc5_reset_ctx();
}



