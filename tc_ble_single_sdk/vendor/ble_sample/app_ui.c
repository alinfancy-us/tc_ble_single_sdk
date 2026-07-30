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
 * 中文说明：本文件实现 ble_sample 示例的人机交互（UI）处理逻辑，包含 OTA 启动/
 * 结果回调（可选）、键盘按键扫描与上报（UI_KEYBOARD_ENABLE）以及按钮消抖/上报
 * （UI_BUTTON_ENABLE）两套互斥的实现。上报数据通过 GATT Notify 推送给主机，
 * 与具体 MCU 平台无关。
 */

extern u32  latest_user_event_tick;

#if (BLE_OTA_SERVER_ENABLE)
/**
 * @brief      this function is used to register the function for OTA start.
 * @param[in]  none
 * @return     none
 *
 * 中文说明：OTA 开始回调。标记 ota_is_working 并刷新最近事件时间戳，
 * 防止 OTA 进行中因无操作超时而进入 deepSleep。
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
 *
 * 中文说明：OTA 结束回调。默认仅在 `#if(0 && UI_LED_ENABLE)` 调试开关下用 LED 闪烁
 * 提示 OTA 成功/失败（常规编译时该分支不会被编译进去）。
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
 *
 * 中文说明：键盘矩阵状态发生变化时的处理函数。同时按两键不处理；单键按下时
 * 区分多媒体/消费类按键（音量加减）与普通键盘按键，分别通过 HID Consumer/
 * Keyboard Input Report 特征值 Notify 上报；键释放时根据之前记录的键类型上报释放
 * （清 0）报告。
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
 *
 * 中文说明：键盘按键检测入口，以 8ms 为间隔进行按键扫描防抖，检测到改变时
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
	 *
	 * 中文说明：按钮消抖处理。将本次按钮采样值推入历史缓冲，只有连续 3 次采样
	 * 一致且与上一次确认值不同时，才判定为有效的按钮状态变化，避免机械抖动
	 * 引起误触发。
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
	 *
	 * 中文说明：逐个读取按钮 GPIO 电平得到当前按下位图，经 3 次采样消抖后，
	 * 若 read_key 为真且确实发生变化，则填充 vc_event 中的按钮编码列表并返回 1。
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
	 *
	 * 中文说明：按钮按下/释放处理入口。同时按两键不处理；单键按下时区分
	 * USER_BTN_1（音量+）/USER_BTN_2（音量-）并通过 Consumer Report Notify 上报；
	 * 全部释放时清除按下标志并在之前有上报过消费报告时补一次释放（值 0）上报。
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
