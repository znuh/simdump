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
#include <stdint.h>
#include <alloca.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "terminal.h"
#include "rs232_if.h"

static int fd;

static int phoenix_debug = 0;

int term_phoenix_init(char *dev, int debug)
{
	phoenix_debug = debug;
	return (fd = rs232_init(dev, 9600, 1, 5));
}

void term_phoenix_close()
{
	rs232_close(fd);
}

int term_phoenix_reset(uint8_t * atr, int maxlen)
{
	int res;

	rs232_flush(fd);

//	res = rs232_set_dtr(fd, 1);
	res = rs232_set_rts(fd, 1);

	if (res < 0)
		return res;

	// 20ms should do the trick
	usleep(20000);

	res = rs232_set_rts(fd, 0);

	if (res < 0)
		return res;

	return rs232_read(fd, atr, maxlen);
}

int term_phoenix_send(uint8_t * buf, int len)
{
	uint8_t *echobuf = NULL;
	int res;

	assert((echobuf = alloca(len)));

	res = rs232_send(fd, buf, len);

	if (res < 0)
		return res;
	else if (res != len)
		return -EIO;

	// kill the echo!
	//printf("echo\n");
	res = rs232_read(fd, echobuf, len);
	//printf("echo done\n");
	
	if (res < 0)
		return res;
	else if (res != len)
		return -EIO;

	// paranoia mode!
	if (memcmp(buf, echobuf, len))
		return -EIO;

	return 0;
}

int term_phoenix_apdu(apdu_t * apdu)
{
	uint8_t hdr[10];
	uint8_t ack;
	int res;

	// TODO: honor debug
	
	hdr[0] = apdu->cla;
	hdr[1] = apdu->ins;
	hdr[2] = apdu->p1;
	hdr[3] = apdu->p2;
	hdr[4] = apdu->p3;

	// send header
	if ((res = term_phoenix_send(hdr, 5)))
		return res;

	// ACK?
	res = rs232_read(fd, &ack, 1);

	//printf("ack %x %x\n",ack,apdu->ins);

	if (res < 0)
		return res;
	else if (res != 1)
		return -EIO;
	else if (ack != apdu->ins) {
		res = rs232_read(fd, apdu->sw+1, 1);
		if(res != 1)
			return res;
		apdu->sw[0] = ack;
		return 0;
	}

	apdu->ack = ack;

	// data?
	if (apdu->p3) {

		// data out
		if (apdu->dout) {
			if ((res = term_phoenix_send(apdu->dout, apdu->p3)))
				return res;
		}
		// data in
		else if (apdu->din) {
			res = rs232_read(fd, apdu->din, apdu->p3);

			if (res < 0)
				return res;
			else if (res != apdu->p3)
				return -EIO;
		}

	}
	//printf("sw\n");
	// receive SW1, SW2
	res = rs232_read(fd, apdu->sw, 2);

	if (res < 0)
		return res;
	else if (res != 2)
		return -EIO;

	return 0;
}

int term_phoenix_pps(uint8_t *obuf, uint8_t *ibuf) {
//	uint8_t buf[16];
	int res = term_phoenix_send(obuf, 4);
//	assert(res == 4);
	// TODO: switch baudrate
//	res = rs232_setbaud(fd, 115200);
	assert(!res);
	res = rs232_read(fd, obuf, 4);
	printf("%d\n",res);
	return 0;
}

terminal_t phoenix_term = {
	.name = "phoenix",
	.init = term_phoenix_init,
	.close = term_phoenix_close,
	.reset = term_phoenix_reset,
	.apdu = term_phoenix_apdu,
	.pps = term_phoenix_pps
};
