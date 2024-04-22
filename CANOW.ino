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
#define CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM 25
#endif

#ifdef CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM
#undef CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM
#define CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM 64
#endif

#ifdef CONFIG_LWIP_TCPIP_RECVMBOX_SIZE
#undef CONFIG_LWIP_TCPIP_RECVMBOX_SIZE
#define CONFIG_LWIP_TCPIP_RECVMBOX_SIZE 1024
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
#define CAN_BUFF_LEN 200
uint8_t rxBuff[TCP_RX_BUF_LEN] = { 0 };

typedef struct __attribute((packed)){
  unsigned int id;
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
unsigned char buffSend[14] = { 0 };
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
        canMsg[captured_n].id = CAN.getCanId();
        canMsg[captured_n].ext = CAN.isExtendedFrame();
        CAN.readMsgBuf(&canMsg[captured_n].len, canMsg[captured_n].data);
        captured_n++;
      }
  }
  /* Send captured message to socket */
  if(millis() >= currTime + 100)
  {
    currTime = millis();
    client.write((uint8_t*)canMsg, sizeof(canMsg_t)*captured_n);
    memset(canMsg, 0x00, sizeof(canMsg_t)*CAN_BUFF_LEN);
    captured_n = 0;
  }
  /* Check incoming data from socket */
  rxLen = client.read(rxBuff, TCP_RX_BUF_LEN);
  if(rxLen > 4)
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
