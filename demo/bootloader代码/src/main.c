#include "gpio.h"
#include "uart.h"
#include "can.h"
#include "bootloader.h"

#ifdef MK64F12
#error "yandld: MK64 only has CAN0, no CAN1"
#endif

#define CAN_TX_ID  0x12
#define CAN_RX_ID  0

/*���̣�
1������������λ������ 00 04 00 ......
2���������4sû���յ�01 00 ....�����appģʽ
3�������λ������ 01 00 ....����bootloaderģʽ
4����λ���ϴ� 04 00 ˵�����ѽ���bootloaderģʽ
5����λ������ 05 00 ����λ��ִ�в���������������ɺ�����λ������05 00 ��ʾ�������
6����λ������ 02 00 ִ��flashд�룬����λ��д��һ�κ���02 00 ����λ��������������
7����λ����⵽hex����û���ˣ�����03 00����λ��ִ�����ݽ����жϺ󣬷���03 00 ������appģʽ
*/
uint8_t Time_Tasks(void);
uint8_t Tick_Tasks(void);
//��ת��bootloader��ַ
void bootloader_entry()
{
	printf("bootloader\r\n");
	GoToUserAPPorBOOT(Bootloader_StartAddr);
	while(1);/*stop here for anyhow*/
}
//��ת��app��ַ
void app_entry(void)
{
	GoToUserAPPorBOOT(APP_StartAddr);
	while(1);/*stop here for anyhow*/
}
void CAN_ISR(void)
{
    uint8_t buf[8];
    uint8_t len;
    uint32_t id;
		int i = 0;
    if(CAN_ReadData(HW_CAN0, 3, &id, buf, &len) == 0&&id>>CAN_ID_STD_SHIFT ==0x25)
    {
			//printf("ID: %x DATA: %x %x LEN:%x\r\n",id>>CAN_ID_STD_SHIFT,buf[0],buf[1],len);
			AfterBootloader_CmdType result = Bootloader_DataParse_g(buf,len);
			/*EntryBootloader=0,Reset=1,Data=2,DataEnd=3,CheckBootloader=4,Erase=5,ERR=0xff*/
			//printf("result: %x\r\n",result);
			switch (result)
			{
				case EntryBootloader:
					break;
				case Reset:
					printf("bootloader_entry\r\n");
					bootloader_entry();          /*jump to bootloader and should not back */
					break;
				case Data:
					//printf("data_entry\r\n");
					//����λ�����͵ȴ�����ָ��
					if(!flash_write(buf, len))
					{
						CAN_WriteData(HW_CAN0, 0, G_SendCANID, g_Bootloader_DataReponse, 8); 
					}
					GPIO_ToggleBit(HW_GPIOE, 0);
					break;
				case DataEnd:	
					printf("data-end_entry\r\n");
				//��¼��ɷ�����ɱ�־
					app_entry();          /*jump to app and should not back */
					CAN_WriteData(HW_CAN0, 0, G_SendCANID, g_Bootloader_DataProgramEndReponse, 8);
					break;
				case CheckBootloader:
					printf("check_entry\r\n");
					//���Ϸ������ѽ���bootģʽ
					CAN_WriteData(HW_CAN0, 0, G_SendCANID, g_Bootloader_CheckBootloaderReponse, 8);
					break;
				
				case Erase:
					printf("erase_entry\r\n");
				
					for(i=0;i<40;i++)
				{
					flash_erase(APP_StartAddr+256*8*i);
				}
					//������ɱ�־
				if(!flash_erase(APP_StartAddr))
				{
					CAN_WriteData(HW_CAN0, 0, G_SendCANID, g_Bootloader_EraseFlashReponse, 8); 
				}
					break;
				default:
					break;
			}/*end if switch*/
    }
}
uint32_t countTick = 0;
uint8_t Tick_Tasks()
{
    countTick++;
    //ÿ��100��Լ1ms
    if(countTick==100)
    {
        countTick=0;
				printf("In bootloader!\r\n");
        return Time_Tasks();
    }
    return 1;
}

uint32_t countTimer=0;
uint8_t Time_Tasks()
{
    countTimer++;
    if(countTimer>=(1000*g_GOTOAPP_TIMEOUT))
    {
			printf("app entry!\r\n");
			countTimer = 0;
    	app_entry();		
      return 0; 	
		}
    return 1;
}

int main(void)
{
		__enable_irq();
    DelayInit();
		GPIO_QuickInit(HW_GPIOE,0,kGPIO_Mode_OPP);//led��
    UART_QuickInit(UART1_RX_PC03_TX_PC04, 115200);
    CAN_QuickInit(CAN0_TX_PB18_RX_PB19, 125*1000);
		CAN_WriteData(HW_CAN0, 0, G_SendCANID, g_Bootloader_CheckBootloaderReponse, 8);
    CAN_SetRxMB(HW_CAN0, 3, CAN_RX_ID);
			
		uint8_t run=1;
		while(run)
	{
		CAN_ISR(); //can
		run = Tick_Tasks();
	}
    while(1)
    {
			CAN_ISR();
			
    }
}



