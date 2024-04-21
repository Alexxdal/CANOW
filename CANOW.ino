#include "mcp_can_dfs.h"
#include "mcp_canbus.h"
#include <SPI.h>
#include <WiFi.h>
#include <strings.h>

/* Wifi Access Point Info */
const char* ssid     = "CANOW";
const char* password = "CANOW1234";

/* TCP Client */
WiFiClient client;

/* MCP2515 SPI PINS */
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCLK 14
#define HSPI_CS   15

/* Init CAN Library */
MCP_CAN CAN(HSPI_CS);  

void setup() {
  /* Start WiFi SoftAP */
  WiFi.softAP(ssid, password);
  Serial.begin(115200);
  while (CAN_OK != CAN.begin(CAN_125KBPS))    // set 250 at 16mhz so at 8mhz is 500
  {
      Serial.println("CAN BUS FAIL!");
      delay(500);
  }
  Serial.println("CAN BUS OK!");
  /* Connect to server */
  while(!client.connect(IPAddress(192,168,4,2), 11666)){
    Serial.println("Connection to host failed");
    delay(1000);
  }
  Serial.println("Connected to Host");
}

typedef struct __attribute((packed)){
  unsigned long id;
  unsigned char ext;
  unsigned char len;
  unsigned char data[8];
} canMsg_t;

#define BUFF_LEN 200
uint16_t captured_n = 0;
canMsg_t canMsg[BUFF_LEN] = { 0 };

unsigned char buffSend[14] = { 0 };
unsigned char len = 0;
unsigned char buf[8] = { 0 };
unsigned long currTime = 0;
void loop() 
{
  if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
  {
      /* Copy msg to buffer */
      if(captured_n < BUFF_LEN)
      {
        canMsg[captured_n].id = CAN.getCanId();
        canMsg[captured_n].ext = CAN.isExtendedFrame();
        CAN.readMsgBuf(&canMsg[captured_n].len, canMsg[captured_n].data);
        captured_n++;
      }
  }
  if(millis() >= currTime + 1000)
  {
    currTime = millis();
    for(int i = 0; i < captured_n; i++)
    {
      memcpy(&buffSend[0], &canMsg[i].id, 4);
      memcpy(&buffSend[4], &canMsg[i].ext, 1);
      memcpy(&buffSend[5], &canMsg[i].len, 1);
      memcpy(&buffSend[6], canMsg[i].data, 8);
      client.write((uint8_t *)buffSend, 14);
    }
    Serial.printf("Received %d messages.\n", captured_n);
    client.flush();
    memset(canMsg, 0x00, sizeof(canMsg_t)*BUFF_LEN);
    captured_n = 0;
  }
}
