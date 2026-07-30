/********************************************************************************************************
 * @file    app_ui.h
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
 * 本文件声明了 BLE Remote 用户界面（按键、LED、OTA 提示）相关的对外接口，包括 UI 初始化、
 * 按键扫描回调及 OTA 状态提示函数等。
 */
#ifndef APP_UI_H_
#define APP_UI_H_


/* 中文说明：MCU 正常上电/深睡唤醒时的 UI 初始化，配置按键唤醒 GPIO、LED 初始状态等。 */
void app_ui_init_normal(void);
/* 中文说明：MCU 从深度睡眠 retention 模式唤醒时的 UI 初始化（仅恢复按键唤醒 GPIO 与 LED）。 */
void app_ui_init_deepRetn(void);


/**
 * @brief      this function is used to detect if key pressed or released.
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
/* 中文说明：按键矩阵扫描入口，检测按键按下/释放状态变化，并驱动 IR 学习状态、音频状态检测等联动逻辑。 */
void proc_keyboard(u8 e, u8 *p, int n);





#if (BLE_REMOTE_OTA_ENABLE)
	void app_enter_ota_mode(void);
	void app_debug_ota_result(int result);
#endif



extern 	u8 		key_type;
extern	int 	key_not_released;

extern	int 	ir_not_released;
extern	u8 		user_key_mode;
extern	u8      ir_hw_initialed;
extern	u8 		ota_is_working;
extern	int		lowBatt_alarmFlag;











#endif /* APP_UI_H_ */
