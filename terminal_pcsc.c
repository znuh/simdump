
/*  Copyright (C) 2011 Matthias Fassl <mf@x1598.at> 
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
#include <winscard.h>
#include "terminal.h"
#include "utils.h"

#define CHECK(f, rv) \
 if (SCARD_S_SUCCESS != rv) \
 { \
  printf(f ": %s\n", pcsc_stringify_error(rv)); \
 }

LONG rv; 

SCARDCONTEXT hContext;
LPTSTR mszReaders;
SCARDHANDLE hCard;
DWORD dwReaders, dwActiveProtocol, dwRecvLength;

SCARD_IO_REQUEST pioSendPci;
BYTE pbRecvBuffer[256];



int term_pcsc_init(char *dev, int debug)
{
	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	CHECK("SCardEstablishContext", rv) 
	#ifdef SCARD_AUTOALLOCATE
	dwReaders = SCARD_AUTOALLOCATE;

	rv = SCardListReaders(hContext, NULL, (LPTSTR)&mszReaders, &dwReaders);
	CHECK("SCardListReaders", rv) 
	#else
	rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
	CHECK("SCardListReaders", rv) 
	mszReaders = calloc(dwReaders, sizeof(char));
	rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
	CHECK("SCardListReaders", rv) 
	#endif

	printf("reader name: %s\n", mszReaders);

	rv = SCardConnect(hContext, mszReaders, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
	CHECK("SCardConnect", rv)

	switch(dwActiveProtocol)
	{
		case SCARD_PROTOCOL_T0:
			pioSendPci = *SCARD_PCI_T0;
			break;

		case SCARD_PROTOCOL_T1:
			pioSendPci = *SCARD_PCI_T1;
			break;
	}
	if(rv == SCARD_S_SUCCESS) {
		return 1;
	}
	return 0;
}

void term_pcsc_close()
{
	rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
	CHECK("SCardDisconnect", rv)

	#ifdef SCARD_AUTOALLOCATE
	rv = SCardFreeMemory(hContext, mszReaders);
	CHECK("SCardFreeMemory", rv)

	#else
	free(mszReaders);
	#endif

	rv = SCardReleaseContext(hContext);

	CHECK("SCardReleaseContext", rv)
}

int term_pcsc_reset(uint8_t * atr, int maxlen)
{
	DWORD dwState, dwAtrLen, dwReaderLen;
    	BYTE pbAtr[64];
	// TODO: return full ATR?
    	rv = SCardReconnect(hCard, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,  SCARD_RESET_CARD, &dwActiveProtocol);
    	rv = SCardStatus(hCard,NULL, &dwReaderLen, &dwState, &dwActiveProtocol, pbAtr, &dwAtrLen);

    	if ( rv == SCARD_S_SUCCESS ) {
        	memcpy(atr, pbAtr, maxlen);
		printf("ATR: ");
		dump_hex(atr,dwAtrLen);
        	return 1;
    	}
	return 0;
}

int term_pcsc_apdu(apdu_t * apdu)
{
	BYTE data[256];
	int sendLength = 5+apdu->p3;

	//build data to send from apdu
	data[0] = apdu->cla;
	data[1] = apdu->ins;
	data[2] = apdu->p1;
	data[3] = apdu->p2;
	data[4] = apdu->p3;
	if(apdu->p3) {
		memcpy(&data[5],apdu->dout,apdu->p3);
	}
	//send data
	dwRecvLength = sizeof(pbRecvBuffer);
	dump_hex(data,sendLength);
	rv = SCardTransmit(hCard, &pioSendPci, data, sendLength, NULL, pbRecvBuffer, &dwRecvLength);
	CHECK("SCardTransmit", rv)
	printf("Receive Length: %d\n",(int) dwRecvLength);
	// ugly hack
	apdu->sw[0] = pbRecvBuffer[0];
	apdu->sw[1] = pbRecvBuffer[1];
	if(apdu->sw[0] == 0x90 && apdu->sw[1] == 0x00) {
		return 0;
	}
	return 1;
}

int term_pcsc_pps(uint8_t *obuf, uint8_t *ibuf) {
	return 0;
}

terminal_t pcsc_term = {
	.name = "pcsc",
	.init = term_pcsc_init,
	.close = term_pcsc_close,
	.reset = term_pcsc_reset,
	.apdu = term_pcsc_apdu,
	.pps = term_pcsc_pps
};
