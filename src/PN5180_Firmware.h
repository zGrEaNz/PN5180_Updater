/*
 Name:		PN5180_FIRMWARE.h
 Created:	8/8/2019 8:00:00 AM
 Author:	zGrEaNz
 Editor:	http://www.visualmicro.com
 Base on:	PN5180 Secure Firmware Update Library (SW4055)
*/

#ifndef _PN5180_FIRMWARE_H
#define _PN5180_FIRMWARE_H

#include <Arduino.h>
#include <SPI.h>

/********************************************************************************************************/
/*							PRIVATE DEFINES FOR THE DOWNLOAD LIBRARY									*/
/********************************************************************************************************/

#define PN5180_DL_DIRECTION_BYTE_POS			0	/** <Very first byte is direction byte.											*/
#define PN5180_DL_DIRECTION_BYTE_LEN			1	/** <direction byte is 1 byte.													*/

#define PN5180_DL_HEADER_BYTE0_POS				1	/** <First byte of header.														*/
#define PN5180_DL_HEADER_BYTE1_POS				2	/** <Second byte of header.														*/
#define PN5180_DL_HEADER_LEN					2	/** <DL header is 2 bytes.														*/

#define PN5180_DL_PAYLOAD_OFFSET				(PN5180_DL_HEADER_LEN)	/** <Payload starts after header							*/

#define PN5180_DL_PAYLOAD_OPCODE_POS			0	/* first byte of payload is opcode byte											*/
#define PN5180_DL_PAYLOAD_OPCODE_LEN			1	/** <Opcode is 1 byte.															*/

#define PN5180_DL_CRC_LEN						2	/** <CRC is 2 bytes.															*/

#define PN5180_DL_FRAME_OVERHEAD				(PN5180_DL_HEADER_LEN + PN5180_DL_CRC_LEN)	/** < bytes apart from payload data.	*/

#define PN5180_DL_STATUS_BYTE_POS				(PN5180_DL_PAYLOAD_OFFSET + PN5180_DL_DIRECTION_BYTE_LEN)

#define PN5180_DL_FW_MINOR_VERS_POS				4
#define PN5180_DL_FW_MAJOR_VERS_POS				5

#define PN5180_DL_CMD_FIRST_BYTE				0x7F

/* OPCODES */
#define PN5180_DL_OPCODE_DL_RESET				0xF0	/** <This command resets the IC.								*/
#define PN5180_DL_OPCODE_DL_GETVERSION			0xF1	/** <This command provides the firmware version.				*/
#define PN5180_DL_OPCODE_DL_GETDIEID			0xF4	/** <The command returns the die ID.							*/
#define PN5180_DL_OPCODE_DL_GETSESSIONSTATE		0xF2	/** <This command provides the session state and lifecycle.		*/
#define PN5180_DL_OPCODE_DL_CHECKINTEGRITY		0xE0	/** <This command checks the integrity of components.			*/
#define PN5180_DL_OPCODE_DL_SEC_WRITE			0xC0	/** <This command writes a chunk of data.						*/
#define PN5180_DL_OPCODE_DL_READ				0xA2	/** <This command reads the EEPROM User Area Address specified. */

/* Return codes */
#define PN5180_DL_OK							0x00	/**<.   command passed 				*/
#define PN5180_DL_INVALID_ADDR					0x01	/**<.   address not allowed 		*/
#define PN5180_DL_UNKNOW_CMD					0x0B	/**<.   unknown command 			*/
#define PN5180_DL_ABORTED_CMD					0x0C	/**<.   chunk sequence is large		*/
#define PN5180_DL_PLL_ERROR						0x0D	/**<.   flash not activated 		*/
#define PN5180_DL_ADDR_RANGE_OFL_ERROR			0x1E	/**<.   address out of range 		*/
#define PN5180_DL_BUFFER_OFL_ERROR				0x1F	/**<.   the buffer is too small 	*/
#define PN5180_DL_MEM_BSY						0x20	/**<.   no RSA key access 			*/
#define PN5180_DL_SIGNATURE_ERROR				0x21	/**<.   signature mismatch 			*/
#define PN5180_DL_FIRMWARE_VERSION_ERROR		0x24	/**<.   already up to date 			*/
#define PN5180_DL_PROTOCOL_ERROR				0x28	/**<.   protocol error 				*/
#define PN5180_DL_SFWU_DEGRADED					0x2A	/**<.   eeprom corruption 			*/
#define PN5180_PH_STATUS_DL_FIRST_CHUNK			0x2D	/**<.   first chunk received 		*/
#define PN5180_PH_STATUS_DL_NEXT_CHUNK			0x2E	/**<.   wait for the next chunk 	*/
#define PN5180_PH_STATUS_INTERNAL_ERROR_5		0xC5	/**<.   length mismatch 			*/

/* GetVersion */
#define PN5180_DL_GETVERSION_CMD_LEN			4	/**< get version has a fixed command length of 4.	*/
#define PN5180_DL_GETVERSION_RESP_LEN			10	/**< get version response length.					*/

#define PN5180_DL_GETVERSION_FMxV_POS1			8  /**<  Minor Version Number. */
#define PN5180_DL_GETVERSION_FMxV_POS2			9  /**<  Major Version Number. */

/* GetDieId */
#define PN5180_DL_GETDIEID_CMD_LEN				4	/**< fixed command length of getDieID.	*/
#define PN5180_DL_GETDIEID_RESP_LEN				20	/**< fixed response length of GetDieID. */

#define PN5180_DL_GETDIEID_DIEID_POS			4	/**< Position of the DieID in response. */
#define PN5180_DL_GETDIEID_DIEID_LEN			16	/**< Length of the DieID.				*/

/* SoftReset */
#define PN5180_DL_SOFTRESET_CMD_LEN				4	/**< Command length of soft reset.	*/
#define PN5180_DL_SOFTRESET_RESP_LEN			0	/**< No response for soft reset.	*/

/* CheckSessionState */
#define PN5180_DL_SESSIONSTATE_CMD_LEN			4	/**< GetSessionState command length.					*/
#define PN5180_DL_SESSIONSTATE_RESP_LEN			4	/**< Response length of get session state				*/

#define PN5180_DL_SESSIONSTATE_SSTA_POS			1  /**< position of Session state byte: 0..closed, 1..open	*/
#define PN5180_DL_SESSIONSTATE_SSTA_LEN			1  /**< Session state is 1 byte .							*/
#define PN5180_DL_SESSIONSTATE_LIFE_POS			3  /**< Position of lifeCycle byte.							*/
#define PN5180_DL_SESSIONSTATE_LIFE_LEN			1  /**< Life cycle is represented in 1 byte					*/

/* CheckIntegrity */
#define PN5180_DL_USER_E2AREA_PROTECTED_DATA_OFFSET    0x2BC
#define PN5180_DL_USER_E2AREA_LENGTH                   0xC00

#define PN5180_DL_CHECKINTEGRITY_CMD_LEN		8		/**< Command length of check integrity.				*/
#define PN5180_DL_CHECKINTEGRITY_RESP_LEN		32		/**< Command length of soft reset.					*/

#define PN5180_DL_CHECKINTEGRITY_UOFF_POS		4
#define PN5180_DL_CHECKINTEGRITY_ULEN_POS		6

#define PN5180_DL_CHECKINTEGRITY_RESP_STAT_POS  0
#define PN5180_DL_CHECKINTEGRITY_RESP_CRCS_POS  1

#define PN5180_DL_CHECKINTEGRITY_RESP_CRCS_FUNC_TABLE_BITPOS      6  /**< Function table CRC is OK. */
#define PN5180_DL_CHECKINTEGRITY_RESP_CRCS_PATCH_TABLE_BITPOS     5  /**< Patch table CRC is OK.	*/
#define PN5180_DL_CHECKINTEGRITY_RESP_CRCS_FUNC_CODE_BITPOS       4  /**< Function code CRC is OK.	*/
#define PN5180_DL_CHECKINTEGRITY_RESP_CRCS_PATCH_CODE_BITPOS      3  /**< Patch code CRC is OK.		*/
#define PN5180_DL_CHECKINTEGRITY_RESP_CRCS_PROT_DATA_BITPOS       2  /**< Protected data CRC is OK. */
#define PN5180_DL_CHECKINTEGRITY_RESP_CRCS_TRIM_DATA_BITPOS       1  /**< Trim data CRC is OK.		*/
#define PN5180_DL_CHECKINTEGRITY_RESP_CRCS_USER_DATA_BITPOS       0  /**< User data CRC is OK.		*/

/* Read command */
#define PN5180_DL_READ_CMD_LEN					8			/**< read command length.					*/
#define PN5180_DL_READ_RESP_LEN					4			/**< fixed response part of read command.	*/
#define PN5180_DL_MAX_READ_LEN					546			/**< Maximum of 546 bytes can be readout.	*/
#define PN5180_DL_START_ADDR					0x201380	/**< Start address for the READ function.	*/
#define PN5180_DL_END_ADDR						0x201F80	/**< End address for the READ function.		*/

/* Secure write command */
#define PN5180_DL_SWRITE_RESP_LEN				4	/**<respose length for chunk write.				*/
#define PN5180_DL_SWRITE_PAYLOD_OFFSET			2	/**<Chunk data starts after 2nd byte.			*/
#define PN5180_DL_SWRITE_MAX_CHUNK_SIZE			256	/**<Chunk size due to BAL buffer limitations.	*/

/********************************************************************************************************/

/*
 *
 * brief Download session state information structure.
 * This structure will contain the session state and lifecycle information.
 *
 */
typedef struct PN5180_DOWNLOAD_SESSION_STATE_INFO {
	uint8_t SessionState;
	uint8_t LifeCycle;
} PN5180_DOWNLOAD_SESSION_STATE_INFO_T;

/*
 *
 * brief Integrity information structure.
 * this structure will hold the integrity information of all the applicable blocks.
 *
 */
typedef struct PN5180_DOWNLOAD_INTEGRITY_INFO {
	uint8_t PatchTableOk;
	uint8_t FunctionCodeOk;
	uint8_t PatchCodeOk;
	uint8_t UserDataOk;
} PN5180_DOWNLOAD_INTEGRITY_INFO_T;

class PN5180_Firmware {
public:
	PN5180_Firmware(uint8_t Reset, uint8_t Busy, uint8_t Req, uint8_t Nss, uint8_t Mosi = PA7, uint8_t Miso = PA6, uint8_t Sck = PA5);
	void PrintHex8(uint8_t* Data, size_t Size, bool Reverse = false);
	void Begin();
	void End();
	bool SecureCommandTransceive(uint8_t* CommandBuffer, size_t CommandBufferSize, uint8_t** ResponseBuffer, size_t* ResponseBufferSize);
	bool SecureDownloadTransceive(uint8_t* CommandBuffer, size_t CommandBufferSize, uint8_t** ResponseBuffer, size_t* ResponseBufferSize, size_t MaxChunkSize = 256);
	bool ReadEEPROM(uint8_t** ReadBuffer, size_t ReadSize, uint8_t StartAddress);
	bool GetFirmwareVersion(uint8_t* MajorVersion, uint8_t* MinorVersion);
	bool GetDieID(uint8_t* DieId);
	bool CheckIntegrity(PN5180_DOWNLOAD_INTEGRITY_INFO_T* IntegrityInfo);
	bool CheckSessionState(PN5180_DOWNLOAD_SESSION_STATE_INFO* SessionStateInfo);
	bool DoUpdateFirmware(uint8_t* FirmwareImage, size_t FirmwareImageSize);
	bool DoSoftReset();
private:
	uint8_t ResetPin, BusyPin, ReqPin, NssPin, MosiPin, MisoPin, SckPin;
	SPISettings SPIConfig;
	bool RawTransceive(const uint8_t* TxBuffer, const size_t TxSize, uint8_t* RxBuffer, size_t RxSize);
	uint16_t CalcCRC16(uint8_t* Data, size_t Size);
};

#endif