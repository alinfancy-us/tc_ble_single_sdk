/********************************************************************************************************
 * @file    rc_ir.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2020
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
/*
 * 中文说明：
 * 本文件声明了 BLE Remote 红外发射（NEC 协议）相关的常量、控制结构体与接口函数，用于将按键
 * 转换为标准 NEC 红外码并通过 PWM DMA FIFO 方式发送，同时支持长按自动重复发送。
 */
#ifndef RC_IR_H_
#define RC_IR_H_




#define IR_CARRIER_FREQ					38000  	// 1 frame -> 1/38k -> 1000/38 = 26 us
#define PWM_CARRIER_CYCLE_TICK			( CLOCK_SYS_CLOCK_HZ/IR_CARRIER_FREQ )  //16M: 421 tick, f = 16000000/421 = 38004,T = 421/16=26.3125 us
#define PWM_CARRIER_HIGH_TICK			( PWM_CARRIER_CYCLE_TICK/3 )   // 1/3 duty

#define PWM_CARRIER_HALF_CYCLE_TICK		(PWM_CARRIER_CYCLE_TICK>>1)


#define IR_HIGH_CARR_TIME			565			// in us
#define IR_HIGH_NO_CARR_TIME		1685
#define IR_LOW_CARR_TIME			560
#define IR_LOW_NO_CARR_TIME			565
#define IR_INTRO_CARR_TIME			9000
#define IR_INTRO_NO_CARR_TIME		4500

#define IR_SWITCH_CODE              0x0d
#define IR_ADDR_CODE                0x00
#define IR_CMD_CODE                 0xbf

#define IR_REPEAT_INTERVAL_TIME     40500
#define IR_REPEAT_NO_CARR_TIME      2250
#define IR_END_TRANS_TIME			563

//#define IR_CARRIER_FREQ				37917//38222
#define IR_CARRIER_DUTY				3
#define IR_LEARN_SERIES_CNT     	160




enum{
	IR_SEND_TYPE_TIME_SERIES,
	IR_SEND_TYPE_BYTE,
	IR_SEND_TYPE_HALF_TIME_SERIES,
};

/**
 * @brief	The structure for some control parameters or status related to IR functionality.
 */
typedef struct{
	u32 cycle;
	u16 hich;
	u16 cnt;
}ir_ctrl_t;
/**
 * @brief	The structure used to controlling and sending IR signals
 */
typedef struct{
	ir_ctrl_t *ptr_irCtl;
	u8 type;
	u8 start_high;
	u8 ir_number;
	u8 code;
}ir_send_ctrl_data_t;


#define IR_GROUP_MAX		8

#define IR_SENDING_NONE  		0
#define IR_SENDING_DATA			1
#define IR_SENDING_REPEAT		2
/**
 * @brief	The structure used to IR communication control
 */
typedef struct{
	u8 is_sending;
	u8 repeat_enable;
	u8 last_cmd;
	u8 rsvd;

	u32 sending_start_time;
}ir_send_ctrl_t;

ir_send_ctrl_t ir_send_ctrl;

/**
 * @brief      this function is used to init IR
 * @param[in]  none
 * @return     none
 */
/* 中文说明：初始化红外发射用的 PWM 波形参数（NEC 协议的逻辑 0/1、起始位、停止位、重复码等时序），B85/B87
 * 使用 PWM0_N 引脚复用方式，TC321X 使用专用红外发射硬件模块。 */
void rc_ir_init(void);

/**
 * @brief      this function is used to stop IR sending
 * @param[in]  none
 * @return     none
 */
/* 中文说明：按键释放时调用，停止当前红外发送（含重复码发送）并关闭相关中断。 */
void ir_send_release(void);

/**
 * @brief      this function is used to check IR is sending
 * @param[in]  none
 * @return     1 - IR is sending
 * 			   0 - IR is not sending
 */
/* 中文说明：查询当前是否仍在发送红外码，若发送超时（300ms）会自动停止发送。 */
int ir_sending_check(void);

/**
 * @brief      this function is used to do IR sending
 * @param[in]  addr1 - the first part of the NEC address.
 * @param[in]  addr2 - the second part of the NEC address.
 * @param[in]  cmd - the command code to be transmitted.
 * @return     none
 */
/* 中文说明：按 NEC 协议组装地址与命令（含命令取反校验字节）为 PWM 波形序列并启动 DMA 发送；
 * 若与上一次发送的命令相同则忽略（去抖/防重复触发）。 */
void ir_nec_send(u8 addr1, u8 addr2, u8 cmd);




#endif /* RC_IR_H_ */
