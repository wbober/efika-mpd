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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"
#include "rc5_x.h"
#include "uart.h"
#include "timers.h"
#include "defines.h"

/*
** constant definitions
*/
static const PROGMEM unsigned char custom_chars[] =
{
	0x07, 0x08, 0x13, 0x14, 0x14, 0x13, 0x08, 0x07,
	0x00, 0x10, 0x08, 0x08, 0x08, 0x08, 0x10, 0x00
};// 


struct _lcd {
	struct {
	    char buffer[BUFFER_LENGTH];
		char *sp;
		uint8_t length;
		uint8_t pause_cnt;
	} line[LINE_COUNT];
	uint8_t flags;

} lcd;

uint8_t check_cmd(uint8_t c, uint8_t *cmd, uint8_t *length) {
	switch (HIGH_NIBBLE(c)) {
		case CMD_LCD_UPDATE:
			*cmd = c;
			*length = BUFFER_LENGTH;
			return 1;
		case CMD_LCD_FLAGS:
			*cmd = c;
			*length = 1;
			return 1;
		case CMD_LCD_DIM:
			*cmd = c;
			*length = 1;
			return 1;
	}

	return 0;
}


void lcd_update(uint8_t line_no) {
	uint8_t i = 0;
	char c;
	
	lcd_gotoxy(0, line_no);
	while (i < LINE_LENGTH && (c = *(lcd.line[line_no].sp + i)) != 0) {
		lcd_putc(c);
		i += 1;
	}
	while (i < LINE_LENGTH) {
		lcd_putc((char)32);
		i += 1;
	}
}

void lcd_scroll_cb(uint8_t id) 
{
	uint8_t i = 0;
	
	if (BIT_IS_CLEAR(lcd.flags, LCD_AUTO_SCROLL))
		return;

	for (i = 0; i < LINE_COUNT - 1; i++) {
		if (lcd.line[i].length > LINE_LENGTH) {
			if (lcd.line[i].sp < lcd.line[i].buffer + lcd.line[i].length - LINE_LENGTH)
				lcd.line[i].sp += 1;
			else {
				lcd.line[i].sp = lcd.line[i].buffer;
			}
			lcd_update(i);
		}
	}
}

void uart_timeout_cb(uint8_t id) 
{
	ctx_t *ctx = (ctx_t *)timer_get_pdata(id);
	ctx->cnt = 0;
	ctx->state = ST_IDLE;

	lcd_puts_P("  Efika MPD Player  \n");
	lcd_puts_P("                    ");
}

void debounce_cb(uint8_t id) 
{
	ctx_t *ctx = (ctx_t *)timer_get_pdata(id);
	CLEAR_BIT(ctx->flags, FLAG_DEBOUNCE);
}

void ps_cb(uint8_t id)
{
	PS_PORT &= ~_BV(PS_PIN);
}

void uart_collect(ctx_t *ctx, uint8_t k) 
{
	switch (HIGH_NIBBLE(ctx->cmd)) {
		case CMD_LCD_UPDATE: {
			uint8_t line = LOW_NIBBLE(ctx->cmd);
			lcd.line[line].buffer[BUFFER_LENGTH - ctx->cnt] = k;
			if (ctx->cnt == 1) {
				lcd.line[line].length = strlen(lcd.line[line].buffer);
				lcd.line[line].sp = lcd.line[line].buffer;
				lcd_update(line);
			}
			break;		
		}

		case CMD_LCD_FLAGS: {
			lcd.flags = k;
			break;
		}

		case CMD_LCD_DIM: {
			if (k >=0 && k < 4) {
				lcd_brightness(k);
			}
			break;
		}

		default:
			break;
	}
}

void uart_handle_data(ctx_t *ctx, uint8_t k) {
	switch (ctx->state) {
		case ST_IDLE: {
			if (check_cmd(k, &ctx->cmd, &ctx->cnt)) {
				ctx->state = ST_DATA;
				timer_enable(ctx->uart_timeout_timer);
			}
			break;
		}

		case ST_DATA: {
			if (ctx->cnt > 0 ) {
				uart_collect(ctx, k);
				ctx->cnt -= 1;
				if (ctx->cnt == 0) {
					timer_disable(ctx->uart_timeout_timer);
					ctx->state = ST_IDLE;
				}
				
			}
			break;
		}
	}
}

int main(void)
{
	ctx_t ctx;
    uint8_t i = 0;
	uint8_t addr = 0, code = 0;
	uint16_t c;
  
  	// delay for VFD startup
	for (i =0 ; i< 10; i++) {
		_delay_ms(30);	
	}

	// TXB1040 enable pin and Optocoupler inputs
	DDRD = _BV(DDD4) | _BV(DDD6) | _BV(DDD7);
	PORTD |= _BV(PD4);

	timer_init();
	timer_set(timer_add(lcd_scroll_cb), LCD_SCROLL, _BV(TIMER_ENABLED) | _BV(TIMER_AUTO), NULL);
	ctx.uart_timeout_timer = timer_add(uart_timeout_cb);
	timer_set(ctx.uart_timeout_timer, UART_TIMEOUT, 0, (void *)&ctx);
	ctx.debounce_timer = timer_add(debounce_cb);
	timer_set(ctx.debounce_timer, DEBOUNCE_DURATION, 0, (void *)&ctx);
	ctx.ps_timer = timer_add(ps_cb);
	timer_set(ctx.ps_timer, PS_DURATION, 0, (void *)&ctx);

	rc5_init();	
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU)); 
    lcd_init(LCD_DISP_ON_CURSOR);

	lcd_puts_P("  Efika MPD Player  \n");
	lcd_puts_P("     please wait    ");
	lcd_brightness(2);

	ctx.state = ST_IDLE;
	ctx.flags = 0;
		
	memset(&lcd, 0, sizeof(lcd));
	sei();

    while (1) { 

		if (BIT_IS_SET(rc5_data, RC5_FLAG_VALID)) {
			rc5_get(rc5_data, &addr, &code);

			if (BIT_IS_CLEAR(ctx.flags, FLAG_DEBOUNCE)) {
				uart_putc(addr);
				uart_putc(code);
				SET_BIT(ctx.flags, FLAG_DEBOUNCE);
				timer_enable(ctx.debounce_timer);
			}
			
			if (code == STB_CODE) {
				PS_PORT |= _BV(PS_PIN);
				timer_enable(ctx.ps_timer);
			}
			
			rc5_data = 0;
		}

		c = uart_getc();
        if (!(c & UART_NO_DATA)) {
			uart_handle_data(&ctx, (uint8_t)c & 0xFF);
		}

		timer_update();
	}
}
