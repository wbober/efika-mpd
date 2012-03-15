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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

void loop(int tty_fd, int stdin_fd) 
{
	int run = 1;
	int n;
	char buffer[20];
	unsigned char rc5[2];
	int result;
	struct timeval timeout;
	
	fd_set rdset, wrset;	
	memset(buffer, '\0', 20);

	timeout.tv_sec = 0;
	timeout.tv_usec = 500;

	do {				
		FD_ZERO(&wrset);	
		FD_ZERO(&rdset);
		FD_SET(stdin_fd, &rdset);
		FD_SET(tty_fd, &rdset);
		FD_SET(tty_fd, &wrset);
	
		if (select(FD_SETSIZE, &rdset, &wrset, NULL, &timeout) < 0) {
			printf("select failed: %d : %s", errno, strerror(errno));
		}
		
		if (FD_ISSET(tty_fd, &rdset)) {
			while (read(tty_fd, &rc5, sizeof(rc5)) > 0) {
				result = rc5[1];
				result <<= 8;
				result += rc5[0];
				//printf("%d %d\n", rc5[0], rc5[1]);
				printf("%d\n", result);
			} 
		}
		
	} while(!usleep(100000) &&  run);		
}

int main(int argc, char **argv)
{
	int stdin_fd = 0;
	int tty_fd = 0;
	struct termios tio;
		
	/* Make the input non blocking */
	stdin_fd = open("/dev/stdin", O_NONBLOCK|O_RDONLY);
	if (stdin_fd < 0) {
		printf("Could not open stdin");
		exit(0);
	}
	
	tty_fd = open(argv[1], O_RDWR | O_NONBLOCK);      
	if (tty_fd < 0) {
		printf("Cannot open %s\n", argv[1]);
		exit(0);
	}
	
	/* Open serial port */
    memset(&tio, 0, sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;    
    cfsetospeed(&tio,B19200);           
    cfsetispeed(&tio,B19200);
    tcsetattr(tty_fd, TCSANOW, &tio);
	
	/* set correct hostname */		
	loop(tty_fd, stdin_fd);

	close(stdin_fd);
	close(tty_fd);
		
	return 1;
}

