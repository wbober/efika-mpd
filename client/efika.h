/* efikampd (simple mpd inteface for Efika 5k2)
 * Copyright (C) 2011 Wojciech Bober <wojciech.bober@gmail.com>
  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef EFIKA_MPD_H

#define BV(m) (1 << m)
#define LINE_LENGTH 40
#define ARRAY_LENGTH(a) (sizeof(a)/sizeof(a[0]))
#define RADIO_LIST_FILE_NAME "radio.csv"

typedef enum {
	LCD_UPDATE_LINE1 = 0,
	LCD_UPDATE_LINE2 = 1,
	LCD_UPDATE_FLAGS = 2,
	LCD_UPDATE_DIM = 3
} lcd_update_flags_t;

typedef enum {
	LCD_AUTO_SCROLL = 0,
} lcd_mode_flags_t;

typedef enum {
	CMD_UNKNOWN = 0,
	CMD_LCD_UPDATE = 1,
	CMD_LCD_FLAGS = 2,
	CMD_LCD_DIM = 3,
	CMD_PLAY,
	CMD_PAUSE,
	CMD_NEXT,
	CMD_PREV,
	CMD_UP,
	CMD_DOWN,
	CMD_AUX,
	CMD_TUNER,
	CMD_STDBY,
	CMD_MENU,
	CMD_1,
	CMD_2,
	CMD_3,
	CMD_4,
	CMD_5,
	CMD_6,
	CMD_7,
	CMD_8,
	CMD_9,
	CMD_0,
	CMD_DIM,
	CMD_BRIGHT,
	CMD_QUIT,
	CMD_STOP,
	CMD_RANDOM,
	CMD_REPEAT,
	CMD_INFO,
	CMD_SELECT
} cmd_t;

typedef struct {
	unsigned char code;
	cmd_t cmd;
} map_entry_t;

typedef enum {
	STATE_STDBY,
	STATE_IDLE,
	STATE_PLAY,
	STATE_PAUSE,
	STATE_STOPPED
} state_t;

typedef struct {
	unsigned char update_flags;
	unsigned char mode_flags;
	unsigned char marker_pos;
	unsigned char dim_value;
	char buffer[2][LINE_LENGTH];
} lcd_t;

struct _ctx_t;
typedef int (*menu_handler_t)(struct _ctx_t *);

typedef struct _menu {
	char *file;
	char *text;
	struct _menu *next;
	struct _menu *prev;
	struct _menu *child;
	struct _menu *parent;
	menu_handler_t handler_select;
	menu_handler_t handler_play;
} menu_t;

typedef struct {
	int hrs;
	int min;
	int sec;
} short_time_t;

typedef struct {
	char *mpd_hostname;
	char *mpd_port;
	char *mpd_password;
	char *username;
	char *serial_path;
	char *serial_baud;
} options_t;

typedef struct _ctx_t {
	state_t state;
	lcd_t lcd;
	char *artist;
	char *album;
	MpdObj *mi;
	menu_t *pmenu;
	int run;
	short_time_t time;
	int tty_fd;
} efikampd_ctx_t;

#endif
