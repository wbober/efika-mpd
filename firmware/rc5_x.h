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

#ifndef RC5_X
#define RC5

#define RC5_TIMER_VALUE		155	// count 10 ticks of timer for f=100kHz with prescaler = 8
#define RC5_FLAG_VALID		15
#define RC5_FLAG_FLIP		11
#define RC5_BIT_S2			12

#define RC5_PULSE_DURATION	7
#define RC5_START_TRESHOLD	12
#define RC5_TOGGLE_TIMEOUT	20

typedef enum {
	IDLE = 0,
	START,
	SYNC,
	FINISH
} rc5_state_t;

typedef struct {
	rc5_state_t r_state;
	uint8_t r_bit_cnt;
	uint8_t r_tick_cnt;
	uint16_t r_data;
} rc5_ctx_t;

/*
typedef struct {
	uint16_t data;
} rc5_data_t;
*/

/*
extern rc5_data_t rc5_data;
*/

extern uint16_t rc5_data;

void rc5_init();
void rc5_get(uint16_t data, uint8_t *addr, uint8_t *cmd);

#endif
