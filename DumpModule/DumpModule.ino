/*
 Name:    DumpModule.ino
 Created: 8/8/2019 8:00:00 AM
 Author:  zGrEaNz
*/

#include "PN5180_Firmware.h"

#define PN5180_RST_PIN      PB1
#define PN5180_BUSY_PIN     PA4
#define PN5180_REQ_PIN      PA1
#define PN5180_NSS_PIN      PB0

#define STM32_MOSI_PIN      PA7
#define STM32_MISO_PIN      PA6
#define STM32_SCK_PIN       PA5

PN5180_Firmware Pn5180(PN5180_RST_PIN, PN5180_BUSY_PIN, PN5180_REQ_PIN, PN5180_NSS_PIN, STM32_MOSI_PIN, STM32_MISO_PIN, STM32_SCK_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200L);

  Pn5180.Begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Press Any Key To Dump PN5180 Info!");

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

  size_t EEPROMReadSize = (PN5180_DL_END_ADDR - PN5180_DL_START_ADDR);

  uint8_t MajorVersion, MinorVersion;

  uint8_t* DieId = (uint8_t*)malloc(PN5180_DL_GETDIEID_DIEID_LEN * sizeof(uint8_t));
  
  uint8_t* EEPROMData = (uint8_t*)malloc(EEPROMReadSize * sizeof(uint8_t));

  Pn5180.GetFirmwareVersion(&MajorVersion, &MinorVersion);
  Pn5180.GetDieID(DieId);

  Serial.print("[Info] Firmware Version "); Serial.print(MajorVersion, HEX);  Serial.print("."); Serial.println(MinorVersion, HEX);
  Serial.print("[Info] DieID : "); Pn5180.PrintHex8(DieId, PN5180_DL_GETDIEID_DIEID_LEN);

  free(DieId);

  PN5180_DOWNLOAD_INTEGRITY_INFO_T IntegrityInfo;

  Pn5180.CheckIntegrity(&IntegrityInfo);

  Serial.print("[Info] Integrity->FunctionCodeOk : "); Serial.println(IntegrityInfo.FunctionCodeOk);
  Serial.print("[Info] Integrity->PatchCodeOk : "); Serial.println(IntegrityInfo.PatchCodeOk);
  Serial.print("[Info] Integrity->PatchTableOk : "); Serial.println(IntegrityInfo.PatchTableOk);
  Serial.print("[Info] Integrity->UserDataOk : "); Serial.println(IntegrityInfo.UserDataOk);

  PN5180_DOWNLOAD_SESSION_STATE_INFO SessionStateInfo;

  Pn5180.CheckSessionState(&SessionStateInfo);

  Serial.print("[Info] SessionState->LifeCycle : "); Serial.println(SessionStateInfo.LifeCycle);
  Serial.print("[Info] SessionState->SessionState : "); Serial.println(SessionStateInfo.SessionState);

  Pn5180.ReadEEPROM(&EEPROMData, EEPROMReadSize, PN5180_DL_START_ADDR);

  Serial.print("[Info] EEPROM Size "); Serial.print(EEPROMReadSize); Serial.println(" Bytes.");
  Serial.print("[Info] EEPROM Data : "); Pn5180.PrintHex8(EEPROMData, EEPROMReadSize);

  free(EEPROMData);
}