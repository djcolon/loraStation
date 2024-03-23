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

String lastPackage = "";
int noReceiveCounter = 0;

void VextON(void)
{
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  digitalWrite(Vext, HIGH);
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
}

void loop()
{
  Serial.print(F("[SX1262] Waiting for incoming transmission ... "));

  // you can receive data as an Arduino String
  // NOTE: receive() is a blocking method!
  //       See example ReceiveInterrupt for details
  //       on non-blocking reception method.
  String str;
  int state = radio.receive(str);

  // you can also receive data as byte array
  /*
    byte byteArr[8];
    int state = radio.receive(byteArr, 8);
  */

  if (state == RADIOLIB_ERR_NONE)
  {
    // packet was successfully received
    noReceiveCounter = 0;
    lastPackage = str;
    digitalWrite(LED, HIGH);
    Serial.println(F("success!"));

    // print the data of the packet
    Serial.print(F("[SX1262] Data:\t\t"));
    Serial.println(str);

    // print the RSSI (Received Signal Strength Indicator)
    // of the last received packet
    Serial.print(F("[SX1262] RSSI:\t\t"));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));

    // print the SNR (Signal-to-Noise Ratio)
    // of the last received packet
    Serial.print(F("[SX1262] SNR:\t\t"));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));
    digitalWrite(LED, LOW);
  }
  else if (state == RADIOLIB_ERR_RX_TIMEOUT)
  {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));
    noReceiveCounter++;
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
  display.clear();
  display.drawString(0, 0, lastPackage + "(" + noReceiveCounter + ")");
  display.display();
}