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
 * 中文说明：本文件为 feature_ll_state 示例的程序入口，完成中断入口、时钟/GPIO/射频
 * 硬件初始化，并根据是否为 DeepSleep Retention 唤醒选择调用不同的用户初始化函数，
 * 最后进入 BLE 主循环。
 */


#if (FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY || FEATURE_TEST_MODE == TEST_SCANNING_ONLY || FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE || \
	FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)

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
 * 中文说明：主函数，必须运行在 RAM 代码中。依次选择 32K 时钟源、
 * 根据芯片型号（B85 使用默认参数，其他芯片使用指定时钟模式/电容配置）完成 CPU
 * 唤醒初始化，判断是否为 DeepSleep Retention 唤醒并调用对应初始化函数，最后进入
 * 主循环。
 */
_attribute_ram_code_ int main (void)    //must run in ramcode
{
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

	if( deepRetWakeUp ){
		user_init_deepRetn();
	}
	else{
		user_init_normal();
	}

    irq_enable();

	while (1) {
		main_loop();
	}
}

#endif  //end of (FEATURE_TEST_MODE == ...)
