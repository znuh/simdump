#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <endian.h>
#include <assert.h>

#include "terminal.h"
#include "utils.h"

extern int class_byte;

int get_status(uint8_t *buf, int len) {
	apdu_t apdu;
	int res;
	
	apdu.cla = class_byte;
	apdu.ins = 0xc0;
	apdu.p1 = apdu.p2 = 0;
	apdu.p3 = len;

	apdu.din = buf;
	apdu.dout = NULL;

	res = term_apdu(&apdu);
	return res;
}

int select_fid(uint16_t fid, uint8_t *resp, int resp_len) {
	apdu_t apdu;
	int res;
	int get_len;
	
	apdu.cla = class_byte;
	apdu.ins = 0xa4;
	apdu.p1 = apdu.p2 = 0;

	// USIM
	if(!class_byte)
		apdu.p2=4;
	
	apdu.p3 = 2;

	fid = htobe16(fid);
	apdu.din = NULL;
	apdu.dout = (uint8_t *) &fid;

	res = term_apdu(&apdu);
	if(res < 0)
		return res;

	if((apdu.sw[0] == 0x94) && (apdu.sw[1] == 0x04))
		return 0;

	if((apdu.sw[0] == 0x6a) && (apdu.sw[1] == 0x82))
		return 0;

	if((apdu.sw[0] != 0x9f) && (apdu.sw[0] != 0x61)) {
		fprintf(stderr, "select FID %02x SW: %02x%02x\n",
		        be16toh(fid),apdu.sw[0],apdu.sw[1]);
		return -1;
	}
	
	get_len = apdu.sw[1];

	if(resp) {
		assert(resp_len >= get_len);
		res = get_status(resp, get_len);
	}

	if(res < 0)
		return res;
	
	return get_len;
}

int read_binary(uint8_t *buf, int len) {
	apdu_t apdu;
	int rcnt=0, res;

	apdu.cla = class_byte;
	apdu.ins = 0xb0;
	apdu.dout = NULL;

	while(len) {
		int chunklen = len > 255 ? 255 : len;

		apdu.p1 = rcnt>>8;
		apdu.p2 = rcnt&0xff;
		apdu.din = buf;
		apdu.p3 = chunklen;

		res = term_apdu(&apdu);
		if(res < 0)
			return res;

		if((apdu.sw[0] != 0x90) || (apdu.sw[1] != 0x00)) {
			fprintf(stderr,"read ofs %d len %d %02x%02x\n",
			        rcnt, chunklen, apdu.sw[0], apdu.sw[1]);
			return -1;
		}
		
		len -= chunklen;
		buf += chunklen;
		rcnt += chunklen;
	}
	
	return rcnt;
}

int read_record(uint8_t *buf, int len) {
	apdu_t apdu;
	int res;
	
	apdu.cla = class_byte;
	apdu.ins = 0xb2;
	apdu.p1 = 0;
	apdu.p2 = 2;
	apdu.p3 = len;

	apdu.din = buf;
	apdu.dout = NULL;

	res = term_apdu(&apdu);
	if(res < 0)
		return res;

	// ok
	if((apdu.sw[0] == 0x90) && (apdu.sw[1] == 0x00))
		return len;

	// last read
	if((apdu.sw[0] == 0x94) && (apdu.sw[1] == 0x02))
		return 0;

	// access conditions
	if((apdu.sw[0] == 0x98) && (apdu.sw[1] == 0x04))
		return 0;
	
//	if(apdu.sw[0] != 0x67) {
		fprintf(stderr,"read rec sw %02x%02x\n",apdu.sw[0],apdu.sw[1]);
		return 0;
//	}

	apdu.p3 = apdu.sw[1];

	assert(len >= apdu.p3);

	res = term_apdu(&apdu);
	if(!res)
		return apdu.p3;
	else
		return res;	
}

int auth(char *pin) {
	uint8_t buf[8];
	apdu_t apdu;
	int res;

	memset(buf, 0xff, sizeof(buf));
	memcpy(buf,pin, strlen(pin));
		
	apdu.cla = class_byte;
	apdu.ins = 0x20;
	apdu.p1 = 0;
	apdu.p2 = 1;
	apdu.p3 = 8;

	apdu.din = NULL;
	apdu.dout = buf;

	res = term_apdu(&apdu);
	if(res < 0)
		return res;

	if((apdu.sw[0] == 0x90) && (apdu.sw[1] == 0))
		return 0;

	return -1;
}

int usim_detect(void) {
	apdu_t apdu;
	uint16_t fid;
	int res;
	
	apdu.cla = 0;
	apdu.ins = 0xa4;
	apdu.p1 = apdu.p2 = 0;
	apdu.p3 = 2;

	fid = htobe16(0x3f00);
	apdu.din = NULL;
	apdu.dout = (uint8_t *) &fid;

	res = term_apdu(&apdu);
	if(res < 0)
		return res;

	if((apdu.sw[0] == 0x6e) && (apdu.sw[1] == 0x00))
		return 0;
	else
		return 1;
}
