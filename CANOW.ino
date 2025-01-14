#include "mcp_can_dfs.h"
#include "mcp_canbus.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include <SPI.h>
#include <WiFi.h>
#include <strings.h>

extern "C" {
/* Speed Optimization */
#ifdef CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED
#undef CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED
#define CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED
#endif

#ifdef CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED
#undef CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED
#define CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED
#endif

#ifdef CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM
#undef CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM
#define CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM 20
#endif

#ifdef CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM
#undef CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM
#define CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM 32
#endif

#ifdef CONFIG_LWIP_TCPIP_RECVMBOX_SIZE
#undef CONFIG_LWIP_TCPIP_RECVMBOX_SIZE
#define CONFIG_LWIP_TCPIP_RECVMBOX_SIZE 16
#endif
}

/* MCP2515 SPI PINS */
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCLK 14
#define HSPI_CS   15
/* Socket RX buffer len */
#define TCP_RX_BUF_LEN 128
/* CAN Messages memory buffer len */
#define CAN_BUFF_LEN 100
uint8_t rxBuff[TCP_RX_BUF_LEN] = { 0 };

typedef struct __attribute((packed)){
  unsigned long id;
  unsigned char ext;
  unsigned char len;
  unsigned char data[8];
} canMsg_t;

/* Wifi Access Point Info */
const char* ssid     = "CANOW";
const char* password = "CANOW1234";

/* TCP Client */
WiFiClient client;

/* Init CAN Library */
MCP_CAN CAN(HSPI_CS);  

void setup() {
  /* Init Serial port */
  Serial.begin(115200);

  /* Start WiFi SoftAP */
  WiFi.softAP(ssid, password);
  delay(100);
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40);
  esp_wifi_config_80211_tx_rate(WIFI_IF_AP, WIFI_PHY_RATE_MCS7_SGI);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  /* Init MCP2515 */
  while (CAN_OK != CAN.begin(CAN_125KBPS))
  {
      Serial.println("CAN BUS FAIL!");
      delay(500);
  }
  Serial.println("CAN BUS OK!");
  Serial.println("CAN Autobaud started...");
  
  uint8_t baudRateTest[] = { CAN_100KBPS, CAN_125KBPS, CAN_200KBPS, CAN_250KBPS, CAN_500KBPS, CAN_1000KBPS };
  while(true)
  {
    for(int i = 0; i < sizeof(baudRateTest); i++)
    {
      /* Change baud */
      while (CAN_OK != CAN.begin(baudRateTest[i])){}
      /* Check if we can receive some messages */
      uint8_t autobaud_counter = 0;
      for(uint8_t i = 0; i < 5; i++){
        if(CAN_MSGAVAIL == CAN.checkReceive())
        {
          autobaud_counter++;
        }
      }
      /* Messages received */
      if(autobaud_counter > 0)
      {
        switch(baudRateTest[i])
        {
          case CAN_100KBPS:
            Serial.println("Baudrate 100Kbps.");
            break;
          case CAN_125KBPS:
            Serial.println("Baudrate 125Kbps.");
            break;
          case CAN_200KBPS:
            Serial.println("Baudrate 200Kbps.");
            break;
          case CAN_250KBPS:
            Serial.println("Baudrate 250Kbps.");
            break;
          case CAN_500KBPS:
            Serial.println("Baudrate 500Kbps.");
            break;
          case CAN_1000KBPS:
            Serial.println("Baudrate 1000Kbps.");
            break;
          default:
            Serial.println("Baudrate unknown.");
            break;
        }
        break;
      }
    }
  }
  

  /* Connect to server */
  while(!client.connect(IPAddress(192,168,4,2), 11666)){
    Serial.println("Connection to host failed");
    delay(1000);
  }
  client.setNoDelay(true);
  Serial.println("Connected to Host");
}

uint16_t captured_n = 0;
canMsg_t canMsg[CAN_BUFF_LEN] = { 0 };
unsigned long currTime = 0;
int rxLen = 0;
void loop() 
{
  /* check if data coming */
  if(CAN_MSGAVAIL == CAN.checkReceive())
  {
      /* Copy msg to buffer */
      if(captured_n < CAN_BUFF_LEN)
      {
        canMsg[captured_n].ext = CAN.isExtendedFrame();
        CAN.readMsgBufID(&canMsg[captured_n].id, &canMsg[captured_n].len, (uint8_t*)&canMsg[captured_n].data);
        captured_n++;
      }
  }
  /* Send captured message to socket */
  if(millis() >= currTime + 50)
  {
    currTime = millis();
    if(client.connected())
    {
      client.write((uint8_t*)canMsg, sizeof(canMsg_t)*captured_n);
      captured_n = 0;

      /* Check incoming data from socket */
      rxLen = client.read(rxBuff, TCP_RX_BUF_LEN);
      if(rxLen > 10)
      {
        uint32_t canid = 0;
        uint8_t ext = 0;
        uint8_t len = 0;
        uint8_t data[8] = { 0 };
        memcpy(&canid, &rxBuff[0], 4);
        memcpy(&ext, &rxBuff[4], 1);
        memcpy(&len, &rxBuff[5], 1);
        memcpy(data, &rxBuff[6], len);
        CAN.sendMsgBuf(canid, ext, len, data);
        /*Serial.printf("Sent message with ID: %d.\tEXT: %d\tLEN: %d\n", canid, ext, len);
        for(int i = 0; i < len; i++)
        {
          Serial.printf("%02X", data[i]);
        }
        Serial.printf("\n");*/
      }
    }
    else
    {
      /* Connect to server */
      while(!client.connect(IPAddress(192,168,4,2), 11666)){
        Serial.println("Connection to host failed");
        delay(1000);
      }
      client.setNoDelay(true);
      Serial.println("Connected to Host");
    }
  } 
}
