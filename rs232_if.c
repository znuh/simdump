/*  Copyright (C) 2011 znuh <Zn000h AT gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "rs232_if.h"

//#define TERM_DEBUG

int rs232_setbaud(int fd, int baud) {
	int res;
	struct termios termio;

	if ((res = tcgetattr(fd, &termio)) < 0) {
		printf("rs232_setbaud tcgetattr: %d\n", res);
		close(fd);
		return res;
	}

	switch (baud) {
	case 9600:
		termio.c_cflag = B9600;
		break;
	case 19200:
		termio.c_cflag = B19200;
		break;
	case 38400:
		termio.c_cflag = B38400;
		break;
	case 115200:
		termio.c_cflag = B115200;
		break;
	default:
		printf("baud rate not supported: %d\n", baud);
		return -1;
	}

	if ((res = tcsetattr(fd, TCSANOW, &termio)) < 0) {
		printf("rs232_setbaud tcsetattr: %d\n", res);
		return res;
	}

	return res;
	
}

int rs232_init(char *dev, int baud, int parity, int time)
{
	int res, fd = open(dev, O_RDWR);
	struct termios termio;

	// TODO: odd parity?

	if (fd < 0) {
		printf("rs232_init open %s: %d\n", dev, fd);
		return fd;
	}

	if ((res = tcgetattr(fd, &termio)) < 0) {
		printf("rs232_init tcgetattr: %d\n", res);
		close(fd);
		return res;
	}

	switch (baud) {
	case 9600:
		termio.c_cflag = B9600;
		break;
	case 19200:
		termio.c_cflag = B19200;
		break;
	case 38400:
		termio.c_cflag = B38400;
		break;
	case 115200:
		termio.c_cflag = B115200;
		break;
	default:
		printf("baud rate not supported: %d\n", baud);
		return -1;
	}

	termio.c_cflag |= CS8 | CREAD | CLOCAL | CSTOPB | (parity ? PARENB : 0);
	termio.c_iflag = IGNBRK | IGNPAR;
	termio.c_oflag = 0;
	termio.c_lflag = 0;

	termio.c_cc[VMIN] = 0;
	termio.c_cc[VTIME] = time;

	if ((res = tcsetattr(fd, TCSANOW, &termio)) < 0) {
		printf("rs232_init tcsetattr: %d\n", res);
		close(fd);
		return res;
	}

	return fd;
}

int rs232_close(int fd)
{
	close(fd);
	return 0;
}

int rs232_read(int fd, uint8_t * d, int len)
{
	int total = 0, res;

	while ((len > 0) && ((res = read(fd, d, len)) > 0)) {
#ifdef TERM_DEBUG
		printf("read %d\n", res);
#endif
		len -= res;
		d += res;
		total += res;
	}

#ifdef TERM_DEBUG
	if (res < 1)
		printf("read %d\n", res);
#endif

	return (res < 0) ? res : total;
}

int rs232_send(int fd, uint8_t * d, int len)
{
	int total = 0, res;

	while ((len > 0) && ((res = write(fd, d, len)) > 0)) {
#ifdef TERM_DEBUG
		printf("write %d\n", res);
#endif
		total += res;
		len -= res;
		d += res;
	}

#ifdef TERM_DEBUG
	if (res < 1)
		printf("write %d\n", res);
#endif

	if (res >= 0)
		res = total;

	return res;
}

int rs232_set_rts(int fd, int rts)
{
	int status, res;

	if ((res = ioctl(fd, TIOCMGET, &status)) < 0) {
		printf("term_reset ioctl(TIOCMGET): %d\n", res);
		return res;
	}

//	status &= ~TIOCM_DTR;
	
	status &= ~TIOCM_RTS;

	if (rts)
		status |= TIOCM_RTS;

	if ((res = ioctl(fd, TIOCMSET, &status)) < 0) {
		printf("term_reset ioctl(TIOCMSET): %d\n", res);
		return res;
	}

	return 0;
}

int rs232_flush(int fd)
{
	uint8_t foobuf[8];

	// flush buffer
	while (rs232_read(fd, foobuf, 8) > 0) {
	}
	
	return 0;
}
