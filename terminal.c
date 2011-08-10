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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "terminal.h"
#include "utils.h"

extern terminal_t phoenix_term;
extern terminal_t fpga_term;

static terminal_t *term_list[] = {
	&phoenix_term,
//	&fpga_term,
	NULL
};

static terminal_t *term = NULL;

static int term_debug = 0;

static FILE *term_log = NULL;

int term_init(char *dev, int debug, FILE *log)
{
	char buf[32];
	int res = -1, cnt;

	term_log = log;
	if(term_log)
		term_debug = debug;
	
	for (cnt = 0; term_list[cnt]; cnt++) {
		terminal_t *t = term_list[cnt];

		strcpy(buf, t->name);
		strcat(buf, ":");

		if (!(strncmp(buf, dev, strlen(buf)))) {

			if ((res = t->init(dev + strlen(buf), term_debug & DEBUG_TERM)) >= 0) {
				term = t;
				return res;
			}	// try open

		}		// compare term name

	}			// foreach term known

	return -1;
}

void term_close()
{
	assert(term != NULL);
	term->close();
	term = NULL;
}

int term_reset(uint8_t * atr, int maxlen)
{
	assert(term != NULL);
	return term->reset(atr, maxlen);
}

int term_pps(int new_etu) {
	uint8_t pps_obuf[]={0xFF, 0x10, 0x94, 0xFF};
	uint8_t pps_ibuf[4];
	int res=0;

	// disabled due to non-working nature of this code :)
	assert(0);

	pps_obuf[3] = pps_obuf[0] ^ pps_obuf[1] ^ pps_obuf[2];
	
	// TODO: etu divs: 12, 6, 4, 2

	assert(term != NULL);
	res = term->pps(pps_obuf, pps_ibuf);

	
	return res;
}

void dump_apdu(apdu_t *apdu, int res) {
	
	fprintf(term_log, "------\n");
	fprintf(term_log, "APDU %02x %02x | %02x %02x %02x:\n",
		apdu->cla, apdu->ins, apdu->p1, apdu->p2, apdu->p3);
		
	if((apdu->dout) && (apdu->p3)){
		fprintf(term_log, "dout: ");
		
		hexdump(term_log, apdu->dout, apdu->p3, 1);
		
		fprintf(term_log, "\n");
	}
}

int term_apdu(apdu_t * apdu)
{
	int res = 0;
	
	assert(term != NULL);
	
	if(term_debug & DEBUG_APDU)
		dump_apdu(apdu, res);
	
	res = term->apdu(apdu);
		
	if(term_debug & DEBUG_APDU) {
		
		if(res) {
			fprintf(term_log, "APDU ERROR: %d\n",res);
			return res;
		}

		if((apdu->din) && (apdu->p3)){
			fprintf(term_log, "din : ");
			hexdump(term_log, apdu->din, apdu->p3, 1);
			fprintf(term_log, "\n");
		}
		
		fprintf(term_log, "ACK : %02x\nSW  : %02x %02x\n",apdu->ack,apdu->sw[0],apdu->sw[1]);
	}
	
	return res;
}
