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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"
#include "application/usbstd/usbkeycode.h"

#include "app.h"
#include "app_att.h"
#include "app_ui.h"

/*
 * 中文说明：
 * 本文件实现应用层的人机交互(UI)逻辑，包括 OTA 进入/结束回调、键盘矩阵扫描
 * 变化处理以及独立按键检测与防抖处理。根据 app_config.h 中
 * UI_KEYBOARD_ENABLE / UI_BUTTON_ENABLE 的配置二选一启用相应代码分支，
 * 检测到按键事件后通过 GATT Notify 推送 HID 报告给主机。本文件与芯片型号无关。
 */

extern u32  latest_user_event_tick;
#if(TEST_2P4G_MODE)
u8      reboot_flag_t=0;
#endif
#if (BLE_OTA_SERVER_ENABLE)
/**
 * @brief      this function is used to register the function for OTA start.
 * @param[in]  none
 * @return     none
 * 中文说明：OTA 开始时的回调函数，标记 ota_is_working 为真并刷新最后用户事件
 * 时间戳，供电源管理模块避免在 OTA 进行中进入深度睡眠。
 */
void app_enter_ota_mode(void)
{
	tlkapi_send_string_data(APP_OTA_LOG_EN, "[APP][OTA] enter ota mode", 0, 0);
	ota_is_working = 1;
	latest_user_event_tick = clock_time();
}


/**
 * @brief      this function is used to register the function for OTA end.
 * @param[in]  result - OTA result
 * @return     none
 * 中文说明：OTA 结束时的回调函数。仅在调试开关打开时通过蓝色 LED 闪烁提示
 * OTA 成功/失败，默认情况下该调试代码被 #if(0 ...) 关闭不执行。
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
 * 中文说明：处理键盘矩阵状态变化。刷新最后用户事件时间戳；当只有单键按下时，
 * 区分音量键(CR_VOL_UP/DN)与普通键盘按键，分别通过 HID 消费电子报告/键盘报告
 * 推送 Notify 给主机；当所有按键释放时，推送对应的释放(数值为0)报告。
 * 当 TEST_2P4G_MODE 使能时，每次单键按下会翻转工作模式标志位，释放时若检测到标志
 * 与当前工作模式不一致则置位重启标志，切换 BLE/2.4G 私有协议工作模式。
 */
void key_change_proc(void)
{

	latest_user_event_tick = clock_time();  //record latest key change time


	u8 key0 = kb_event.keycode[0];
	u8 key_buf[8] = {0,0,0,0,0,0,0,0};

	key_not_released = 1;
	if (kb_event.cnt == 2)   //two key press, do  not process
	{

	}
	else if(kb_event.cnt == 1)
	{

        #if(TEST_2P4G_MODE)
        analog_write(USED_DEEP_ANA_REG,  analog_read(USED_DEEP_ANA_REG) ^ (RF_WORKING_MODE));   // change the working mode of RF
        #endif

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
        #if(TEST_2P4G_MODE)
        if( ( (analog_read(USED_DEEP_ANA_REG) & (RF_WORKING_MODE)) != rf_working_mode) ){
            reboot_flag_t=1;            // ready for reboot
        }
        #endif
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
 * 中文说明：键盘扫描入口函数，每 8ms 执行一次矩阵扫描，若检测到按键变化
 * 则调用 key_change_proc 处理具体的按键/释放逻辑。
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
	 * 中文说明：按键防抖处理。将最新采样值推入 4 帧历史缓冲，只有连续三次采样
	 * 一致且与上一次确认的稳定值不同时，才判定为一次有效的按键状态变化。
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
	 * 中文说明：读取所有按键 GPIO 电平得到当前按键位图，经防抖处理后，若
	 * read_key 为真且检测到变化，则填充当前按下的按键列表并返回 1。
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
	 * 中文说明：独立按键检测处理函数。检测到按键变化后，单键按下时发送
	 * 对应的音量加/减 HID 消费电子报告，释放时发送数值为 0 的释放报告。
	 */
	void proc_button(u8 e, u8 *p, int n)
	{

		int det_key = vc_detect_button(1);

		if (det_key)  //key change: press or release
		{
			extern u32 latest_user_event_tick;
			latest_user_event_tick = clock_time();

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
