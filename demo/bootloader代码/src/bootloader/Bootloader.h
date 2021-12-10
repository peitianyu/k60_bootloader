#ifndef __CH_LIB_BOOTLOADER_H__
#define __CH_LIB_BOOTLOADER_H__

#include "common.h"
#include "flash.h"
//����app��ʱʱ��Ϊ4s��������4s����appģʽ
#define g_GOTOAPP_TIMEOUT 4
/* ����boot��appģʽ��ַ*/
#define Bootloader_StartAddr 0x0000
#define APP_StartAddr 			 0x6000
#define G_ReceiveCANID 				0x25
#define G_SendCANID 					0x12
#define BL_FLASH_OK     (0x00)
#define BL_FLASH_ERR    (0x01)


//����λ������������
extern uint8_t g_Bootloader_EntryBootloaderReponse[8];
extern uint8_t g_Bootloader_CheckBootloaderReponse[8];
extern uint8_t g_Bootloader_EraseFlashReponse[8];
extern uint8_t g_Bootloader_DataReponse[8];
extern uint8_t g_Bootloader_DataProgramEndReponse[8];

typedef enum 
{
    EntryBootloader=0,Reset=1,Data=2,DataEnd=3,CheckBootloader=4,Erase=5,ERR=0xff
}AfterBootloader_CmdType;

void GoToUserAPPorBOOT(uint32_t start_addr);
/* ��������֡data[0] */
AfterBootloader_CmdType Bootloader_DataParse_g(uint8_t *data, uint8_t dataLength);
uint32_t flash_erase(uint32_t addr);
uint32_t flash_write(const uint8_t *buf, uint32_t len);
#endif


