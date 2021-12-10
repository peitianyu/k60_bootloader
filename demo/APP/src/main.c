#include "gpio.h"
#include "uart.h"
#include "can.h"
#include "bootloader.h"

#ifdef MK64F12
#error "yandld: MK64 only has CAN0, no CAN1"
#endif

#define CAN_TX_ID  0x10
#define CAN_RX_ID  0

uint8_t buf[8];

//跳转到bootloader地址
void bootloader_entry()
{
	printf("bootloader\r\n");
	GoToUserAPPorBOOT(Bootloader_StartAddr);
	while(1);/*stop here for anyhow*/
}

void CAN_ISR(void)
{
    
    uint8_t len;
    uint32_t id;	
		printf("enisr\r\n");
    if(CAN_ReadData(HW_CAN0, 3, &id, buf, &len) == 0&&id>>CAN_ID_STD_SHIFT ==0x25)
    {
			printf("enisr3\r\n");
    }
}


int main(void)
{
	//跳转过来后，注意开总中断
		__enable_irq();
		uint8_t data[8] = {6,0,0,0,0,0,0,0};
    DelayInit();
		GPIO_QuickInit(HW_GPIOE,0,kGPIO_Mode_OPP);//led灯
    UART_QuickInit(UART1_RX_PC03_TX_PC04, 115200);
    
    CAN_QuickInit(CAN0_TX_PB18_RX_PB19, 125*1000);
    CAN_CallbackInstall(HW_CAN0, CAN_ISR);
    CAN_ITDMAConfig(HW_CAN0,3, kCAN_IT_RX);
    CAN_SetRxMB(HW_CAN0, 3, CAN_RX_ID);
		CAN_WriteData(HW_CAN0, 2, CAN_TX_ID, data, 8); /* 使用邮箱2 发送ID:0x10 发送 06 00 00 00 表示进入APP */
		printf("APP entry!\r\n");
	
    while(1)
    {
			//只能在主函数中进行跳转，若在中断中可能出现不进入中断情况
				if(buf[0]==1)
				{
						bootloader_entry();
						buf[0]=0;
				}
        DelayMs(200);
				GPIO_ToggleBit(HW_GPIOE, 0);

    }
}


