/********************************************************************************************************
 * @file    blm_ota.h
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
#ifndef BLM_OTA_H_
#define BLM_OTA_H_

/* 中文说明：本头文件声明 OTA（固件空中升级）主循环处理函数以及 OTA 测试模式标志，
 * 配合 blm_ota.c 实现 Master 作为 OTA 主导方向时的固件推送/校验逻辑。
 */

/**
 * @brief		ota proc in main loop
 * @param[in]	none
 * @return      none
 */
void proc_ota (void);


extern int 	master_ota_test_mode;



#endif /* BLM_OTA_H_ */
