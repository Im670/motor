#ifndef _CMD_PROTO_H_
#define _CMD_PROTO_H_


#define TX_DATA_LEN  (2)
#define RX_DATA_LEN  (7)

#define TX_HEAD    (0x55)
#define RX_HEAD    (0xAA)


/*-------------------
LED_Data：    bit5~bit0   分别对应6个LED， 0为灭，1为亮
LED是否闪烁:  bit5~bit0  分别对应6个LED， 0为不闪烁，1为闪烁
-----------------------*/
typedef struct
{
	u8 head;   //0x55 固定值
	u8 data[TX_DATA_LEN];
	u8 chksum;
}led_ctrl_cmd_t;


typedef struct
{
	u8 head;   //0xAA 固定值
	u8 data[MOTOR_CHN_NUM];
	u8 para;   //参数 
	u8 chksum;
	//u8 chksum2;
}motor_ctrl_cmd_t;


void proto_init(void);
int proto_read_data(u8 *pdata ,u8 len);
int proto_send_data(u8 len_en,u8 led_flash);
int proto_proc_data(motor_ctrl_cmd_t *pm_ctrl);
void proto_start_recv(void);
int is_proto_sending(void);
int is_proto_send_finished(void);
void proto_test(void);

#endif


