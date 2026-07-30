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


#if (FEATURE_TEST_MODE == TEST_POWER_ADV)

/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 *
 * 中文：中断处理入口，必须运行在RAM中(_attribute_ram_code_)以保证响应速度。
 * 内部直接转发给 BLE 协议栈的中断处理入口，用户一般不需要修改。
 */
_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler();
}


/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 *
 * 中文：程序入口，完成芯片最基础的时钟/电源/GPIO初始化，然后根据唤醒来源分发到
 * user_init_normal(冷启动/普通唤醒) 或 user_init_deepRetn(深度休眠保留唤醒)，
 * 最后进入 main_loop 无限循环。
 */
_attribute_ram_code_ int main (void)    //must run in ramcode
{
	blc_pm_select_internal_32k_crystal();	//中文：选择使用芯片内部32K RC振荡器(而非外接32K晶振)，无需外接32K晶体

	#if(MCU_CORE_TYPE == MCU_CORE_825x)
		cpu_wakeup_init();												//中文：825x平台CPU唤醒初始化
	#else
		cpu_wakeup_init(LDO_MODE,INTERNAL_CAP_XTAL24M);					//中文：其他平台，指定电源模式(LDO)和内部24M晶振电容配置
	#endif

	#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
		wd_32k_stop();														//中文：TC321X平台需要停止基于32K时钟的看门狗，避免初始化期间误复位
	#endif

	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp　//中文：判断本次启动是否来自深度休眠保留唤醒

	rf_drv_ble_init();	//中文：射频驱动初始化(BLE相关)

	gpio_init(!deepRetWakeUp);  //analog resistance will keep available in deepSleep mode, so no need initialize again　//中文：GPIO初始化；若是从deepRetn唤醒，模拟电阻配置在深度休眠期间依然保持，无需重复初始化

	clock_init(SYS_CLK_TYPE);	//中文：系统时钟初始化

	if( deepRetWakeUp ){
		user_init_deepRetn();	//中文：深度休眠保留唤醒分支，快速恢复
	}
	else{
		user_init_normal();	//中文：冷启动/普通唤醒分支，完整初始化
	}

    irq_enable();	//中文：使能全局中断

	while (1) {
		main_loop();	//中文：无限循环调用主循环函数，驱动BLE协议栈运转
	}
}

#endif  //end of (FEATURE_TEST_MODE == ...)
