#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <time.h>
#include <malloc.h> 
#include <utils/Log.h>
#include "serial.h" 
#include "util.h"
#include "device.h" 
#include "file_op.h" 

#define WRITE			 (1)
#define READ			 (0)
#define NO_CRC_READ		 (1)
#define RE_BUFF_LEN		(16)

#define ERR_SIZE		(-16)

char file_buff[256];
enum {
	MAC_LEN = 8,
};

/*dddd*/


Device::Device()
{
	this->fd = -1;

} 
Device::~Device()
{	
	this->fd = -1 ;
}
/*
 ** if success return this open device fd else error is -1 
 */
int Device::open_tty_device(const char* tty_name )
{	
	this->fd = ngb_Open((char*)tty_name) ;
	if (this->fd < 0) {
	}
	return this->fd ;
}

void Device::close_tty_device(void)
{
	ngb_Close(this->fd) ;
}
/*
 ** return 0 success -1 error 
 */
int Device::set_speed_tty_device(int speed) 
{	
	return ngb_SetSpeed(this->fd,speed) ;
}
/*
 ** return 0 success -1 error 
 */
int Device::set_parity_tty_device(int parity)
{
	return ngb_SetParity(this->fd,parity) ;
}
/*
 ** return 0 success -1 error
 */
int Device::set_bits_tty_device(int bits) 
{
	return ngb_SetBits(this->fd,bits) ;
}
/*
 **  return 0 success -1 error 
 */
int Device::set_stop_bits(int stopbits) 
{
	return ngb_SetStopBits(this->fd,stopbits) ;
}

/*
 ** return 0 success -1 error
 */
int Device::set_flow_control_tty_device(int flow)
{
	return ngb_SetFlowControl(this->fd,flow) ;

}

uchar mac_buff[8];

int Device::operator_device(const char* mac,char type,char cmd,char val)
{
	unsigned char size = 0;
	unsigned char i = 0;
	unsigned char k = 0;
	time_t start = 0;
	time_t end = 0;
	const char *p = mac;
	unsigned char mac_test[8] = {0};
	if((NULL == mac)||(strlen(mac)<1)) {
		return -2 ;
	}
#if WRITE	

	memset(mac_buff, 0, 8);

	ym_data_t out_data  ;
	init_ym_data(&out_data) ;
	/*copy mac type val cmd crc to struct ym_data*/
	COPY_STR2MAC(out_data.mac,(char*)mac) 	;
	COPY_STR2MAC(mac_buff,(char*)mac) 	;
	out_data.type = type ;
	out_data.val = val ;
	out_data.cmd = cmd;
	out_data.crc = get_crc_result((uchar*)&out_data, (sizeof(ym_data_t) - 2)) ;
	if( (size = ngb_Write( this->fd,&out_data,sizeof(ym_data_t) ) ) < 0) {
		return -3 ;
	}
	if(size != 16) {
		return ERR_SIZE;
	}
#endif /*WRITE*/

#if NO_CRC_READ
	ym_data_t *in_data=NULL ; //Input data buff. 
	int read_size = -1;
	int j = 0;
	int n = 0;
	char read_buff[16];
	char js_buff[1] = {0};

	memset(read_buff, 0, 16);
here:
	for(j = 0; j < 16; j++) {
		if( (read_size = ngb_Read(this->fd, js_buff, 1))  < 0) {
			return (-1);
		}
		read_buff[j] = js_buff[0];
		if(read_buff[0] != 0xFE) {
			j = 0;
		}
	}
	if(read_buff[0] != 0xFE || read_buff[15] != 0x23) {
		return -500;
	}


	/*test mac address*/	
	for(k = 0; k < 8; k++) {
		if(mac_buff[k] != read_buff[k+1]) {
			if(n != 3) {
				n++;
				goto here;
			}else{
				return -300;
			}
		}
	} /*end for*/

	in_data = (ym_data_t *)read_buff;
	return in_data->val;

#endif /*NO_CRC_READ*/


	return (size * 100) ; //returen write_size 100 p, raoguo 0~100
}


#define		RET_SIZE	(256)
#define		TMP_SIZE	(4)
#define		MAC_SIZE	(8)
#define		TYPE_LOCA	(9)
#define		CMD_LOCA	(10)
#define		VAL_LOCA	(11)
/*List COO device list*/
char list_buff[RET_SIZE] ;
char * Device::list_device(void)
{
	ym_data_t *in_data=NULL ; //Input data buff. 
	int read_size = -1;
	int j = 0;
	int n = 0;
	unsigned char read_buff[16];
	char temp_buff[TMP_SIZE];
	unsigned char js_buff[1] = {0};
	memset(read_buff, 0, 16);
	memset(list_buff, 0, RET_SIZE);

	for(j = 0; j < 16; j++) {
		if( (read_size = ngb_Read(this->fd, js_buff, 1))  < 0) {
			return (char*)"null";
		}
		read_buff[j] = js_buff[0];
		if(read_buff[0] != 0xFE) {
			j = 0;
		}
	}
	if(read_buff[0] != 0xFE || read_buff[15] != 0x23) {
		return (char*)"null";
	}
	/*test mac address*/	
	for(n = 0; n < MAC_SIZE; n++){
		memset(temp_buff, 0, TMP_SIZE);
		sprintf(temp_buff, "%02X", read_buff[n+1]);
		strcat(list_buff, temp_buff);
		if(n == 7) {
			strcat(list_buff, "_");
			/*type*/			
			memset(temp_buff, 0, TMP_SIZE);
			sprintf(temp_buff, "%d", read_buff[TYPE_LOCA]);
			strcat(list_buff, temp_buff);
			strcat(list_buff, "_");
			/*CMD*/
			memset(temp_buff, 0, TMP_SIZE);
			sprintf(temp_buff, "%d", read_buff[CMD_LOCA]);
			strcat(list_buff, temp_buff);
			strcat(list_buff, "_");
			/*VAL*/
			memset(temp_buff, 0, TMP_SIZE);
			sprintf(temp_buff, "%d", read_buff[VAL_LOCA]);
			strcat(list_buff, temp_buff);
			break;
		} 
		strcat(list_buff, ":");
	}
	return list_buff;

}


