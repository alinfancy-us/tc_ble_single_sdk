/********************************************************************************************************
 * @file    app.h
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
 * 本文件声明了 BLE Remote 应用层的核心入口函数，包括正常上电/深度睡眠唤醒初始化、
 * Flash 保护操作回调以及 BLE 主循环入口，供 main.c 调用。
 */
#ifndef APP_H_
#define APP_H_




/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 */
/* 中文说明：MCU 上电或从深度睡眠（非 retention）唤醒后的用户初始化入口，完成 BLE 协议栈、GATT、
 * ADV、电源管理等模块的初始化。 */
void user_init_normal(void);

/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 */
/* 中文说明：MCU 从深度睡眠 retention 模式唤醒后的初始化入口，恢复必要的协议栈状态与 UI 状态。 */
void user_init_deepRetn(void);

/**
 * @brief      flash protection operation, including all locking & unlocking for application
 * 			   handle all flash write & erase action for this demo code. use should add more more if they have more flash operation.
 * @param[in]  flash_op_evt - flash operation event, including application layer action and stack layer action event(OTA write & erase)
 * 			   attention 1: if you have more flash write or erase action, you should should add more type and process them
 * 			   attention 2: for "end" event, no need to pay attention on op_addr_begin & op_addr_end, we set them to 0 for
 * 			   			    stack event, such as stack OTA write new firmware end event
 * @param[in]  op_addr_begin - operating flash address range begin value
 * @param[in]  op_addr_end - operating flash address range end value
 * 			   attention that, we use: [op_addr_begin, op_addr_end)
 * 			   e.g. if we write flash sector from 0x10000 to 0x20000, actual operating flash address is 0x10000 ~ 0x1FFFF
 * 			   		but we use [0x10000, 0x20000):  op_addr_begin = 0x10000, op_addr_end = 0x20000
 * @return     none
 *//* 中文说明：Flash 保护操作回调，根据传入事件类型（应用初始化 / OTA 擦除旧固件开始与结束 /
 * OTA 写入新固件开始与结束）执行对应的 Flash 加锁/解锁操作，避免运行期误擦写非法区域。 */void app_flash_protection_operation(u8 flash_op_evt, u32 op_addr_begin, u32 op_addr_end);

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
/* 中文说明：BLE 主循环声明，由 main.c 中 main() 的 while(1) 循环反复调用，驱动协议栈、音频、按键、
 * LED 及低功耗流程。 */
void main_loop(void);




#endif /* APP_H_ */
