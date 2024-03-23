/*
  Code Base from RadioLib: https://github.com/jgromes/RadioLib/tree/master/examples/SX126x

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include "HT_SSD1306Wire.h"

#define LoRa_MOSI 10
#define LoRa_MISO 11
#define LoRa_SCK 9

#define LoRa_nss 8
#define LoRa_dio1 14
#define LoRa_nrst 12
#define LoRa_busy 13

//#define VEXT 21
//#define USERKEY 9
//#define SDA_OLED 4
//#define SCL_OLED 15
//#define RST_OLED 16
//#define LED      10

SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
SX1262 radio = new Module(LoRa_nss, LoRa_dio1, LoRa_nrst, LoRa_busy);

#define SCREEN_REFRESH_RATE_US 100000
int64_t lastRefresh = 0;
String lastPackage = "";
int64_t lastMessageTime = 0;
float lastRssi = 0.0;
float lastSnr = 0.0;

void VextON(void)
{
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  digitalWrite(Vext, HIGH);
}

// Interrupt receive, see:
// https://github.com/jgromes/RadioLib/blob/master/examples/SX126x/SX126x_Receive_Interrupt/SX126x_Receive_Interrupt.ino
// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(Vext, OUTPUT);
  VextON();

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Starting...");
  display.display();

  // Radio
  SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_nss);

  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
}

void loop()
{
  if(receivedFlag) {
    // reset flag
    receivedFlag = false;
    // Read data.
    String str;
    int state = radio.readData(str);

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      lastMessageTime =  esp_timer_get_time();
      lastPackage = str;
      lastRssi = radio.getRSSI();
      lastSnr = radio.getSNR();
      Serial.println(str);
    }
    else if (state == RADIOLIB_ERR_RX_TIMEOUT)
    {
      // timeout occurred while waiting for a packet
      Serial.println(F("timeout!"));
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));
    }
    else
    {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
  }
  // Refresh the screen every now and then.
  int64_t now = esp_timer_get_time();
  if(now - lastRefresh > SCREEN_REFRESH_RATE_US) {
    int timeSinceLast = (now - lastMessageTime) / 1000000;
    display.clear();
    display.drawString(0, 0, lastPackage);
    display.drawString(0, 12, "Time since: " + String(timeSinceLast) + "s");
    display.drawString(0, 24, "Last Rssi:  " + String(lastRssi) + "dBm");
    display.drawString(0, 36, "Last SNR :  " + String(lastSnr) + "dB");
    display.display();
    lastRefresh = now;
  }

}