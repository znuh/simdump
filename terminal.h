#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdio.h>
#include <errno.h>

#include <stdint.h>

#define DEBUG_APDU		1
#define DEBUG_TERM		2

typedef struct apdu_s {
	uint8_t cla;
	uint8_t ins;
	uint8_t p1;
	uint8_t p2;
	uint8_t p3;
	uint8_t ack;
	uint8_t *dout;
	uint8_t *din;
	uint8_t sw[2];
} apdu_t;

typedef struct terminal_s {

	// name
	const char *name;

	// functions
	int (*init) (char *dev, int debug);
	void (*close) (void);
	int (*reset) (uint8_t * atr, int maxlen);
	int (*apdu) (apdu_t * apu);
	int (*pps) (uint8_t *obuf, uint8_t *ibuf);

	// TODO: features?

	// TODO: private data?
} terminal_t;

int term_init(char *dev, int debug, FILE *log);
void term_close(void);

int term_reset(uint8_t * atr, int maxlen);
int term_apdu(apdu_t * apdu);
int term_pps(int new_etu);

#endif				// TERMINAL_H
