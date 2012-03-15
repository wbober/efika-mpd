#ifndef DEFINES_H
#define DEFINES_H

#ifndef UART_BAUD_RATE
#define UART_BAUD_RATE 	19200
#endif

#ifndef BUFFER_LENGTH
#define BUFFER_LENGTH	40
#endif

#ifndef LINE_LENGTH
#define LINE_LENGTH		20
#endif

#ifndef LINE_COUNT
#define LINE_COUNT		2
#endif

#ifndef LCD_SCROLL
#define LCD_SCROLL		1000L
#endif

#ifndef UART_TIMEOUT
#define UART_TIMEOUT	1000L
#endif

#ifndef HELLO_STRING
#define HELLO_STRING	"  Efika MPD Player  \n"
#endif

#ifndef DEBOUNCE_DURATION
#define DEBOUNCE_DURATION	200
#endif

#ifndef STB_CODE
#define STB_CODE		66
#endif

#define SET_BIT(p, m) (p |= _BV(m))
#define CLEAR_BIT(p, m) (p &= ~_BV(m))

#define BIT_IS_SET(p, m) ((p & _BV(m)) > 0)
#define BIT_IS_CLEAR(p, m) (!BIT_IS_SET(p, m))

#define PS_PORT	PORTD
#define PS_PIN	PD7
#define PS_DURATION	500

typedef enum {
	LCD_AUTO_SCROLL = 0
} lcd_flags_t;

typedef enum {
	ST_IDLE,
	ST_DATA
} state_t;

typedef enum {
	CMD_UNKNOWN = 0,
	CMD_LCD_UPDATE = 1,
	CMD_LCD_FLAGS = 2,
	CMD_LCD_DIM = 3
} cmd_t;

typedef enum {
	FLAG_DEBOUNCE = 0,
} flags_t;

typedef struct {
	uint8_t flags;
	uint8_t cnt;
	state_t state;
	uint8_t cmd;
	uint8_t uart_timeout_timer;
	uint8_t debounce_timer;
	uint8_t ps_timer;
	uint16_t rc5data;
} ctx_t;

#define HIGH_NIBBLE(n)	(n >> 4)
#define LOW_NIBBLE(n)	(n & 0x0F)

#endif
