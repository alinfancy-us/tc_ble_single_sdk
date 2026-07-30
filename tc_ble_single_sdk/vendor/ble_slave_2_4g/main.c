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
/*******************************************************************************************************
 * 中文说明：
 * 本文件是 ble_slave_2_4g 工程的入口文件，包含芯片上电复位后的初始化流程（时钟、GPIO、RF、看门狗等）
 * 以及主循环 main()。当工程同时使能了 2.4G 私有协议测试模式（TEST_2P4G_MODE）时，本文件的中断处理函数
 * 会根据 irq_bleModeFlag 在 BLE 协议栈中断处理与 2.4G 并发（concurrent）中断处理之间进行切换。
 * 本文件中的芯片相关分支（cpu_wakeup_init/debug_config 等）已按 MCU_CORE_TYPE 区分 B85/B87/TC321X，
 * 本次仅关注 B85 (MCU_CORE_825x) 分支及无芯片区分的公共逻辑。
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "./2_4g_test/2p4g_common.h"

volatile unsigned char irq_bleModeFlag = 1;

/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 */
/* 中文说明：系统中断入口。当使能了 2.4G 测试模式且当前处于 2.4G 私有协议模式（irq_bleModeFlag==0）时，
 * 转去调用 2.4G 并发中断处理函数；否则（包括 BLE 模式）调用 BLE 协议栈自带的中断处理函数。
 */
_attribute_ram_code_ void irq_handler(void)
{
#if (TEST_2P4G_MODE)
    if(!irq_bleModeFlag)
    {
        concurrent_irq_handler();
    }
    else
#endif
    {
        irq_blt_sdk_handler();
    }
}

#if (RF_DEBUG_IO_ENABLE)
/**
 * @brief       this function is only for internal rf test.
 *              TX EN: tx enable.
 *              TX ON: on tx data.
 *              RX EN: rx enable.
 *              TX DATA: high level indicate 1,low level indicate 0,the interval is daterate.
 *              RX_DATA: high level indicate 1,low level indicate 0,the interval is RX DATA CLK.
 *              RX DATA CLK: interval for rx date.
 *              CLK DIG: indicate system clock.
 *              RX ACCESS DET: accesscode begin to detect(rx timstamp).
 * @param[in]   none
 * @return      none
 */
/* 中文说明：内部 RF 调试专用，将若干 GPIO 复用为调试信号输出（TX/RX 使能、数据、时钟等），
 * 便于用逻辑分析仪观测收发时序。B85 分支通过寄存器 0x5a8/0x586/0x5b6 配置调试 IO，与 TC321X 分支互不影响。
 */
void debug_config(void)
{
#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
    gpio_set_func(GPIO_PA0, DBG0_IO);  //TX EN
    gpio_set_func(GPIO_PA4, DBG0_IO);  //TX ON
    gpio_set_func(GPIO_PA5, DBG0_IO);  //RX EB
    gpio_set_func(GPIO_PA6, DBG0_IO);  //RX DATE
    gpio_set_func(GPIO_PA7, DBG0_IO);  //RX ACCESS DET
    gpio_set_func(GPIO_PB0, DBG0_IO);  //RX DATA CLK
    gpio_set_func(GPIO_PB2, DBG0_IO);  //TX DATA
    gpio_set_func(GPIO_PB4, DBG0_IO);  //CLK DIG

    write_reg8(0x574, read_reg8(0x574)|BIT(3)|BIT(4)|BIT(5));

#elif(MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
    WRITE_REG8(0x5a8, 0xff);//tx_en: PA0 TX_ON:PA1 RX_EN:PA2
    WRITE_REG8(0x586, 0x00);
    WRITE_REG8(0x5b6, READ_REG8(0x5b6) | (1 << 1));
#endif
}
#endif

/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
/* 中文说明：程序主入口，运行在 RAM code 中。依次完成：CPU 唤醒初始化、TC321X 看门狗停止（仅该芯片）、
 * 判断是否为深度休眠保持(deep retention)唤醒、RF/GPIO/时钟初始化、可选看门狗启动、用户初始化
 * (user_init_normal)、调试 IO 配置、开中断，最后进入 while(1) 主循环反复调用 main_loop()。
 */
_attribute_ram_code_ int main (void)    //must run in ramcode
{

	DBG_CHN0_LOW;   //debug
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

    user_init_normal();

#if (RF_DEBUG_IO_ENABLE)
    debug_config();
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

