/********************************************************************************************************
 * @file    main.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "../feature_config.h"

#if (FEATURE_TEST_MODE == TEST_MD_MASTER)
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"

/*
 * 中文说明：本文件为 feature_md_master 示例的程序入口，完成时钟/GPIO/射频硬件
 * 初始化后调用 user_init 并进入 BLE 主循环。
 */


/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 * 中文说明：中断入口，转发给 BLE 协议栈的中断处理函数。
 */
_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler();
}


/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 * 中文说明：主函数。根据芯片型号（B85 使用默认参数，其他芯片使用指定时钟模式/
 * 电容配置）完成 CPU 唤醒初始化、时钟、GPIO 与射频初始化，然后调用 user_init
 * 并进入主循环。
 */
int main (void)
{
	#if(MCU_CORE_TYPE == MCU_CORE_825x)
		cpu_wakeup_init();
	#else
		cpu_wakeup_init(LDO_MODE,INTERNAL_CAP_XTAL24M);
	#endif

	#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
		wd_32k_stop();
	#endif

	clock_init(SYS_CLK_TYPE);

	gpio_init(1);

	rf_drv_ble_init();

	user_init();

    irq_enable();

	while (1) {
		main_loop();
	}
}


#endif  //end of (FEATURE_TEST_MODE == ...)
