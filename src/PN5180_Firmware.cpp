/*
 Name:		PN5180_Firmware.cpp
 Created:	8/8/2019 8:00:00 AM
 Author:	zGrEaNz
 Editor:	http://www.visualmicro.com
 Base on:	PN5180 Secure Firmware Update Library (SW4055)
*/

#include "PN5180_Firmware.h"

PN5180_Firmware::PN5180_Firmware(uint8_t Reset, uint8_t Busy, uint8_t Req, uint8_t Nss, uint8_t Mosi, uint8_t Miso, uint8_t Sck) {
	ResetPin = Reset;
	NssPin = Nss;
	MosiPin = Mosi;
	MisoPin = Miso;
	SckPin = Sck;
	BusyPin = Busy;
	ReqPin = Req;

	SPIConfig = SPISettings(7000000L, MSBFIRST, SPI_MODE0);
}

uint16_t PN5180_Firmware::CalcCRC16(uint8_t* Data, size_t Size) {
	uint16_t CRC16New;
	uint16_t CRC16 = 0xFFFFU;

	for (int32_t I = 0; I < Size; I++) {
		CRC16New = (uint8_t)(CRC16 >> 8) | (CRC16 << 8);
		CRC16New ^= Data[I];
		CRC16New ^= (uint8_t)(CRC16New & 0xFF) >> 4;
		CRC16New ^= CRC16New << 12;
		CRC16New ^= (CRC16New & 0xFF) << 5;
		CRC16 = CRC16New;
	}

	return CRC16;
}

void PN5180_Firmware::PrintHex8(uint8_t* Data, size_t Size, bool Reverse) {
	char Temp[16];

	if (!Reverse) {
		for (int32_t I = 0; I < Size; I++) {
			sprintf(Temp, "0x%.2X", Data[I]);
			Serial.print(Temp); Serial.print(" ");
		}
	} else {
		for (int32_t I = Size - 1; I >= 0; I--) {
			sprintf(Temp, "0x%.2X", Data[I]);
			Serial.print(Temp); Serial.print(" ");
		}
	}

	Serial.println("");

	return;
}

void PN5180_Firmware::Begin() {
	SPI.setMOSI(MosiPin);
	SPI.setMISO(MisoPin);
	SPI.setSCLK(SckPin);

	pinMode(ResetPin, OUTPUT);
	pinMode(NssPin, OUTPUT);
	pinMode(BusyPin, INPUT);
	pinMode(ReqPin, OUTPUT);

	digitalWrite(NssPin, HIGH);

	/* Enter to download mode. */
	digitalWrite(ReqPin, HIGH);

	/* Reset chip. */
	digitalWrite(ResetPin, LOW); delay(1);
	digitalWrite(ResetPin, HIGH); delay(1);

	SPI.begin();

	return;
}

void PN5180_Firmware::End() {
	digitalWrite(NssPin, HIGH);

	/* Exit from download mode. */
	digitalWrite(ReqPin, LOW);

	/* Reset chip. */
	digitalWrite(ResetPin, LOW); delay(1);
	digitalWrite(ResetPin, HIGH); delay(1);

	SPI.end();

	return;
}

bool PN5180_Firmware::RawTransceive(const uint8_t* TxBuffer, const size_t TxSize, uint8_t* RxBuffer, size_t RxSize) {
	bool IsSend = false, IsReceive = false;

	SPI.beginTransaction(SPIConfig);

	if (TxBuffer != NULL && TxSize != 0) {
		while (digitalRead(BusyPin) == HIGH);

		digitalWrite(NssPin, LOW); delay(2);

		SPI.transfer((uint8_t*)TxBuffer, TxSize);

		digitalWrite(NssPin, HIGH); delay(1);

		IsSend = true;
	}

	if (RxBuffer != NULL && RxSize != 0) {
		while (digitalRead(BusyPin) == LOW);

		digitalWrite(NssPin, LOW); delay(2);

		memset(RxBuffer, 0xFF, RxSize);

		SPI.transfer(RxBuffer, RxSize);
		
		digitalWrite(NssPin, HIGH); delay(1);

		IsReceive = true;
	}

	SPI.endTransaction();

	if (TxBuffer != NULL && TxSize != 0 && !IsSend)
		return false;

	if (TxBuffer[3] != PN5180_DL_OPCODE_DL_RESET && RxBuffer != NULL && RxSize != 0 && !IsReceive)
		return false;

	return true;
}

bool PN5180_Firmware::SecureCommandTransceive(uint8_t* CommandBuffer, size_t CommandBufferSize, uint8_t** ResponseBuffer, size_t* ResponseBufferSize) {
	bool Status = true;

	size_t TxLength, RxLength;

	uint16_t Checksum, ChecksumPos;

	do {
		uint8_t* TxBuffer = (uint8_t*)malloc((PN5180_DL_FRAME_OVERHEAD + CommandBufferSize + PN5180_DL_DIRECTION_BYTE_LEN) * sizeof(uint8_t));
		uint8_t* RxBuffer = (uint8_t*)malloc((PN5180_DL_FRAME_OVERHEAD + *ResponseBufferSize + PN5180_DL_DIRECTION_BYTE_LEN) * sizeof(uint8_t));

		/* First byte required in Download mode. */
		TxBuffer[PN5180_DL_DIRECTION_BYTE_POS] = PN5180_DL_CMD_FIRST_BYTE;

		/* Length fields updation to Txframe. */
		TxBuffer[PN5180_DL_HEADER_BYTE0_POS] = (uint8_t)((CommandBufferSize >> 8) & 0x03);
		TxBuffer[PN5180_DL_HEADER_BYTE1_POS] = (uint8_t)(CommandBufferSize & 0xFF);

		/* Store command To Txframe */
		memcpy(&TxBuffer[PN5180_DL_DIRECTION_BYTE_LEN + PN5180_DL_HEADER_LEN], &CommandBuffer[0], CommandBufferSize);

		/* CRC calculation. */
		Checksum = CalcCRC16(&TxBuffer[PN5180_DL_HEADER_BYTE0_POS], PN5180_DL_HEADER_LEN + CommandBufferSize);

		/* CRC field updation to Txframe. */
		TxBuffer[CommandBufferSize + PN5180_DL_PAYLOAD_OFFSET + PN5180_DL_DIRECTION_BYTE_LEN] = (uint8_t)((Checksum >> 8) & 0xFF);
		TxBuffer[CommandBufferSize + PN5180_DL_PAYLOAD_OFFSET + PN5180_DL_DIRECTION_BYTE_LEN + 1] = (uint8_t)(Checksum & 0xFF);

		/* No response available for SOFTRESET command. */
		if (TxBuffer[PN5180_DL_HEADER_LEN + PN5180_DL_DIRECTION_BYTE_LEN] != PN5180_DL_OPCODE_DL_RESET)
			RxLength = *ResponseBufferSize + PN5180_DL_FRAME_OVERHEAD + PN5180_DL_DIRECTION_BYTE_LEN;

		/* Totalframe = Frame Header + Data + CRC */
		TxLength = PN5180_DL_FRAME_OVERHEAD + CommandBufferSize + PN5180_DL_DIRECTION_BYTE_LEN;

		/* Perform a SPI exchange. */
		if (!RawTransceive(TxBuffer, TxLength, RxBuffer, RxLength)) {
			Serial.println("[Error] Transceive Failed.");
			Status = false;
			free(TxBuffer);
			free(RxBuffer);			
			break;
		}

		/* Check CRC of the response. */
		Checksum = CalcCRC16(&(RxBuffer[PN5180_DL_HEADER_BYTE0_POS]), (RxLength - PN5180_DL_CRC_LEN - PN5180_DL_DIRECTION_BYTE_LEN));

		ChecksumPos = RxLength - (PN5180_DL_CRC_LEN);

		if ((RxBuffer[ChecksumPos] != (uint8_t)((Checksum >> 8) & 0xFF)) || (RxBuffer[ChecksumPos + 1] != (uint8_t)(Checksum & 0xFF))) {
			/* CRC mismatch. */
			Serial.println("[Error] Rx CRC mismatch.");
			Status = false;
			free(TxBuffer);
			free(RxBuffer);			
			break;	
		}

		/* Check status of the response. */
		if (RxBuffer[PN5180_DL_STATUS_BYTE_POS] != PN5180_DL_OK) {
			/* Error status received. */
			Serial.print("[Error] Error Status ["); Serial.print(RxBuffer[PN5180_DL_STATUS_BYTE_POS], HEX); Serial.println("]");
			Status = false;
			free(TxBuffer);
			free(RxBuffer);
			break;
		}

		/* Setting the return buffer. */
		*ResponseBuffer = &(RxBuffer)[PN5180_DL_PAYLOAD_OFFSET + PN5180_DL_DIRECTION_BYTE_LEN];
		*ResponseBufferSize = RxLength - PN5180_DL_FRAME_OVERHEAD - PN5180_DL_DIRECTION_BYTE_LEN;

		free(TxBuffer);
		free(RxBuffer);

	} while (0);

	return Status;
}

bool PN5180_Firmware::SecureDownloadTransceive(uint8_t* CommandBuffer, size_t CommandBufferSize, uint8_t** ResponseBuffer, size_t* ResponseBufferSize, size_t MaxChunkSize) {
	bool Status = true, LastChunk;

	size_t ChunkCommandLength, ChunkResponseLength, TxLength, RxLength;

	uint16_t Checksum, ChecksumPos, ChunkCounter;
	
	do {
		/* Chunking the length when the command length is greater than MAX Size supported by SPI. */
		if (CommandBufferSize > MaxChunkSize) {
			ChunkCommandLength = MaxChunkSize;
			ChunkResponseLength = PN5180_DL_SWRITE_RESP_LEN;
			LastChunk = false;
		} else {
			ChunkCommandLength = CommandBufferSize;
			ChunkResponseLength = *ResponseBufferSize;
			LastChunk = true;
		}

		uint8_t* ChunkBuffer = (uint8_t*)malloc((PN5180_DL_DIRECTION_BYTE_LEN + MaxChunkSize + PN5180_DL_FRAME_OVERHEAD) * sizeof(uint8_t));
		uint8_t* RxBuffer = (uint8_t*)malloc((ChunkResponseLength + PN5180_DL_FRAME_OVERHEAD + PN5180_DL_DIRECTION_BYTE_LEN) * sizeof(uint8_t));

		/* Copy the chunk for transmission. */
		memcpy(&(ChunkBuffer[PN5180_DL_DIRECTION_BYTE_LEN + PN5180_DL_HEADER_LEN]), &(CommandBuffer[ChunkCounter]), ChunkCommandLength); /* PRQA S 3200 */

		/* Forming the final frame with header. */
		if (!LastChunk) {
			/* Chunk of a Frame transmitted with bit10 set Required for chunking. */
			ChunkBuffer[PN5180_DL_DIRECTION_BYTE_POS] = PN5180_DL_CMD_FIRST_BYTE;
			ChunkBuffer[PN5180_DL_HEADER_BYTE0_POS] = (uint8_t)(((ChunkCommandLength >> 8) | 0x4) & 0x07);
			ChunkBuffer[PN5180_DL_HEADER_BYTE1_POS] = (uint8_t)(ChunkCommandLength & 0xFF);
		} else {
			ChunkBuffer[PN5180_DL_DIRECTION_BYTE_POS] = PN5180_DL_CMD_FIRST_BYTE;
			ChunkBuffer[PN5180_DL_HEADER_BYTE0_POS] = (uint8_t)((ChunkCommandLength >> 8) & 0x03);
			ChunkBuffer[PN5180_DL_HEADER_BYTE1_POS] = (uint8_t)(ChunkCommandLength & 0xFF);
		}

		/* Calculate and add the CRC to the transmit data chunk. */
		Checksum = CalcCRC16(&ChunkBuffer[PN5180_DL_HEADER_BYTE0_POS], PN5180_DL_HEADER_LEN + ChunkCommandLength);

		ChunkBuffer[PN5180_DL_DIRECTION_BYTE_LEN + ChunkCommandLength + PN5180_DL_PAYLOAD_OFFSET] = (uint8_t)((Checksum >> 8) & 0xFF);
		ChunkBuffer[PN5180_DL_DIRECTION_BYTE_LEN + ChunkCommandLength + PN5180_DL_PAYLOAD_OFFSET + 1] = (uint8_t)(Checksum & 0xFF);

		/* Totalframe = Frame Header + Data + CRC */
		TxLength = PN5180_DL_DIRECTION_BYTE_LEN + PN5180_DL_FRAME_OVERHEAD + ChunkCommandLength;

		/* Response length. */
		RxLength = ChunkResponseLength + PN5180_DL_FRAME_OVERHEAD + PN5180_DL_DIRECTION_BYTE_LEN;

		/* Perform a SPI exchange. */
		if (!RawTransceive(ChunkBuffer, TxLength, RxBuffer, RxLength)) {
			Serial.println("[Error] Transceive Failed.");
			Status = false;
			free(ChunkBuffer);
			free(RxBuffer);
			break;
		}

		CommandBufferSize -= ChunkCommandLength;
		ChunkCounter += ChunkCommandLength;

		/* Check CRC of the response. */
		Checksum = CalcCRC16(&(RxBuffer[PN5180_DL_HEADER_BYTE0_POS]), (RxLength - PN5180_DL_CRC_LEN - PN5180_DL_DIRECTION_BYTE_LEN));

		ChecksumPos = RxLength - PN5180_DL_CRC_LEN;

		if ((RxBuffer[ChecksumPos] != (uint8_t)((Checksum >> 8) & 0xFF)) || (RxBuffer[ChecksumPos + 1] != (uint8_t)(Checksum & 0xFF))) {
			//!Attention: normal situation in case of Read, and Reset commands.
			Serial.println("[Error] Rx CRC Mismatch.");
			Status = false;
			free(ChunkBuffer);
			free(RxBuffer);
			break;
		}

		/* Check status of the response. */
		if (RxBuffer[PN5180_DL_STATUS_BYTE_POS] != PN5180_DL_OK && RxBuffer[PN5180_DL_STATUS_BYTE_POS] != PN5180_PH_STATUS_DL_FIRST_CHUNK && RxBuffer[PN5180_DL_STATUS_BYTE_POS] != PN5180_PH_STATUS_DL_NEXT_CHUNK) {
			//!Attention: normal situation in case of Reset command (if successful)
			Serial.print("[Warning] Bad Status ["); Serial.print(RxBuffer[PN5180_DL_STATUS_BYTE_POS], HEX); Serial.println("]");
			Status = false;
			free(ChunkBuffer);
			free(RxBuffer);
			break;
		}

		/* Setting the return buffer. */
		*ResponseBuffer = &(RxBuffer)[PN5180_DL_FRAME_OVERHEAD + PN5180_DL_DIRECTION_BYTE_LEN];
		*ResponseBufferSize = RxLength - PN5180_DL_FRAME_OVERHEAD - PN5180_DL_DIRECTION_BYTE_LEN;

		/* Identifying the Last Chunk. */
		if ((CommandBufferSize - ChunkCommandLength) <= 0)
			LastChunk = true;

		free(ChunkBuffer);
		free(RxBuffer);

	} while (CommandBufferSize > 0);

	return Status;
}

bool PN5180_Firmware::ReadEEPROM(uint8_t** ReadBuffer, size_t ReadSize, uint8_t StartAddress) {
	size_t RxBufferSize = ReadSize + PN5180_DL_READ_RESP_LEN;

	if (ReadSize > PN5180_DL_MAX_READ_LEN)
		return false;

	/* Return error if the address to read is out of range. */
	if ((StartAddress < PN5180_DL_START_ADDR) || (StartAddress > PN5180_DL_END_ADDR))
		return false;

	uint8_t* TxBuffer = (uint8_t*)malloc(PN5180_DL_READ_CMD_LEN * sizeof(uint8_t));

	TxBuffer[PN5180_DL_PAYLOAD_OPCODE_POS] = PN5180_DL_OPCODE_DL_READ;
	TxBuffer[1] = 0x00;

	/* Setting length (little endian) */
	TxBuffer[2] = (uint8_t)(ReadSize & 0xFF);
	TxBuffer[3] = (uint8_t)((ReadSize >> 8) & 0xFF);

	/* Setting address (little endian) */
	TxBuffer[4] = (uint8_t)(StartAddress & 0xFF);
	TxBuffer[5] = (uint8_t)((StartAddress >> 8) & 0xFF);
	TxBuffer[6] = (uint8_t)((StartAddress >> 16) & 0xFF);
	TxBuffer[7] = (uint8_t)((StartAddress >> 24) & 0xFF);

	/* Send command and get response. */
	if (!SecureCommandTransceive(TxBuffer, PN5180_DL_READ_CMD_LEN, ReadBuffer, &RxBufferSize)) {
		free(TxBuffer);
		return false;
	}

	free(TxBuffer);

	return true;
}

bool PN5180_Firmware::GetFirmwareVersion(uint8_t* MajorVersion, uint8_t* MinorVersion) {
	uint8_t* TxBuffer = (uint8_t*)malloc(PN5180_DL_GETVERSION_CMD_LEN * sizeof(uint8_t));
	uint8_t* RxBuffer = (uint8_t*)malloc(PN5180_DL_GETVERSION_RESP_LEN * sizeof(uint8_t));

	size_t RxBufferSize = PN5180_DL_GETVERSION_RESP_LEN;

	/* Form the command. */
	TxBuffer[PN5180_DL_PAYLOAD_OPCODE_POS] = PN5180_DL_OPCODE_DL_GETVERSION;
	TxBuffer[1] = 0x00;
	TxBuffer[2] = 0x00;
	TxBuffer[3] = 0x00;

	/* Send command and get response. */
	if (!SecureCommandTransceive(TxBuffer, PN5180_DL_GETVERSION_CMD_LEN, &RxBuffer, &RxBufferSize)) {
		free(TxBuffer);
		free(RxBuffer);
		return false;
	}

	/* Copy the major and minor version. */
	*MajorVersion = RxBuffer[PN5180_DL_GETVERSION_FMxV_POS2];
	*MinorVersion = RxBuffer[PN5180_DL_GETVERSION_FMxV_POS1];

	free(TxBuffer);
	free(RxBuffer);

	return true;
}

bool PN5180_Firmware::GetDieID(uint8_t* DieId) {
	uint8_t* TxBuffer = (uint8_t*)malloc(PN5180_DL_GETDIEID_CMD_LEN * sizeof(uint8_t));
	uint8_t* RxBuffer = (uint8_t*)malloc(PN5180_DL_GETDIEID_RESP_LEN * sizeof(uint8_t));

	size_t RxBufferSize = PN5180_DL_GETDIEID_RESP_LEN;

	/* Form the command. */
	TxBuffer[PN5180_DL_PAYLOAD_OPCODE_POS] = PN5180_DL_OPCODE_DL_GETDIEID;
	TxBuffer[1] = 0x00;
	TxBuffer[2] = 0x00;
	TxBuffer[3] = 0x00;

	/* Send command and get response. */
	if (!SecureCommandTransceive(TxBuffer, PN5180_DL_GETDIEID_CMD_LEN, &RxBuffer, &RxBufferSize)) {
		free(TxBuffer);
		free(RxBuffer);
		return false;
	}

	memcpy(DieId, &RxBuffer[PN5180_DL_GETDIEID_DIEID_POS], PN5180_DL_GETDIEID_DIEID_LEN);

	free(TxBuffer);
	free(RxBuffer);

	return true;
}

bool PN5180_Firmware::CheckIntegrity(PN5180_DOWNLOAD_INTEGRITY_INFO_T* IntegrityInfo) {
	uint8_t CRCBitmap;
	uint8_t* TxBuffer = (uint8_t*)malloc(PN5180_DL_CHECKINTEGRITY_CMD_LEN * sizeof(uint8_t));
	uint8_t* RxBuffer = (uint8_t*)malloc(PN5180_DL_CHECKINTEGRITY_RESP_LEN * sizeof(uint8_t));

	size_t RxBufferSize = PN5180_DL_CHECKINTEGRITY_RESP_LEN;

	uint16_t UOFF = PN5180_DL_USER_E2AREA_PROTECTED_DATA_OFFSET;
	uint16_t ULEN = PN5180_DL_USER_E2AREA_LENGTH;
	
	/* Form the command. */
	TxBuffer[PN5180_DL_PAYLOAD_OPCODE_POS] = PN5180_DL_OPCODE_DL_CHECKINTEGRITY;
	TxBuffer[1] = 0x00;
	TxBuffer[2] = 0x00;
	TxBuffer[3] = 0x00;

	/* UOFF and ULEN stored little endian. */
	TxBuffer[PN5180_DL_CHECKINTEGRITY_UOFF_POS] = (uint8_t)(UOFF & 0xFF);
	TxBuffer[PN5180_DL_CHECKINTEGRITY_UOFF_POS + 1] = (uint8_t)((UOFF >> 8) & 0xFF);
	TxBuffer[PN5180_DL_CHECKINTEGRITY_ULEN_POS] = (uint8_t)(ULEN & 0xFF);
	TxBuffer[PN5180_DL_CHECKINTEGRITY_ULEN_POS + 1] = (uint8_t)((ULEN >> 8) & 0xFF);

	if (!SecureCommandTransceive(TxBuffer, PN5180_DL_CHECKINTEGRITY_CMD_LEN, &RxBuffer, &RxBufferSize)) {
		free(TxBuffer);
		free(RxBuffer);
		return false;
	}

	CRCBitmap = RxBuffer[PN5180_DL_CHECKINTEGRITY_RESP_CRCS_POS];

	/* Trim Data is not part of firmware and PN5180 does not use function table. */
	IntegrityInfo->FunctionCodeOk = (CRCBitmap >> PN5180_DL_CHECKINTEGRITY_RESP_CRCS_FUNC_CODE_BITPOS) & 0x01;
	IntegrityInfo->PatchCodeOk = (CRCBitmap >> PN5180_DL_CHECKINTEGRITY_RESP_CRCS_PATCH_CODE_BITPOS) & 0x01;
	IntegrityInfo->PatchTableOk = (CRCBitmap >> PN5180_DL_CHECKINTEGRITY_RESP_CRCS_PATCH_TABLE_BITPOS) & 0x01;
	IntegrityInfo->UserDataOk = (CRCBitmap >> PN5180_DL_CHECKINTEGRITY_RESP_CRCS_USER_DATA_BITPOS) & 0x01;

	free(TxBuffer);
	free(RxBuffer);

	return true;
}

bool PN5180_Firmware::CheckSessionState(PN5180_DOWNLOAD_SESSION_STATE_INFO* SessionStateInfo) {
	uint8_t* TxBuffer = (uint8_t*)malloc(PN5180_DL_SESSIONSTATE_CMD_LEN * sizeof(uint8_t));
	uint8_t* RxBuffer = (uint8_t*)malloc(PN5180_DL_SESSIONSTATE_RESP_LEN * sizeof(uint8_t));

	size_t RxBufferSize = PN5180_DL_SESSIONSTATE_RESP_LEN;

	/* Form the command. */
	TxBuffer[PN5180_DL_PAYLOAD_OPCODE_POS] = PN5180_DL_OPCODE_DL_GETSESSIONSTATE;
	TxBuffer[1] = 0x00;
	TxBuffer[2] = 0x00;
	TxBuffer[3] = 0x00;

	if (!SecureCommandTransceive(TxBuffer, PN5180_DL_SESSIONSTATE_CMD_LEN, &RxBuffer, &RxBufferSize)) {
		free(TxBuffer);
		free(RxBuffer);
		return false;
	}

	/*
     * Session State	- 0: close  - 1: open*/

    /*
     * Life cycle
				- 11d: creation
				- 13d: initializing
				- 17d: operational
				- 19d: termination */
	SessionStateInfo->SessionState = RxBuffer[PN5180_DL_SESSIONSTATE_SSTA_POS];
	SessionStateInfo->LifeCycle = RxBuffer[PN5180_DL_SESSIONSTATE_LIFE_POS];

	free(TxBuffer);
	free(RxBuffer);

	return true;
}

bool PN5180_Firmware::DoUpdateFirmware(uint8_t* FirmwareImage, size_t FirmwareImageSize) {
	bool Status = true;

	size_t CurrentBlockLength, RxBuffSize = PN5180_DL_SWRITE_RESP_LEN;

	uint8_t MajorVersion, MinorVersion;

	uint32_t CurrentBlock;

	if (FirmwareImageSize == 0)
		return false;

	GetFirmwareVersion(&MajorVersion, &MinorVersion);

	if (MajorVersion < FirmwareImage[PN5180_DL_FW_MAJOR_VERS_POS] && MinorVersion <= FirmwareImage[PN5180_DL_FW_MINOR_VERS_POS]) {
		Serial.println("[Error] Impossible For Downgrade.");

		return false;

	} else if (MajorVersion == FirmwareImage[PN5180_DL_FW_MAJOR_VERS_POS] && MinorVersion == FirmwareImage[PN5180_DL_FW_MINOR_VERS_POS]) {
    	Serial.println("[Info] Already Up To Date.");

    	return false;
  	}

  	Serial.print("[Info] Current Firmware "); Serial.print(MajorVersion, HEX); Serial.print("."); Serial.println(MinorVersion, HEX);
	Serial.print("[Info] Will Update To Firmware "); Serial.print(FirmwareImage[PN5180_DL_FW_MAJOR_VERS_POS], HEX); Serial.print("."); Serial.println(FirmwareImage[PN5180_DL_FW_MINOR_VERS_POS], HEX);
  	Serial.print("[Info] Update Size "); Serial.print(FirmwareImageSize); Serial.println(" Bytes.\n");

  	uint32_t StartTime = millis();

	/* Get current block length (stored bigendian) */
	CurrentBlockLength = (uint16_t)(((uint16_t)(FirmwareImage[CurrentBlock]) << 8) + FirmwareImage[CurrentBlock + 1]);

	while (CurrentBlock < FirmwareImageSize) {
		uint8_t* TxBuff = (uint8_t*)malloc((CurrentBlockLength) * sizeof(uint8_t));
		uint8_t* RxBuff = (uint8_t*)malloc(PN5180_DL_SWRITE_RESP_LEN * sizeof(uint8_t));

		/* The entire firmware is transmitted frame by frame. */
		memcpy(TxBuff, &FirmwareImage[CurrentBlock + PN5180_DL_SWRITE_PAYLOD_OFFSET], CurrentBlockLength);	/* PRQA S 3200 */

		if (!SecureDownloadTransceive(TxBuff, CurrentBlockLength, &RxBuff, &RxBuffSize)) {
			Status = false;
			free(TxBuff);
			free(RxBuff);
			break;
		}

		free(TxBuff);
		free(RxBuff);

		/* Advance pointer to next frame. */
		CurrentBlock += CurrentBlockLength + PN5180_DL_SWRITE_PAYLOD_OFFSET;

		float CurrentPercent = (float)CurrentBlock / (float)(FirmwareImageSize / 100);

		Serial.print("[Info] Uploading.. "); Serial.print(CurrentPercent, 2); Serial.println(" %");
		Serial.print("[Info] Uploaded "); Serial.print(CurrentBlock); Serial.println(" Bytes.\n");

		if (CurrentBlock >= FirmwareImageSize) {
			/* Whole image downloaded. */
			uint32_t EndTime = millis();
			Serial.print("[Info] Upload Success In "); Serial.print(float(EndTime - StartTime) / float(1000), 2); Serial.println(" Seconds.");

			break;
		}

		/* Calculate the next block length. */
		CurrentBlockLength = (uint16_t)(((uint16_t)(FirmwareImage[CurrentBlock]) << 8) & (0xFF00));
		CurrentBlockLength += FirmwareImage[CurrentBlock + 1];

		/* This check is to ensure that we don't read beyond the array of image. We should strictly limit our read to what the higher layer says is the size of image. */
		if ((CurrentBlock + CurrentBlockLength) > FirmwareImageSize) {
			Serial.println("[Error] Download Overflow.\n");
			Status = false;
			free(TxBuff);
			free(RxBuff);
			break;
		}
	}
	
	return Status;
}

bool PN5180_Firmware::DoSoftReset() {
	uint8_t* TxBuffer = (uint8_t*)malloc(PN5180_DL_SOFTRESET_CMD_LEN * sizeof(uint8_t));
	uint8_t* RxBuffer = (uint8_t*)malloc(PN5180_DL_SOFTRESET_RESP_LEN * sizeof(uint8_t));

	size_t RxBufferSize = PN5180_DL_SOFTRESET_RESP_LEN;

	/* Form the command. */
	TxBuffer[PN5180_DL_PAYLOAD_OPCODE_POS] = PN5180_DL_OPCODE_DL_RESET;
	TxBuffer[1] = 0x00;
	TxBuffer[2] = 0x00;
	TxBuffer[3] = 0x00;

	/* Send command and get response. */
	if (!SecureCommandTransceive(TxBuffer, PN5180_DL_SOFTRESET_CMD_LEN, &RxBuffer, &RxBufferSize)) {
		free(TxBuffer);
		free(RxBuffer);
		return false;
	}

	free(TxBuffer);
	free(RxBuffer);

	return true;
}