
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

#define CHECK(f, rv) \
 if (SCARD_S_SUCCESS != rv) \
 { \
  printf(f ": %s\n", pcsc_stringify_error(rv)); \
  return -1; \
 }

LONG rv; 

SCARDCONTEXT hContext;
LPTSTR mszReaders;
SCARDHANDLE hCard;
DWORD dwReaders, dwActiveProtocol, dwRecvLength;

SCARD_IO_REQUEST pioSendPci;
BYTE pbRecvBuffer[256];


static int pcsc_debug = 0;

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

	rv = SCardConnect(hContext, mszReaders, SCARD_SHARE_SHARED,
	SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
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
	// SCARD RECONNECT
	/*
	cs_debug("PCSC resetting card in (%s)", pcsc_reader->pcsc_name);
    	rv = SCardReconnect(pcsc_reader->hCard, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,  SCARD_RESET_CARD, &pcsc_reader->dwActiveProtocol);
    	cs_debug("PCSC resetting done on card in (%s)", pcsc_reader->pcsc_name);
    	cs_debug("PCSC Protocol (T=%d)",( pcsc_reader->dwActiveProtocol == SCARD_PROTOCOL_T0 ? 0 :  1));

    	if ( rv != SCARD_S_SUCCESS )  {
       		cs_debug("Error PCSC failed to reset card (%lx)", rv);
        	return(0);
    	}
	*/
}

int term_pcsc_apdu(apdu_t * apdu)
{
	dwRecvLength = sizeof(pbRecvBuffer);
	rv = SCardTransmit(hCard, &pioSendPci, cmd1, sizeof(cmd1),
	NULL, pbRecvBuffer, &dwRecvLength);
	CHECK("SCardTransmit", rv)

	printf("response: ");
	for(i=0; i<dwRecvLength; i++)
		printf("%02X ", pbRecvBuffer[i]);
	printf("\n");
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
