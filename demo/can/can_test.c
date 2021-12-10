#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <pthread.h>
#include "can.h"

#define PF_CAN 29

#define AF_CAN PF_CAN

#define SIOCSCANBAUDRATE        (SIOCDEVPRIVATE+0)

#define SIOCGCANBAUDRATE        (SIOCDEVPRIVATE+1)

#define SOL_CAN_RAW (SOL_CAN_BASE + CAN_RAW)
#define CAN_RAW_FILTER  1
#define CAN_RAW_RECV_OWN_MSGS 0x4 

#define N 100

#define data_send  		0x02
#define data_end  		0x03
#define boot_check  	0x04
#define flash_erase  	0x05

typedef __u32 can_baudrate_t;

struct ifreq ifr;
int hex[100];

int *str2hex(char* str)
{
 
    int k0,k1;
    int i=0;
    for (i = 1; i < strlen(str)-2; i++)
        {
            k0 = str[i]-48;           
            if(str[i]>='A' && str[i]<='F')
                k0=str[i]-55;
            if (i%2==0)
            {
                //printf("\n");
                hex[i/2-1]=k1*16+k0;
                //printf("0x%x",hex[i/2]);
            } 
             
            k1=k0;                   
        }
    return hex;
}
int init_can(char* can) {
	int sock;
	struct sockaddr_can addr;

	sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(sock < 0) {
		printf("error\n");
		return -1;
	}

	addr.can_family = AF_CAN;
	strcpy(ifr.ifr_name, can );

	int ret;

	ret = ioctl(sock, SIOCGIFINDEX, &ifr);  //get index
	if(ret && ifr.ifr_ifindex == 0) {
		printf("Can't get interface index for can0, code= %d, can0 ifr_ifindex value: %d, name: %s\n", ret, ifr.ifr_ifindex, ifr.ifr_name);
		close(sock);
		return -1;
	}

	printf("%s can_ifindex = %x\n",ifr.ifr_name,ifr.ifr_ifindex);
	addr.can_ifindex = ifr.ifr_ifindex;

	int recv_own_msgs = 0;//set loop back:  1 enable 0 disable

	setsockopt(sock, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,&recv_own_msgs, sizeof(recv_own_msgs));
	if (bind(sock,(struct sockaddr*)&addr,sizeof(addr))<0) {
		printf("bind error\n");
		close(sock);
		return -1;
	}

	return sock;
}

struct can_frame Can_Read_thread(void* psock)
{
	int sock = *(int *)psock;
	int i = 0;
	struct can_frame frame;
	while(1) {
        	memset(&frame,0,sizeof(struct can_frame));
		read(sock,&frame,sizeof(struct can_frame));
		
		if(frame.can_dlc)
		{
			// printf("\n%s DLC:%d ID:%d Data:", ifr.ifr_name, frame.can_dlc, frame.can_id);		
			// for(i = 0; i < frame.can_dlc; i++)
			// {
			// 	printf("%#x ",frame.data[i]);
			// }
			// printf("\n");
			return frame;
    	}
	}	
}

int Write_Can_Data(int sock,char* str,int len)
{
	struct can_frame frame;
	int i;
	int nbytes = 0;

	frame.can_id = 0x25; //can device id
	//printf("len: %d\r\n", len);
	while(len) {

		if(len > sizeof(frame.data)) {
			memset(&frame.data,0,sizeof(frame.data));
			memcpy(&frame.data,str,sizeof(frame.data));
			frame.can_dlc = sizeof(frame.data);
			str += sizeof(frame.data);
			len -= sizeof(frame.data);
			//printf("len: %d", frame.can_dlc);
		}
		else {
			memset(&frame.data,0,sizeof(frame.data));
			memcpy(&frame.data,str,len);
			frame.can_dlc = len;
			str = NULL;
			len = 0;
		}
		write(sock, &frame, sizeof(struct can_frame));
		usleep(100);
	}
}

int main(int argc ,char** argv)
{
	struct can_frame data_frame;
	FILE *fp;
	char* file = argv[2];
	int sock = init_can(argv[1]);
	char str[N + 1];
	int* h;
	int state;
	char data[8]={0,0,0,0,0,0,0,0};
	    //判断文件是否打开与can口打开
    if ( (fp = fopen(file, "rt")) == NULL || sock < 0) 
    {
        puts("Fail to open file!");
        exit(0);
        return 0;
    }
    data[0]=1;
	Write_Can_Data(sock,data,sizeof(data));
	data[0]=0;

	while(1) {
		data[0]=0;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		data[4]=0;
		data[5]=0;
		data[6]=0;
		data[7]=0;
		data_frame = Can_Read_thread(&sock);
		state = data_frame.data[0];
		if (data_frame.can_id==0x12)
		{
			switch(state)
			{
				case flash_erase:
					printf("flash_erase ok!\n");
					state = data_send;
				case data_send:	
					while(1)
					{
						if( fgets(str, N, fp) != NULL ) {
					        h=str2hex(str);
					        if (h[0]==0x10)
					        {
					        	data[0]=0x02;
					        	data[1]=0;
					        	data[2]=h[1];
					        	data[3]=h[2];
					    		data[4]=h[4];
					        	data[5]=h[5];
					        	data[6]=h[6];
					        	data[7]=h[7];
					        	Write_Can_Data(sock,data,sizeof(data));
					        	data[1]=1;
					        	data[2]=h[1];
					        	data[3]=h[2]+4;
					        	data[4]=h[8];
					        	data[5]=h[9];
					        	data[6]=h[10];
					        	data[7]=h[11];
					        	Write_Can_Data(sock,data,sizeof(data));
					        	data[1]=2;
					        	data[2]=h[1];
					        	data[3]=h[2]+8;
					        	data[4]=h[12];
					        	data[5]=h[13];
					        	data[6]=h[14];
					        	data[7]=h[15];
					        	Write_Can_Data(sock,data,sizeof(data));
					        	data[1]=3;
					        	data[2]=h[1];
					        	data[3]=h[2]+12;
					        	data[4]=h[16];
					        	data[5]=h[17];
					        	data[6]=h[18];
					        	data[7]=h[19];
					        	Write_Can_Data(sock,data,sizeof(data));
					        	break;
					        }
					        else if (h[0]==0x08||h[0]==0x04)
					        {
					        	data[0]=3;
								Write_Can_Data(sock,data,sizeof(data));
								printf("bootloader ok!\n");	
								break;				        	
					        }
						}
					}break;
				case boot_check:
					data[0]=5;
					Write_Can_Data(sock,data,sizeof(data));
					printf("bootloader entry!\n");
				break;
			}
		}
		if (data_frame.can_id==0x10&&data_frame.data[0]==6)
		{
			printf("APP up!\n");
			break;
		}
	}
	close(sock);
	printf("ok\n");	
	return 0;
}
