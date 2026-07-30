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
/*
 * 中文说明：
 * 本文件为程序入口文件，负责芯片时钟/GPIO/射频初始化、USB 初始化、
 * 调用 user_init 完成应用层初始化，然后进入死循环调度 main_loop。
 * B85(MCU_CORE_825x) 与 B87(MCU_CORE_827x) 在 cpu_wakeup_init 调用参数上存在差异，
 * 其余逻辑两者共用。
 */
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "app_audio.h"
#include "application/usbstd/usb.h"


/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 * 中文：中断服务函数。先交由 BLE 协议栈处理其自身中断，若使能音频功能
 * (UI_AUDIO_ENABLE) 且触发 IRQ4 中断，则调用 USB 端点中断处理函数。
 */
_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler();


#if (UI_AUDIO_ENABLE)
	if(reg_irq_src & FLD_IRQ_IRQ4_EN){
		usb_endpoints_irq_handler();
	}
#endif

}


/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 * 中文：程序入口。完成 CPU 唤醒初始化（B85 分支：cpu_wakeup_init() 无参数调用）、
 * 系统时钟初始化、GPIO 初始化、射频初始化、USB 初始化，然后调用 user_init 完成
 * 应用层初始化，最后进入主循环，循环内按需清看门狗并调用 main_loop。
 */
int main (void)
{
	#if(MCU_CORE_TYPE == MCU_CORE_825x)
		cpu_wakeup_init();
	#elif(MCU_CORE_TYPE == MCU_CORE_827x)
		cpu_wakeup_init(LDO_MODE,INTERNAL_CAP_XTAL24M);
	#endif

	clock_init(SYS_CLK_TYPE);

	gpio_init(1);

	rf_drv_ble_init();

	usb_init ();

	user_init ();

    irq_enable();

	while (1) {
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop();
	}
}


