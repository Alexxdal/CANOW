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
  Serial.begin(9600);
  while (CAN_OK != CAN.begin(CAN_500KBPS))    // init can bus : baudrate = 500k
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
}

unsigned char buffSend[13] = { 0 };
void loop() {
  unsigned char len = 0;
  unsigned char buf[8];
  if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
  {
      CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
      unsigned long canId = CAN.getCanId();
      Serial.println("-----------------------------");
      Serial.print("Get data from ID: ");
      Serial.println(canId, HEX);

      memcpy(&buffSend[0], &canId, 4);
      memcpy(&buffSend[4], &len, 1);
      memcpy(&buffSend[5], &buf, 8);
      client.write((uint8_t *)buffSend, 13);

      for(int i = 0; i<len; i++)    // print the data
      {
          Serial.print(buf[i], HEX);
          Serial.print("\t");
      }
      Serial.println();
  }
  delay(1000);
}
