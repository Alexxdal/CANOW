/* This file is generated by BUSMASTER */
/* VERSION [1.2] */
/* BUSMASTER VERSION [3.2.2] */
/* PROTOCOL [CAN] */

/* Start BUSMASTER include header */
#include <CANIncludes.h>
#include <Windows.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
/* End BUSMASTER include header */
#define DEFAULT_BUFLEN 4096

/* Start BUSMASTER global variable */
WSADATA wsa;
SOCKET s, new_socket;
struct sockaddr_in server, client;
int c;
/* End BUSMASTER global variable */

/* Start BUSMASTER Function Prototype  */
GCC_EXTERN void GCC_EXPORT OnDLL_Load();
GCC_EXTERN void GCC_EXPORT OnDLL_Unload();
GCC_EXTERN void GCC_EXPORT OnMsg_All(STCAN_MSG RxMsg);
GCC_EXTERN void GCC_EXPORT OnBus_Connect();
GCC_EXTERN void GCC_EXPORT OnBus_Disconnect();
GCC_EXTERN void GCC_EXPORT OnTimer_CommandsTimer_100( );
/* End BUSMASTER Function Prototype  */

/* Start BUSMASTER Function Wrapper Prototype  */
/* End BUSMASTER Function Wrapper Prototype  */


/* Start BUSMASTER generated function - OnDLL_Load */
void OnDLL_Load() {
  Trace("\nInitialising Winsock...");
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    Trace("Failed. Error Code : %d", WSAGetLastError());
    return;
  }

  Trace("Initialised.\n");

  // Create a socket
  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
    Trace("Could not create socket : %d", WSAGetLastError());
    return;
  }

  Trace("Socket created.\n");

  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(11666);

  // Bind
  if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
    Trace("Bind failed with error code : %d", WSAGetLastError());
    return;
  }

  Trace("Bind done");

  // Listen to incoming connections
  listen(s, 3);

  // Accept and incoming connection
  Trace("Waiting for incoming connections...");

  c = sizeof(struct sockaddr_in);
  while (1) {
    new_socket = accept(s, (struct sockaddr *)&client, &c);
    if (new_socket == INVALID_SOCKET) {
      Trace("accept failed with error code : %d", WSAGetLastError());
      return;
    }

    Trace("Connection accepted");

    int rxLen;
    char recvbuf[DEFAULT_BUFLEN];
	
	int yes = 1;
	int result = setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(int));    // 1 - on, 0 - off
	#define CANMSG_LEN 14
    do {
      rxLen = recv(new_socket, recvbuf, DEFAULT_BUFLEN, 0);
	  if( rxLen % CANMSG_LEN == 0 )
	  {
		Trace("Received %d messages.", rxLen/CANMSG_LEN);
		/* Received correct buffer len */
		for(int i = 0; i < rxLen; i+=CANMSG_LEN)
		{
			STCAN_MSG canMsg;
			/* Clean */
			memset(&canMsg.data, 0x00, 8);
			/* Primi 4 Byte: ID */
			memcpy(&canMsg.id, &recvbuf[i], 4);
			memcpy(&canMsg.isExtended, &recvbuf[i+4], 1);
			memcpy(&canMsg.dlc, &recvbuf[i+5], 1);
			memcpy(&canMsg.data, &recvbuf[i+6], canMsg.dlc);
			SendMsg(canMsg);
		}
	  }
    } while (rxLen > 0);
    Trace("Connection closed\n");
  }

  closesocket(s);
  WSACleanup();
}
/* End BUSMASTER generated function - OnDLL_Load */


/* Start BUSMASTER generated function - OnDLL_Unload */
void OnDLL_Unload() {
  closesocket(s);
  WSACleanup();
}


/* Start BUSMASTER generated function - OnMsg_All */
void OnMsg_All(STCAN_MSG RxMsg) {
	char msg[CANMSG_LEN] = { 0 };
	memcpy(&msg[0], &RxMsg.id, 4);
	memcpy(&msg[4], &RxMsg.isExtended, 1);
	memcpy(&msg[5], &RxMsg.dlc, 1);
	memcpy(&msg[6], &RxMsg.data, 8);
	send(new_socket, (const char*)&msg, CANMSG_LEN, 0);
} /* End BUSMASTER generated function - OnMsg_All */


/* Start BUSMASTER generated function - OnBus_Connect */
void OnBus_Connect() {

} /* End BUSMASTER generated function - OnBus_Connect */


/* Start BUSMASTER generated function - OnBus_Disconnect */
void OnBus_Disconnect() {

} /* End BUSMASTER generated function - OnBus_Disconnect */


/* Start BUSMASTER generated function - OnTimer_CommandsTimer_100 */
void OnTimer_CommandsTimer_100( )
{
  /*for (msgItem = msgMap.begin(); msgItem != msgMap.end(); msgItem++) {
    message_t msg = msgItem->second;
    STCAN_MSG sMsg;
    sMsg.id = msg.id.id_data;
    sMsg.isExtended = true;
    sMsg.isRtr = false;
    sMsg.dlc = msg.dlc;
    for (uint8_t i = 0; i < sMsg.dlc; i++) {
      sMsg.data[i] = msg.payload[i];
    }
    SendMsg(sMsg);
    Sleep(10);
  }*/
}/* End BUSMASTER generated function - OnTimer_CommandsTimer_100 */
