#include "bootloader.h"
#include "common.h"
#include "uart.h"
#include "can.h"

typedef  void (*pFunction)(void);

//����bootloaderģʽ
uint8_t m_Bootloader_EntryBootloaderCmd[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//��������ģʽ
uint8_t m_Bootloader_ResetCmd_g[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//����У��bootloader
uint8_t m_Bootloader_CheckBootloaderCmd[8] = {  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//����bootloader����
uint8_t g_Bootloader_EntryBootloaderReponse[8] = { 0x00, g_GOTOAPP_TIMEOUT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//����app����
uint8_t g_Bootloader_EntryUserAppReponse[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//���bootloader����
uint8_t g_Bootloader_CheckBootloaderReponse[8] = { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//����flash����
uint8_t g_Bootloader_EraseFlashReponse[8] = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//���ݷ�������
uint8_t g_Bootloader_DataReponse[8] ={ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//���ݽ�������
uint8_t g_Bootloader_DataProgramEndReponse[8] = { 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

////��ת�����г����ַ
//void GoToUserAPPorBOOT(uint32_t start_addr)
//{
//    pFunction jump_to_application;
//    uint32_t jump_addr;
//		uint32_t sp = *(uint32_t*)(start_addr);  
//		uint32_t pc	= *(uint32_t*)(start_addr + 4);		//RESET�ж�
//    //jump_addr = *(uint32_t*)(start_addr + 4);  //RESET�ж�
//    //�ر��ж� 
//		CAN_ITDMAConfig(HW_CAN0,3, kCAN_IT_Rx_Disable);
//				jump_to_application = (pFunction)jump_addr;
//				printf("addr��%x\r\n",jump_addr);
//        __set_MSP(start_addr); //ջ��ַ
//        SCB->VTOR = start_addr;
//        jump_to_application();
//}
//����ʼ������
//void Prepare_Before_Jump_m(void)
//{

//	//�ر�can�ж�
//	
//	//flexcan_0_deinit_fnc();   /*shutdown the MSCAN module*/
//	//�ر�cpu�ж�
//	PPCASM (" wrteei 0 "); 		  /*disable the CPU interrupt*/
//	//����NVM��������ʹ�õ��������س���RAM
//    CleanRAM();       /*clean the bootloader used RAM  for NVM driver*/

//}
//��ת�����г����ַ
void GoToUserAPPorBOOT(uint32_t start_addr)
{

	
    pFunction jump_to_application;
		uint32_t sp = 0;  
		uint32_t pc	= 0;

				sp = *(uint32_t*)(start_addr);  
				pc	= *(uint32_t*)(start_addr + 4);		//RESET�ж�

				jump_to_application = (pFunction)pc;
				printf("sp��%x,pc: %x\r\n",sp,pc);
				// Change MSP and PSP
				__set_MSP(sp);
				__set_PSP(sp);
        SCB->VTOR = start_addr;
						//����ʼ������
				__disable_irq();
				//UART_DeInit(HW_UART1);
				//CAN_DeInit(HW_CAN0);
				
				// Jump to application
        jump_to_application();
}

void jump_to_app(uint32_t APPLICATION_BASE)
{
		uint32_t *vectorTable = (uint32_t*)APPLICATION_BASE;
		
    uint32_t sp = vectorTable[0];
    uint32_t pc = vectorTable[1];

    typedef void(*app_entry_t)(void);

    static uint32_t s_stackPointer = 0;
    static uint32_t s_applicationEntry = 0;
    static app_entry_t s_application = 0;

    //bl_deinit_interface();
    s_stackPointer = sp;
    s_applicationEntry = pc;
    printf("sp��%x,pc: %x\r\n",sp,pc);
    s_application = (app_entry_t)s_applicationEntry;

    // Change MSP and PSP
    __set_MSP(s_stackPointer);
    __set_PSP(s_stackPointer);
    
    SCB->VTOR = APPLICATION_BASE;
    
    // Jump to application
    s_application();

    // Should never reach here.
    __NOP();
}





/* ����һ������֡ */
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
/* ����flash */
uint32_t flash_erase(uint32_t addr)
{
		uint32_t count=0;
		FLASH_EraseSector(addr);//ע��
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
uint8_t dataForWrite[4];
/* д��flash */
uint32_t flash_write(const uint8_t *buf, uint32_t len)
{
		uint32_t addr= buf[1]*256*256+buf[2]*256+buf[3];
		dataForWrite[0]=buf[4];
		dataForWrite[1]=buf[5];
		dataForWrite[2]=buf[6];
		dataForWrite[3]=buf[7];
		FLASH_WriteSector(addr,dataForWrite,4);
    if(FLASH_WriteSector(addr, buf, len) == FLASH_OK)
    {
        return BL_FLASH_OK;
    }
    else
    {
        return BL_FLASH_ERR;
    }
}

