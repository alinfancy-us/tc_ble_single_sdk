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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"

#if (FEATURE_TEST_MODE == TEST_LL_PRIVACY_MASTER)


/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 */
/* 中文说明：中断服务函数，转发给协议栈统一的中断处理入口 irq_blt_sdk_handler()。 */
_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler();
}


/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
/* 中文说明：程序入口。完成 CPU 唤醒初始化（B85 使用 cpu_wakeup_init() 无参版本）、
 * 系统时钟与 GPIO 初始化、射频初始化，随后调用 user_init() 完成协议栈及本特性
 * （LL 隐私 - 主机角色）初始化，最后进入 main_loop() 主循环。
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


	user_init ();

    irq_enable();

	while (1) {
		main_loop();
	}
}

#endif
