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
#include "./2_4g_test/2p4g_common.h"

/*
 * 中文说明：
 * 本文件为程序入口文件，包含中断处理入口和 main() 主函数。main() 中完成
 * 时钟/电源管理相关硬件初始化（区分 B85/B87/TC321X 不同初始化方式），
 * 判断是否为深度睡眠 retention 唤醒并调用对应的用户初始化函数，最后进入
 * 主循环。当工程为 2.4G 私有协议测试模式时，会根据 rf_working_mode
 * 切换 BLE 或 2.4G 私有协议的初始化与中断处理路径。
 */

/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 * 中文说明：系统中断处理入口。当 TEST_2P4G_MODE 使能且当前为 2.4G 私有协议
 * 工作模式时调用 2.4G 中断处理函数，否则调用 BLE 协议栈的中断处理函数。
 */
_attribute_ram_code_ void irq_handler(void)
{
#if(TEST_2P4G_MODE)
    if(!rf_working_mode)
    {
        irq_blt_sdk_handler();
    }
    else{
        irq_2p4_handler();
    }
#else
    irq_blt_sdk_handler();
#endif
}


/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 * 中文说明：程序主函数，必须运行在 RAM 中。完成时钟源、唤醒初始化、GPIO、
 * 时钟、看门狗等基础硬件初始化，判断是否为深度睡眠 retention 唤醒后调用
 * user_init_deepRetn 或 user_init_normal 进行用户初始化，最终进入 while(1)
 * 主循环循环调用 main_loop()。B85(MCU_CORE_825x) 使用 cpu_wakeup_init() 无参数
 * 版本进行唤醒初始化。
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

    #if(TEST_2P4G_MODE)
    rf_working_mode=(analog_read(USED_DEEP_ANA_REG) & RF_WORKING_MODE);
    #endif
	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	gpio_init(!deepRetWakeUp);  //analog resistance will keep available in deepSleep mode, so no need initialize again

	clock_init(SYS_CLK_TYPE);

	#if (MODULE_WATCHDOG_ENABLE)
		wd_set_interval_ms(WATCHDOG_INIT_TIMEOUT,CLOCK_SYS_CLOCK_1MS);
		wd_start();
	#endif

    #if(TEST_2P4G_MODE)
        if(!rf_working_mode){
            rf_drv_ble_init();

            if( deepRetWakeUp ){
                user_init_deepRetn();
            }
            else{
                user_init_normal();
            }
        }
    #else
        rf_drv_ble_init();

        if( deepRetWakeUp ){
            user_init_deepRetn();
        }
        else{
            user_init_normal();
        }
    #endif
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

