/********************************************************************************************************
 * @file    app_ui.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
/* 中文说明：本文件实现应用层 UI 交互相关逻辑，包括可选的 OTA 开始/结束回调、键盘扫描处理
 * （UI_KEYBOARD_ENABLE）以及按键去抖与事件上报（UI_BUTTON_ENABLE）。本工程默认两者均未使能（仅
 * UI_LED_ENABLE 使能），本文件无芯片区分，对 B85/B87/TC321X 均适用。
 */
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"
#include "application/usbstd/usbkeycode.h"

#include "app.h"
#include "app_att.h"
#include "app_ui.h"

#if (BLE_OTA_SERVER_ENABLE)
/**
 * @brief      this function is used to register the function for OTA start.
 * @param[in]  none
 * @return     none
 */
/* 中文说明：进入 OTA 模式时的回调。标记 ota_is_working 为真，并关闭 BLE/2.4G 并发模式，
 * 避免 OTA 写 flash 期间受到并发任务干扰。
 */
void app_enter_ota_mode(void)
{
	tlkapi_send_string_data(APP_OTA_LOG_EN, "[APP][OTA] enter ota mode", 0, 0);
	ota_is_working = 1;
	blc_ll_disableConcurrentMode();
}


/**
 * @brief      this function is used to register the function for OTA end.
 * @param[in]  result - OTA result
 * @return     none
 */
/* 中文说明：OTA 结束结果回调。默认该调试 LED 闪烁代码处于关闭状态（#if(0 && UI_LED_ENABLE)），
 * 实际不会编译进去，仅作为调试参考。
 */
void app_ota_end_result(int result)
{
	#if(0 && UI_LED_ENABLE)  //this is only for debug
		if(result == OTA_SUCCESS){  //led for debug: OTA success
			gpio_write(GPIO_LED_BLUE, 1);
			sleep_ms(300);
			gpio_write(GPIO_LED_BLUE, 0);
			sleep_ms(300);
			gpio_write(GPIO_LED_BLUE, 1);
			sleep_ms(300);
			gpio_write(GPIO_LED_BLUE, 0);

			/* attention that this log may lost, because MCU will reboot for successful OTA */
			//tlkapi_send_string_data(APP_OTA_LOG_EN, "[APP][OTA] ota success", &result, 1);
		}
		else{  //OTA fail
			gpio_write(GPIO_LED_BLUE, 1);
			sleep_ms(100);
			gpio_write(GPIO_LED_BLUE, 0);

			/* attention that this log may lost */
			//tlkapi_send_string_data(APP_OTA_LOG_EN, "[APP][OTA] ota fail", &result, 1);
		}
	#endif
}
#endif

#if (UI_KEYBOARD_ENABLE)



_attribute_data_retention_	int 	key_not_released;
_attribute_data_retention_	u8 		key_type;
_attribute_data_retention_	static u32 keyScanTick = 0;


#define CONSUMER_KEY   	   		1
#define KEYBOARD_KEY   	   		2







/**
 * @brief		this function is used to process keyboard matrix status change.
 * @param[in]	none
 * @return      none
 */
/* 中文说明：处理键盘矩阵按键状态变化。同时按下两键不处理；单键按下时区分音量键（consumer）
 * 与普通键盘键（keyboard）两种类型，分别通过 HID 消费者/键盘报告特征发送通知；所有键释放
 * （cnt==0）时分别发送对应类型的释放报告（数据清零）。
 */
void key_change_proc(void)
{


	u8 key0 = kb_event.keycode[0];
	u8 key_buf[8] = {0,0,0,0,0,0,0,0};

	key_not_released = 1;
	if (kb_event.cnt == 2)   //two key press, do  not process
	{

	}
	else if(kb_event.cnt == 1)
	{
		if(key0 >= CR_VOL_UP )  //volume up/down
		{
			key_type = CONSUMER_KEY;
			u16 consumer_key;
			if(key0 == CR_VOL_UP){  	//volume up
				consumer_key = MKEY_VOL_UP;
				tlkapi_send_string_data(APP_KEY_LOG_EN, "[UI][KEY] send Vol+", 0, 0);
				#if (UI_LED_ENABLE)
					gpio_write(GPIO_LED_WHITE,1);
				#endif
			}
			else if(key0 == CR_VOL_DN){ //volume down
				consumer_key = MKEY_VOL_DN;
				tlkapi_send_string_data(APP_KEY_LOG_EN, "[UI][KEY] send Vol-", 0, 0);
				#if (UI_LED_ENABLE)
					gpio_write(GPIO_LED_GREEN,1);
				#endif
			}

			blc_gatt_pushHandleValueNotify (BLS_CONN_HANDLE, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
		}
		else
		{
			key_type = KEYBOARD_KEY;
			key_buf[2] = key0;
			if(key0 == VK_1)
			{
				#if (UI_LED_ENABLE)
					gpio_write(GPIO_LED_BLUE,1);
				#endif
			}
			else if(key0 == VK_2)
			{
				#if (UI_LED_ENABLE)
					gpio_write(GPIO_LED_BLUE,1);
				#endif
			}

			blc_gatt_pushHandleValueNotify (BLS_CONN_HANDLE, HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8);
		}

	}
	else   //kb_event.cnt == 0,  key release
	{
		#if (UI_LED_ENABLE)
			gpio_write(GPIO_LED_WHITE,0);
			gpio_write(GPIO_LED_GREEN,0);
		#endif
		key_not_released = 0;
		if(key_type == CONSUMER_KEY)
		{
			u16 consumer_key = 0;

			blc_gatt_pushHandleValueNotify (BLS_CONN_HANDLE, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
		}
		else if(key_type == KEYBOARD_KEY)
		{
			#if (UI_LED_ENABLE)
				gpio_write(GPIO_LED_BLUE,0);
				gpio_write(GPIO_LED_BLUE,0);
			#endif
			key_buf[2] = 0;

			blc_gatt_pushHandleValueNotify (BLS_CONN_HANDLE, HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8); //release
		}
	}


}



/**
 * @brief      this function is used to detect if key pressed or released.
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
/* 中文说明：键盘扫描入口。以 8ms 为最小间隔限速扫描（避免频繁调用），扫描到按键变化后
 * 调用 key_change_proc() 处理。
 */
void proc_keyboard(u8 e, u8 *p, int n)
{
	if(clock_time_exceed(keyScanTick, 8000)){
		keyScanTick = clock_time();
	}
	else{
		return;
	}

	kb_event.keycode[0] = 0;
	int det_key = kb_scan_key (0, 1);

	if (det_key){
		key_change_proc();
	}
}



#elif (UI_BUTTON_ENABLE)



/////////////////////////////////////////////////////////////////////
	#define MAX_BTN_SIZE			2
	#define BTN_VALID_LEVEL			0

	#define USER_BTN_1				0x01
	#define USER_BTN_2				0x02

	_attribute_data_retention_	u32 ctrl_btn[] = {SW1_GPIO, SW2_GPIO};
	_attribute_data_retention_	u8 btn_map[MAX_BTN_SIZE] = {USER_BTN_1, USER_BTN_2};



	_attribute_data_retention_	int button_not_released = 0;

	_attribute_data_retention_	static int button0_press_flag;
	_attribute_data_retention_	static u32 button0_press_tick;
	_attribute_data_retention_	static int button1_press_flag;
	_attribute_data_retention_	static u32 button1_press_tick;

	_attribute_data_retention_	static int consumer_report = 0;
	/**
	 * @brief 	record the result of key detect
	 */
	typedef	struct{
		u8 	cnt;				//count button num
		u8 	btn_press;
		u8 	keycode[MAX_BTN_SIZE];			//6 btn
	}vc_data_t;
	_attribute_data_retention_	vc_data_t vc_event;

	/**
	 * @brief 	record the status of button process
	 */
	typedef struct{
		u8  btn_history[4];		//vc history btn save
		u8  btn_filter_last;
		u8	btn_not_release;
		u8 	btn_new;					//new btn  flag
	}btn_status_t;
	_attribute_data_retention_	btn_status_t 	btn_status;

	/**
	 * @brief      Debounce processing during button detection
	 * @param[in]  btn_v - vc_event.btn_press
	 * @return     1:Detect new button;0:Button isn't changed
	 */
	/* 中文说明：按键去抖。将最新采样值存入 4 帧历史缓冲区，只有当连续 3 次采样一致
	 * 且与上一次确认的值不同时，才认为按键状态真正发生了变化（返回 1），以此消除机械抚动影响。
	 */
	u8 btn_debounce_filter(u8 *btn_v)
	{
		u8 change = 0;

		for(int i=3; i>0; i--){
			btn_status.btn_history[i] = btn_status.btn_history[i-1];
		}
		btn_status.btn_history[0] = *btn_v;

		if(  btn_status.btn_history[0] == btn_status.btn_history[1] && btn_status.btn_history[1] == btn_status.btn_history[2] && \
			btn_status.btn_history[0] != btn_status.btn_filter_last ){
			change = 1;

			btn_status.btn_filter_last = btn_status.btn_history[0];
		}

		return change;
	}

	/**
	 * @brief      This function is key detection processing
	 * @param[in]  read_key - Decide whether to return the key detection result
	 * @return     1:Detect new button;0:Button isn't changed
	 */
	/* 中文说明：检测所有按键当前电平，根据 BTN_VALID_LEVEL（低电平有效）判断哪些按键被按下，
	 * 经去抖处理后，若状态发生变化且 read_key 为真，则填充 vc_event 中的按键码列表并返回 1。
	 */
	u8 vc_detect_button(int read_key)
	{
		u8 btn_changed, i;
		memset(&vc_event,0,sizeof(vc_data_t));			//clear vc_event
		//vc_event.btn_press = 0;

		for(i=0; i<MAX_BTN_SIZE; i++){
			if(BTN_VALID_LEVEL != !gpio_read(ctrl_btn[i])){
				vc_event.btn_press |= BIT(i);
			}
		}

		btn_changed = btn_debounce_filter(&vc_event.btn_press);


		if(btn_changed && read_key){
			for(i=0; i<MAX_BTN_SIZE; i++){
				if(vc_event.btn_press & BIT(i)){
					vc_event.keycode[vc_event.cnt++] = btn_map[i];
				}
			}

			return 1;
		}

		return 0;
	}


	/**
	 * @brief		this function is used to detect if button pressed or released.
	 * @param[in]	e - event type when this function is triggered by LinkLayer event
	 * @param[in]	p - event callback data pointer for when this function is triggered by LinkLayer event
	 * @param[in]	n - event callback data length when this function is triggered by LinkLayer event
	 * @return      none
	 */
	/* 中文说明：按键事件处理。检测到按键变化后，单键按下时根据按键编号发送音量加/减的 HID
	 * 消费者报告并记录按下时间戳；当两键同时按下时不处理；当所有键释放时，若之前已发送过
	 * 消费者报告，则发送释放（值为 0）报告。
	 */
	void proc_button(u8 e, u8 *p, int n)
	{

		int det_key = vc_detect_button(1);

		if (det_key)  //key change: press or release
		{
			u8 key0 = vc_event.keycode[0];
//			u8 key1 = vc_event.keycode[1];

			button_not_released = 1;

			if(vc_event.cnt == 2)  //two key press
			{

			}
			else if(vc_event.cnt == 1) //one key press
			{
				if(key0 == USER_BTN_1)
				{
					button0_press_flag = 1;
					button0_press_tick = clock_time();

					//send "Vol+"
					u16 consumer_key = MKEY_VOL_UP;
					blc_gatt_pushHandleValueNotify (BLS_CONN_HANDLE, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
					consumer_report = 1;

				}
				else if(key0 == USER_BTN_2)
				{
					button1_press_flag = 1;
					button1_press_tick = clock_time();

					//send "Vol-"
					u16 consumer_key = MKEY_VOL_DN;
					blc_gatt_pushHandleValueNotify (BLS_CONN_HANDLE, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
					consumer_report = 1;
				}
			}
			else{  //release
				button_not_released = 0;

				button0_press_flag = 0;
				button1_press_flag = 0;

				//send release Vol+/Vol-
				if(consumer_report){
					consumer_report = 0;
					u16 consumer_key = 0;
					blc_gatt_pushHandleValueNotify (BLS_CONN_HANDLE, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
				}
			}

		}


	}
#endif   //end of UI_BUTTON_ENABLE
