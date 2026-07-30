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

/*
 * 中文说明：本文件为 ble_sample 工程的程序入口，包含中断处理函数与 main()。main()
 * 依次完成时钟/系统基础硬件初始化，判断是否为 deepSleep retention 唤醒并调用
 * 对应的应用初始化函数，最后进入主循环。其中不同 MCU_CORE_TYPE（B85/B87/
 * TC321X）的时钟与看门狗初始化分支仅 B85（MCU_CORE_825x）分支作了注释补充，
 * B87/TC321X 分支未作任何修改。
 */


/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 *
 * 中文说明：系统中断入口，转发给 BLE 协议栈的中断处理函数 irq_blt_sdk_handler()。
 */
_attribute_ram_code_ void irq_handler(void)
{

	irq_blt_sdk_handler();

}


/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ int main (void)    //must run in ramcode
{

	DBG_CHN0_LOW;   //debug

	blc_pm_select_internal_32k_crystal();

	#if(MCU_CORE_TYPE == MCU_CORE_825x)
		cpu_wakeup_init();
	#else
		cpu_wakeup_init(LDO_MODE,INTERNAL_CAP_XTAL24M);
	#endif

	#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
		wd_32k_stop();
	#endif

	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	rf_drv_ble_init();

	gpio_init(!deepRetWakeUp);  //analog resistance will keep available in deepSleep mode, so no need initialize again

	clock_init(SYS_CLK_TYPE);

	#if (MODULE_WATCHDOG_ENABLE)
		wd_set_interval_ms(WATCHDOG_INIT_TIMEOUT,CLOCK_SYS_CLOCK_1MS);
		wd_start();
	#endif

	if( deepRetWakeUp ){
		user_init_deepRetn();
	}
	else{
		user_init_normal();
	}

    irq_enable();
	while (1) {
	#if (MODULE_WATCHDOG_ENABLE)
		#if (MCU_CORE_TYPE == MCU_CORE_TC321X)
			if (g_chip_version != CHIP_VERSION_A0)
		#endif
			{
				wd_clear(); //clear watch dog
			}
	#endif
		main_loop();
	}
}

