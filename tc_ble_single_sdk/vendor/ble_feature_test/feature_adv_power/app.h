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
#ifndef APP_H_
#define APP_H_

#include "../feature_config.h"

/* 中文说明：本样例仅在 feature_config.h 中选中 FEATURE_TEST_MODE == TEST_POWER_ADV（ADV功耗测试）时才会参与编译 */
#if (FEATURE_TEST_MODE == TEST_POWER_ADV)


/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 *
 * 中文：芯片上电(冷启动)或从非retention的深度休眠唤醒时调用的用户初始化函数，实现在 app.c
 */
void user_init_normal(void);

/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 *
 * 中文：芯片从深度休眠保留(retention)模式唤醒时调用的用户初始化函数，实现在 app.c
 */
void user_init_deepRetn(void);


/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 *
 * 中文：主循环函数，在 main.c 的 while(1) 中被不断调用，实现在 app.c
 */
void main_loop(void);



#endif  //end of (FEATURE_TEST_MODE == ...)
#endif /* APP_H_ */
