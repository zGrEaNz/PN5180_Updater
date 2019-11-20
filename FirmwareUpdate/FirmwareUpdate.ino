/*
 Name:    FirmwareUpdate.ino
 Created: 8/8/2019 8:00:00 AM
 Author:  zGrEaNz
*/

#include "PN5180_Firmware.h"
#include "Firmware.h"

#define PN5180_RST_PIN      PB1
#define PN5180_BUSY_PIN     PA4
#define PN5180_REQ_PIN      PA1
#define PN5180_NSS_PIN      PB0

#define STM32_MOSI_PIN      PA7
#define STM32_MISO_PIN      PA6
#define STM32_SCK_PIN       PA5

/* DEFINE FIRMWARE HEADER IN Firmware.h, How To In Readme */

PN5180_Firmware Pn5180(PN5180_RST_PIN, PN5180_BUSY_PIN, PN5180_REQ_PIN, PN5180_NSS_PIN, STM32_MOSI_PIN, STM32_MISO_PIN, STM32_SCK_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200L);

  Pn5180.Begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Press Any Key To Update PN5180 Firmware!");

  Serial.flush();

  while (!Serial.available());

  while (Serial.available()) {
    Serial.read();  
  }

  Serial.flush();

  Pn5180.End();

  Serial.end();

  Serial.begin(115200L);

  Pn5180.Begin();

  if (Pn5180.DoUpdateFirmware((uint8_t*)PN5180_FIRMWARE, PN5180_FIRMWARE_SIZE))
    Serial.println("[Info] Update Success.\n");
  else
    Serial.println("[Error] Update Failed.\n");
}