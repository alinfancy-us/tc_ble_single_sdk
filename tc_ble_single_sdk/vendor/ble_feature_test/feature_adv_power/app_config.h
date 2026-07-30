/********************************************************************************************************
 * @file    app_config.h
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
#pragma once

#include "../feature_config.h"

/* 中文说明：本配置文件仅在选中“ADV功耗测试”特性(FEATURE_TEST_MODE == TEST_POWER_ADV)时生效 */
#if (FEATURE_TEST_MODE == TEST_POWER_ADV)

///////////////////////// Feature Configuration////////////////////////////////////////////////
#define BLE_APP_PM_ENABLE								1	//中文：开启应用层电源管理(PM)，允许进入suspend低功耗状态
#define PM_DEEPSLEEP_RETENTION_ENABLE					1	//中文：开启深度休眠保留模式(带SRAM保留，唤醒更快，更省电)

#define APP_DEFAULT_HID_BATTERY_OTA_ATTRIBUTE_TABLE		1	//中文：使用SDK默认的HID/电池/OTA GATT属性表(本功耗测试样例不依赖具体服务，保留默认即可)


///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE								0	//中文：GPIO调试开关，未开启
#define UART_PRINT_DEBUG_ENABLE							1	//中文：UART打印调试开关，已临时开启(默认应为0，避免影响功耗测试准确性)。
															//用于通过UART串口打印日志排查问题；正式测量功耗时建议改回0，避免打印本身增加功耗/干扰计时
#define APP_FLASH_INIT_LOG_EN							1	//中文：Flash初始化日志开关，已开启，可打印Flash容量识别/MID读取相关日志
#define APP_LOG_EN										1	//中文：应用层日志总开关，已开启，配合 tlkapi_printf(APP_LOG_EN, ...) 输出应用层调试信息

/////////////////////// Feature Test Board Select Configuration ///////////////////////////////
/* 中文：根据编译时选择的芯片工程宏(__PROJECT_xxx_FEATURE_TEST__)，自动选择对应的开发板型号，
 * 决定 GPIO/按键/LED 等引脚定义使用哪个 boards 头文件(见 vendor/common/boards/) */
#if (__PROJECT_8258_FEATURE_TEST__)
	#define BOARD_SELECT								BOARD_825X_EVK_C1T139A30	//中文：8258平台使用EVK开发板 C1T139A30
#elif (__PROJECT_8278_FEATURE_TEST__)
	#define BOARD_SELECT								BOARD_827X_EVK_C1T197A30	//中文：8278平台使用EVK开发板 C1T197A30
#elif (__PROJECT_TC321X_FEATURE_TEST__)
    //support BOARD_TC321X_EVK_C1T357A20 & BOARD_TC321X_EVK_C1T357A20_V2_1
    #define BOARD_SELECT                                BOARD_TC321X_EVK_C1T357A20_V2_1	//中文：TC321X平台使用EVK开发板 C1T357A20_V2_1
#endif



///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define	UI_KEYBOARD_ENABLE								0	//中文：不使用键盘扫描功能(纯功耗测试，关闭无关UI引起的额外功耗)
#define	UI_LED_ENABLE									0	//中文：不使用LED指示(同上，避免引入额外功耗干扰)



///////////////////////// System Clock  Configuration /////////////////////////////////////////
#define CLOCK_SYS_CLOCK_HZ  								16000000	//中文：系统时钟设为16MHz

#include "vendor/common/default_config.h"	//中文：未在本文件显式定义的其他宏，会从默认配置中获得默认值


#endif  //end of (FEATURE_TEST_MODE == ...)
