#include "bootloader.h"
#include "common.h"
#include "uart.h"
#include "can.h"

typedef  void (*pFunction)(void);

//处于bootloader模式
uint8_t m_Bootloader_EntryBootloaderCmd[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//处于重启模式
uint8_t m_Bootloader_ResetCmd_g[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//请求校验bootloader
uint8_t m_Bootloader_CheckBootloaderCmd[8] = {  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//进入bootloader请求
uint8_t g_Bootloader_EntryBootloaderReponse[8] = { 0x00, g_GOTOAPP_TIMEOUT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//进入app请求
uint8_t g_Bootloader_EntryUserAppReponse[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//检测bootloader请求
uint8_t g_Bootloader_CheckBootloaderReponse[8] = { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//擦除flash请求
uint8_t g_Bootloader_EraseFlashReponse[8] = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//数据发送请求
uint8_t g_Bootloader_DataReponse[8] ={ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//数据结束请求
uint8_t g_Bootloader_DataProgramEndReponse[8] = { 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


//跳转到运行程序地址
void GoToUserAPPorBOOT(uint32_t start_addr)
{
    pFunction jump_to_application;
				uint32_t sp = 0;  
				uint32_t pc	= 0;
				
			printf("start_addr：%x\r\n",start_addr);
				DelayMs(500);
				sp = *(uint32_t*)(start_addr); 
				pc	= *(uint32_t*)(start_addr + 4);		//RESET中断
				jump_to_application = (pFunction)pc;
				printf("sp：%x,pc: %x\r\n",sp,pc);
				// Change MSP and PSP
				__set_MSP(sp);
				__set_PSP(sp);
        SCB->VTOR = start_addr;
						//反初始化操作
				CAN_ITDMAConfig(HW_CAN0,3, kCAN_IT_Rx_Disable);
				
				// Jump to application
        jump_to_application();
}


/* 接收一个数据帧 */
AfterBootloader_CmdType Bootloader_DataParse_g(uint8_t *data, uint8_t dataLength)
{
    AfterBootloader_CmdType result = EntryBootloader;
    if(dataLength!=8)
		return ERR;
    //check command type if is Data
    switch(data[0])
    {
        case EntryBootloader:
            result = EntryBootloader;
            break;
        case Reset:
            result = Reset;
            break;
        case Data:
            result = Data;
            break;
        case DataEnd:
            result = DataEnd;
            break;
        case CheckBootloader:
            result = CheckBootloader;
            break;
        case Erase:
            result = Erase;
            break;
        default:return ERR;
    }
    return result;
}
/* 擦除flash */
uint32_t flash_erase(uint32_t addr)
{
		uint32_t count=0;
		FLASH_EraseSector(addr);//注意
		while(count<100000)
		count++;
    if(FLASH_EraseSector(addr) == FLASH_OK)
    {
        return BL_FLASH_OK;
    }
    else
    {
        return BL_FLASH_ERR;
    }
}
uint8_t dataForWrite[16];
/* 写入flash */
uint32_t addr1=0x6000-16;
uint32_t flash_write(const uint8_t *buf, uint32_t len)
{		
		uint32_t addr;
		if(buf[1]==0)
		{
			dataForWrite[0]=buf[4];
			dataForWrite[1]=buf[5];
			dataForWrite[2]=buf[6];
			dataForWrite[3]=buf[7];
		}
		if(buf[1]==1)
		{
			dataForWrite[4]=buf[4];
			dataForWrite[5]=buf[5];
			dataForWrite[6]=buf[6];
			dataForWrite[7]=buf[7];
		}
		if(buf[1]==2)
		{
			dataForWrite[8]=buf[4];
			dataForWrite[9]=buf[5];
			dataForWrite[10]=buf[6];
			dataForWrite[11]=buf[7];
		}
		if(buf[1]==3)
		{
			//addr= buf[2]*256+buf[3]-12;
			addr=addr1;
			addr1=addr+16;
			dataForWrite[12]=buf[4];
			dataForWrite[13]=buf[5];
			dataForWrite[14]=buf[6];
			dataForWrite[15]=buf[7];
			if(FLASH_WriteSector(addr,dataForWrite,16)==0)
				return BL_FLASH_OK;
			else
				return BL_FLASH_ERR;
		}
		return BL_FLASH_ERR;
}

