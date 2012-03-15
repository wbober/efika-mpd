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

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <libmpd/libmpd.h>
#include <libmpd/debug_printf.h>
#include <pwd.h>
#include "efika.h"
#include "simclist.h"

extern int debug_level;
char update;

static map_entry_t remote_key_map[] = {
	{70, CMD_PLAY},
	{77, CMD_NEXT},
	{78, CMD_PREV},
	{106, CMD_UP},
	{107, CMD_DOWN},
	{113, CMD_AUX},
	{114, CMD_TUNER},
	{66, CMD_STDBY},
	{72, CMD_MENU},
	{91, CMD_1},
	{92, CMD_2},
	{93, CMD_3},
	{94, CMD_4},
	{95, CMD_5},
	{96, CMD_6},
	{97, CMD_7},
	{98, CMD_8},
	{99, CMD_9},
	{100, CMD_0},
	{76, CMD_DIM},
	{75, CMD_BRIGHT},
	{121, CMD_INFO},
	{87, CMD_SELECT}
};

static efikampd_ctx_t ctx;
static menu_t *top_menu;
static FILE *dbg_file;

void lcd_set_flags(efikampd_ctx_t *ctx, int flags);
void lcd_clear(efikampd_ctx_t *ctx);

void dbg_printf(const char *format, ...)
{
	char buf[256];
	va_list args;
	int len;

	va_start(args, format);
	len = vsnprintf(buf, sizeof(buf), format, args);
	buf[sizeof(buf) - 1] = '\0';
	va_end(args);
	fprintf(dbg_file, "%s", buf);
}

void error_callback(MpdObj *mi,int errorid, char *msg, void *userdata) {
	dbg_printf("Error %i: '%s'\n", errorid, msg);
} 


menu_t * menu_create_entry(const char *text, const char *file, menu_handler_t hs, menu_handler_t hp) {
	menu_t *p = (menu_t *)calloc(1, sizeof(menu_t));
	if (file) {
		p->file = (char *)malloc(strlen(file) + 1);
		memcpy(p->file, file, strlen(file) + 1);
	}
	if (text) {
		p->text = (char *)malloc(strlen(text) + 1);
		memcpy(p->text, text, strlen(text) + 1);
	}
	p->handler_play = hp;
	p->handler_select = hs;
	return p;
}

menu_t *menu_free(menu_t *menu) {
	menu_t *p = menu;
	menu_t *next;

	next = menu->next;
	p = menu->child;
	while (p) {
		p = menu_free(p);
	}
	
	free(menu->text);
	if (menu->file) {
		free(menu->file);
	}
	free(menu);
	return next;
}

int menu_select_song(efikampd_ctx_t *ctx) 
{
	mpd_playlist_clear(ctx->mi);
	mpd_playlist_queue_add(ctx->mi, ctx->pmenu->file);
	mpd_playlist_queue_commit(ctx->mi);
	mpd_player_play(ctx->mi);
	ctx->state = STATE_PLAY;
	
	return 0;
}

int menu_play_album(efikampd_ctx_t *ctx) 
{
	MpdData* data = NULL;
	
	if (ctx->pmenu && ctx->pmenu->parent) {
		mpd_database_search_start(ctx->mi, TRUE);
		mpd_database_search_add_constraint(ctx->mi, MPD_TAG_ITEM_ARTIST, ctx->pmenu->parent->text);
		mpd_database_search_add_constraint(ctx->mi, MPD_TAG_ITEM_ALBUM, ctx->pmenu->text);
		data = mpd_database_search_commit(ctx->mi);	
		
		mpd_playlist_clear(ctx->mi);
		while (data != NULL) {
			mpd_playlist_queue_add(ctx->mi, data->song->file);
			data = mpd_data_get_next(data);
		}
		mpd_playlist_queue_commit(ctx->mi);
		mpd_player_play(ctx->mi);
		ctx->state = STATE_PLAY;		
	}
	
	return 0;
}

void menu_populate(menu_t *pmenu, MpdData* data, menu_handler_t hs, menu_handler_t hp, int sort) 
{
	char buffer[LINE_LENGTH];
	menu_t *head = NULL, *p = NULL, *n = NULL;
	
	// create the guard element
	head = menu_create_entry(NULL, NULL, NULL, NULL);
	p = head;
	
	while (data != NULL) {
		if (data->type == MPD_DATA_TYPE_TAG) {
			n = menu_create_entry(data->tag, NULL, hs, hp);
		} else if (data->type == MPD_DATA_TYPE_SONG) {
			snprintf(buffer, LINE_LENGTH, "%s. %s", data->song->track, data->song->title);
			n = menu_create_entry(buffer, data->song->file, hs, hp);
		}

		p = head;
		if (sort) {
			while(p->text && strcmp(p->text, n->text) < 0) {
				p = p->next;
			}
			if (p->prev) {
				p->prev->next = n;
			}
			n->prev = p->prev;
		}
		p->prev = n;
		n->next = p;
		n->parent = pmenu;
		if (p == head) {
			head = n;
		}
		data = mpd_data_get_next(data);
	}
	
	// remove the guard element
	p = head;
	while (p->text)
		p = p->next;
	p->prev->next = NULL;
	menu_free(p);
	
	pmenu->child = head;
}

int menu_fetch_songs(efikampd_ctx_t *ctx) 
{
	MpdData* data = NULL;
	mpd_database_search_start(ctx->mi, TRUE);
	mpd_database_search_add_constraint(ctx->mi, MPD_TAG_ITEM_ARTIST, ctx->pmenu->parent->text);
	mpd_database_search_add_constraint(ctx->mi, MPD_TAG_ITEM_ALBUM, ctx->pmenu->text);
	data = mpd_database_search_commit(ctx->mi);
	menu_populate(ctx->pmenu, data, menu_select_song, menu_select_song, 0);
	
	return 1;
}

int menu_fetch_albums(efikampd_ctx_t *ctx) 
{
	MpdData* data = NULL;
		
	// fetch only if albums list is empty
	if (ctx->pmenu->child != NULL)
		return 1;		
	
	dbg_printf("Albums of: %s\n" , ctx->pmenu->text);
	data = mpd_database_get_albums(ctx->mi, ctx->pmenu->text);
	menu_populate(ctx->pmenu, data, menu_fetch_songs, menu_play_album, 0);
	
	return 1;
}

int menu_fetch_artists(efikampd_ctx_t *ctx) 
{
	MpdData* data = NULL;

	// fetch only if list is empty
	if (ctx->pmenu->child != NULL)
		return 1;		
		
	data = mpd_database_get_artists(ctx->mi);
	menu_populate(ctx->pmenu, data, menu_fetch_albums, NULL, 1);
	
	return 1;
}

int menu_fetch_radio(efikampd_ctx_t *ctx) 
{
	FILE *fp = NULL;
	char line[255];
	char *stream_name;
	char *stream_path;
	menu_t *p = NULL, *n = NULL;
	
	if (ctx->pmenu->child != NULL)
		return 1;		
		
	fp = fopen(RADIO_LIST_FILE_NAME, "r");
	if (fp == NULL) {
		dbg_printf("Could not open %s file", RADIO_LIST_FILE_NAME);
		return 0;
	}
		
	dbg_printf("Reading radio stations from %s\n", RADIO_LIST_FILE_NAME);
	
	while (fgets(line, sizeof(line), fp) != NULL) {
		// get rid of \n character
		line[strlen(line) - 1] = 0;
		stream_name = strtok(line, ",");
		stream_path = strtok(NULL, ",");
		if (stream_name && stream_path) {
			dbg_printf("Adding: \"%s\" as \"%s\"\n", stream_path, stream_name);
			n = menu_create_entry(stream_name, stream_path, menu_select_song, menu_select_song);
			n->parent = ctx->pmenu;
			if (p) {
				n->prev = p;
				p->next = n;
			}
			p = n;
		}
	}
	
	while (p->prev != NULL) {
		p = p->prev;
	}
	
	ctx->pmenu->child = p;
	fclose(fp);
	
	return 1;
}

void menu_select(efikampd_ctx_t *ctx) {
	lcd_clear(ctx);
	if (ctx->pmenu->handler_select) {
		if (ctx->pmenu->handler_select(ctx)) {
			ctx->pmenu = ctx->pmenu->child;
		} else {
			ctx->pmenu = NULL;
		}
	}
	
	ctx->lcd.marker_pos = 0;
	if (ctx->pmenu == NULL) {
		//lcd_set_flags(ctx, 1);
	}
}

void menu_play(efikampd_ctx_t *ctx) 
{
	lcd_clear(ctx);
	if (ctx->pmenu->handler_play) {
		if (!ctx->pmenu->handler_play(ctx)) {
			ctx->pmenu = NULL;
		}
	}
	ctx->lcd.marker_pos = 0;
	if (ctx->pmenu == NULL) {
		//lcd_set_flags(ctx, 0);
	}	
}

menu_t *menu_init() {
	menu_t *head, *n;
	head = menu_create_entry("Library", NULL, menu_fetch_artists, NULL);
	n = menu_create_entry("Radio", NULL, menu_fetch_radio, NULL);
	head->next = n;
	n->prev = head;
	return head;
}

void lcd_set_flags(efikampd_ctx_t *ctx, int flags) 
{
	ctx->lcd.mode_flags = flags;
	ctx->lcd.update_flags |= BV(LCD_UPDATE_FLAGS);
}

void lcd_clear(efikampd_ctx_t *ctx) 
{
	memset(ctx->lcd.buffer[0], 32, LINE_LENGTH);
	memset(ctx->lcd.buffer[1], 32, LINE_LENGTH);
	ctx->lcd.update_flags = BV(LCD_UPDATE_LINE1) | BV(LCD_UPDATE_LINE2);
}

void lcd_update_status(efikampd_ctx_t *ctx) {
	switch (ctx->state) {	
		case STATE_PLAY: {
			mpd_Song *song = mpd_playlist_get_current_song(ctx->mi);
			if (song) {
				snprintf(ctx->lcd.buffer[0], LINE_LENGTH, "%s", song->title ? song->title : song->name);
				snprintf(ctx->lcd.buffer[1], LINE_LENGTH, "%i/%i %02i:%02i %i",
					song->pos + 1,
					mpd_playlist_get_playlist_length(ctx->mi),
					mpd_status_get_elapsed_song_time(ctx->mi)/60,
					mpd_status_get_elapsed_song_time(ctx->mi)%60,
					mpd_status_get_bitrate(ctx->mi));		

			}		
		}
		break;	
		
		case STATE_PAUSE:
			snprintf(ctx->lcd.buffer[1], LINE_LENGTH, "    -- PAUSE --");
		break;

		case STATE_IDLE:		
		case STATE_STOPPED:
			snprintf(ctx->lcd.buffer[0], LINE_LENGTH, "  Efika MPD Player  ");
			memset(ctx->lcd.buffer[1], 32, LINE_LENGTH);
			break;
			
		case STATE_STDBY:
			memset(ctx->lcd.buffer[0], 32, LINE_LENGTH);
			snprintf(ctx->lcd.buffer[1], LINE_LENGTH, "       %02d:%02d  ", ctx->time.hrs, ctx->time.min);
			break;
			
		default:
			memset(ctx->lcd.buffer[0], 32, LINE_LENGTH);
			break;			
	}
	ctx->lcd.update_flags |= BV(LCD_UPDATE_LINE2);
	ctx->lcd.update_flags |= BV(LCD_UPDATE_LINE1);
}

void lcd_update(efikampd_ctx_t *ctx) 
{
	if (ctx->pmenu) {
		const char *first = NULL, *second = NULL;
		if (ctx->lcd.marker_pos == 0) {
			first = ctx->pmenu->text;
			second = ctx->pmenu->next ? ctx->pmenu->next->text : NULL;
		} else {
			first = ctx->pmenu->prev->text;
			second = ctx->pmenu->text;		
		}
		snprintf(ctx->lcd.buffer[0], LINE_LENGTH, "%c %s", ctx->lcd.marker_pos == 0 ? 0x1D : 0x20, first);
		if (second)
			snprintf(ctx->lcd.buffer[1], LINE_LENGTH, "%c %s", ctx->lcd.marker_pos == 1 ? 0x1D : 0x20, second);
		else 
			memset(ctx->lcd.buffer[1], 32, LINE_LENGTH);
		ctx->lcd.update_flags = BV(LCD_UPDATE_LINE1) | BV(LCD_UPDATE_LINE2);
	} else {
		lcd_update_status(ctx);	
	}
}

void lcd_scroll(efikampd_ctx_t *ctx) {
}

void update_time(efikampd_ctx_t *ctx, struct tm* tminfo) 
{
	ctx->time.hrs = tminfo->tm_hour;
	ctx->time.min = tminfo->tm_min;
	ctx->time.sec = tminfo->tm_sec;
	lcd_scroll(ctx);
	lcd_update(ctx);
}

void handle_state_change(efikampd_ctx_t *ctx) {
	if (ctx->state != STATE_STDBY) {
		switch (mpd_player_get_state(ctx->mi)) {
			case MPD_PLAYER_PLAY:
				ctx->state = STATE_PLAY;
				dbg_printf("Playing\n");
				break;
			case MPD_PLAYER_PAUSE:
				ctx->state = STATE_PAUSE;
				dbg_printf("Paused\n");
				break;
			case MPD_PLAYER_STOP:
				ctx->state = STATE_STOPPED;
				dbg_printf("Stopped\n");
				break;
			default:
				break;
		}
	}
	lcd_update(ctx);
}

void status_changed(MpdObj *mi, ChangedStatusType event)
{
	if (event & MPD_CST_DATABASE) {
		menu_free(top_menu);
		top_menu = menu_init();
		ctx.pmenu = NULL;
	}

	// do not handle state change if ui menu is active
	if (ctx.pmenu) {
		return;
	}
		
	if (event & MPD_CST_SONGID) {
		lcd_update(&ctx);
	}

	if (event & MPD_CST_STATE) {
		handle_state_change(&ctx);
	}
	
	if (event & MPD_CST_RANDOM) {
		dbg_printf("Random: %s\n", mpd_player_get_random(ctx.mi)? "On":"Off");
	}
	
	if (event & MPD_CST_PLAYLIST) {
		lcd_update(&ctx);	
	}
	
	if (event & MPD_CST_ELAPSED_TIME) {
		lcd_update(&ctx);
	}	
}

cmd_t map_command(map_entry_t map[], size_t map_size, unsigned char code) {
	int i = 0;
	
	while (i < map_size) {
		if (map[i].code == code) {
			return map[i].cmd;
		}
		i += 1;
	}
	
	return CMD_UNKNOWN;
}

int handle_command(efikampd_ctx_t *ctx, cmd_t cmd) 
{
	switch (cmd) {
		case CMD_NEXT:
			if (!ctx->pmenu) {
				mpd_player_next(ctx->mi);
			}
			break;
			
		case CMD_PREV:
			if (!ctx->pmenu) {
				mpd_player_prev(ctx->mi);
			} else {
				if (ctx->pmenu) {
					ctx->lcd.marker_pos = 0;
					ctx->pmenu = ctx->pmenu->parent;
				}
			}
			lcd_update(ctx);
			break;
			
		case CMD_PLAY:
			if (ctx->pmenu) {
				menu_play(ctx);
			} else {
				if (ctx->state != STATE_PLAY)
					mpd_player_play(ctx->mi);
				else 
					mpd_player_pause(ctx->mi);
			}
			break;

		case CMD_TUNER:
		case CMD_STDBY:
			if (ctx->state != STATE_STDBY) {
				ctx->state = STATE_STDBY;
				ctx->pmenu = NULL;
				mpd_player_stop(ctx->mi);
			} else {
				ctx->state = STATE_IDLE;
			}
			lcd_update(ctx);
			break;
			
		
		case CMD_STOP:
			ctx->state = STATE_STOPPED;
			mpd_player_stop(ctx->mi);
			break;	
			
		case CMD_QUIT:
			ctx->run = 0;
			break;
			
		case CMD_RANDOM:
			mpd_player_set_random(ctx->mi, !mpd_player_get_random(ctx->mi));
			break;
			
		case CMD_MENU:
			if (ctx->pmenu != top_menu) {
				ctx->pmenu = top_menu;
				ctx->lcd.marker_pos = 0;
			}
			//lcd_set_flags(ctx, 0);
			lcd_update(ctx);
			break;
		
		case CMD_SELECT:
			if (ctx->pmenu) {
				menu_select(ctx);
				lcd_update(ctx);
			}
			break;
			
		case CMD_UP:
			if (ctx->pmenu && ctx->pmenu->prev) {
				ctx->pmenu = ctx->pmenu->prev;
				if (ctx->lcd.marker_pos == 1) 
					ctx->lcd.marker_pos = 0;
				lcd_update(ctx);
			}
			break;
			
		case CMD_DOWN:
			if (ctx->pmenu && ctx->pmenu->next) {
				ctx->pmenu = ctx->pmenu->next;
				if (ctx->lcd.marker_pos == 0) 
					ctx->lcd.marker_pos = 1;
				lcd_update(ctx);
			}
			break;

		case CMD_DIM:
			ctx->lcd.dim_value = 2;
			ctx->lcd.update_flags = BV(LCD_UPDATE_DIM);
			break;
			
		case CMD_BRIGHT:
			ctx->lcd.dim_value = 0;
			ctx->lcd.update_flags = BV(LCD_UPDATE_DIM);
			
		default:
			break;
	}

	return 1;
}

void loop(efikampd_ctx_t *ctx) 
{
	unsigned char rc5[2];
	struct timeval timeout;
	cmd_t cmd;
	time_t rawtime;
	struct tm* tminfo;
	
	fd_set rdset, wrset;	
	
	timeout.tv_sec = 0;
	timeout.tv_usec = 500;

	ctx->state = STATE_IDLE;
	ctx->run = 1;
	lcd_update(ctx);
	//lcd_set_flags(ctx, BV(LCD_AUTO_SCROLL1));
	
	do {				
		FD_ZERO(&wrset);	
		FD_ZERO(&rdset);
		FD_SET(ctx->tty_fd, &rdset);
		FD_SET(ctx->tty_fd, &wrset);
		
		time(&rawtime);
		tminfo = localtime(&rawtime);
		if (tminfo->tm_sec != ctx->time.sec) {
			update_time(ctx, tminfo);
		}
	
		if (select(FD_SETSIZE, &rdset, &wrset, NULL, &timeout) < 0) {
			dbg_printf("select failed: %d : %s", errno, strerror(errno));
		}

		if (FD_ISSET(ctx->tty_fd, &rdset)) {
			while (read(ctx->tty_fd, &rc5, sizeof(rc5)) > 0) {
				cmd =  map_command(remote_key_map, ARRAY_LENGTH(remote_key_map), rc5[1]);
				if (cmd != CMD_UNKNOWN)  {
					handle_command(ctx, cmd);
				} else {
					dbg_printf("Not recognized: %d %d\n", rc5[0], rc5[1]);
				}
			} 
		}

		if (FD_ISSET(ctx->tty_fd, &wrset)) {
			int line;
			for (line = 0; line < 2; line++) {
				if (ctx->lcd.update_flags & BV(line)) {
					unsigned char cmd = (CMD_LCD_UPDATE << 4) + line;
					write(ctx->tty_fd, &cmd, 1);
					write(ctx->tty_fd, (const void *)ctx->lcd.buffer[line], LINE_LENGTH);
				}
				if (ctx->lcd.update_flags & BV(LCD_UPDATE_DIM)) {
					unsigned char cmd = ((unsigned char)CMD_LCD_DIM << 4);
					write(ctx->tty_fd, &cmd, 1);				
					write(ctx->tty_fd, &ctx->lcd.dim_value, 1);
				}
			}
			ctx->lcd.update_flags = 0;
		}

		mpd_status_update(ctx->mi);
	} while (!usleep(100000) &&  ctx->run);		
	
	menu_free(top_menu);
}

int init_serial_port(const char *path, int boud) {
	struct termios tio;
	
	int tty_fd = open(path, O_RDWR | O_NONBLOCK);      
	if (tty_fd < 0) {
		dbg_printf("Cannot open %s\n", path);
		return -1;
	}

	memset(&tio, 0, sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag=0;
	tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
	tio.c_lflag=0;
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=5;    
	cfsetospeed(&tio, boud);           
	cfsetispeed(&tio, boud);
	tcsetattr(tty_fd, TCSANOW, &tio);
	
	return tty_fd;
}

void init_dbg() 
{
	dbg_file = fopen("efikampd.log", "w");
}

MpdObj *create_mpd_obj(options_t *opts) {
	int iport = 6600;
	opts->mpd_hostname = getenv("MPD_HOST");
	opts->mpd_port = getenv("MPD_PORT");
	opts->mpd_password = getenv("MPD_PASSWORD");	
		
	if (!opts->mpd_hostname) {
		opts->mpd_hostname = "localhost";
	}
	if (opts->mpd_port){
		iport = atoi(opts->mpd_port);
	}
	
	return mpd_new(opts->mpd_hostname, iport, opts->mpd_password); 
}

void init_ctx(options_t *opts) 
{
	ctx.mi = create_mpd_obj(opts);
	top_menu = menu_init();
	ctx.pmenu = NULL;
}

void cleanup(void) {
	dbg_printf("Cleaning up resources\n");
	close(ctx.tty_fd);
	if (dbg_file) {
		fclose(dbg_file);
	}
	mpd_free(ctx.mi);
	dbg_printf("Done.");
}

void handle_term_signal(int signal) {
	dbg_printf("Caught TERM singal. Exiting.\n");
	exit(EXIT_SUCCESS);
}

void parse_config(const char *fname, options_t *opts) 
{
	/*
	FILE *fp = NULL;
	char *key, *value;
	char line[255];
	
	fp = fopen(fname, "r");
	if (!fp) {
		dbg_printf("Could not open config file %s\n", fname);
		exit(EXIT_FAILURE);
	}
	
	while(fgets(line, sizeof(line) != NULL) {
		key = strtok(line, "=");
		value = strtok(NULL, "=");
		if (key && value) {
			if (strstr(key, CONFIG_PORT) == 0) 
		}
	}
	
	fclose(fp);
	*/
}

int change_user(const char *username) 
{
	struct passwd *p = getpwnam(username);
	int result = 0;
	
	if (p != NULL) {
		if (setuid(p->pw_uid) == 0) {
			dbg_printf("Running as user %s\n", username);
			result = 1;
		} else {
			dbg_printf("Could not run as user %s\n", username);
		}
	} else {
		dbg_printf("Could not find the details of user %s\n", username);
		
	}
	
	return result;
}

int main(int argc, char **argv)
{
	int deamonize = 1;
	options_t opts;
	pid_t pid, sid;

	init_dbg();	
	
	if (deamonize) {
		pid = fork();
		if (pid < 0) {
			printf("Could not for a child process!\n");
			exit(EXIT_FAILURE);
		}
		if (pid > 0) {
			printf("Efika MPD daemon (ver. %s) started.\n", VERSION_STR);
			exit(EXIT_SUCCESS);
		}
		
		// Clean up handler is only valid for the child process, 
		// hence it MUST be installed after fork is done.
		atexit(cleanup);
		umask(0);
		signal(SIGTERM, handle_term_signal);
	}

	dbg_printf("Efika MPD daemon (ver. %s) started\n", VERSION_STR);
	ctx.tty_fd = init_serial_port(argv[1], B19200);
	if (ctx.tty_fd < 0) {
		dbg_printf("Could not open terminal at %s!\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	dbg_printf("Serial port %s opened\n");
	
	sid = setsid();
	if (sid < 0) {
		/* Log the failure */
		dbg_printf("Could not assign child pid\n");
		exit(EXIT_FAILURE);
	}
	
	change_user(argv[2]);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);	
	
	init_ctx(&opts);
	
	mpd_signal_connect_error(ctx.mi, (ErrorCallback)error_callback, NULL);
	mpd_signal_connect_status_changed(ctx.mi,(StatusChangedCallback)status_changed, NULL);
	mpd_set_connection_timeout(ctx.mi, 10);

	if (!mpd_connect(ctx.mi)) {
		dbg_printf("Connected to MPD at %s:%s\n", opts.mpd_hostname, opts.mpd_port);
		mpd_send_password(ctx.mi);
		loop(&ctx);
	} else {
		dbg_printf("Cannot connect to %s:%s", opts.mpd_hostname, opts.mpd_port);
	}

	cleanup();
	exit(EXIT_SUCCESS);
}


