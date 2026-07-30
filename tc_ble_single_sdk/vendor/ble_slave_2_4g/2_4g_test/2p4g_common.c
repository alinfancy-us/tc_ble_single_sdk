/********************************************************************************************************
 * @file    2p4g_common.c
 *
 * @brief   This is the source file for 2.4G SDK
 *
 * @author  2.4G Group
 * @date    12,2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
/* 中文说明：本文件为 2.4G 私有协议与 BLE 并发（concurrent）时的公共辅助函数，包括退出 2.4G 模式时恢复 BLE
 * 射频状态以及 2.4G 空闲循环中的 LED 等处理，与具体芯片型号无关。
 */
#include "2p4g_common.h"
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "../app_config.h"
#include "driver.h"
extern volatile unsigned char irq_bleModeFlag;

/* 中文说明：退出 2.4G 私有协议模式。重置 BLE 射频状态机，并将 irq_bleModeFlag 置 1，
 * 使后续中断重新进入 BLE 协议栈的中断处理分支。
 */
void app_exit_2p4gMode()
{
	rf_ble_state_reset();
    irq_bleModeFlag = 1;
}

/* 中文说明：2.4G 空闲循环处理。仅当使能 UI_LED_ENABLE 时，关闭绿色 LED。
 */
void app_idle_loop_2p4g()
{
#if (UI_LED_ENABLE)
	gpio_write(GPIO_LED_GREEN,0);
#endif
}
