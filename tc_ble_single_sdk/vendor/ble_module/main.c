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
 * 本文件包含 IRQ 中断处理函数 irq_handler 与主函数 main，是 ble_module 示例工程的程序入口，
 * 负责底层初始化、用户初始化分发及主循环调度；其中涉及部分 B85/B87/TC321X 平台差异分支，
 * 本次注释仅针对 B85(825x) 分支及平台无关的公共逻辑，B87/TC321X 专属分支保持原样。
 */
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "app_att.h"
#include "spp.h"


extern my_fifo_t spp_rx_fifo;


/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 *
 * 中文说明：BLE IRQ 中断处理函数。除调用协议栈中断处理 irq_blt_sdk_handler 外，还处理：
 * UART 发送完成中断标志(TC321X 与非 TC321X 走不同寄存器 API，B85 走非 TC321X 分支)、
 * UART RX DMA 完成后切换下一个接收 FIFO 缓冲区地址，以及 UART 校验/停止位错误清除。
 */
_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler();

#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
	if (reg_uart_status1(UART_MODULE_SEL) & FLD_UART_TX_DONE)
#else
	if (reg_uart_status1 & FLD_UART_TX_DONE)
#endif
	{
		Tr_SetUartTxDone();
		uart_clr_tx_done(UART_MODULE_SEL);
	}

#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
	if (dma_chn_irq_status_get(FLD_DMA_CHN_UART_RX) & FLD_DMA_CHN_UART_RX)
#else
	if (dma_chn_irq_status_get() & FLD_DMA_CHN_UART_RX)
#endif
	{
		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);
		u8* w = spp_rx_fifo.p + (spp_rx_fifo.wptr & (spp_rx_fifo.num-1)) * spp_rx_fifo.size;
    	if (w[0] != 0 || w[1] != 0) //Length(u16) is not 0
    	{
    		my_fifo_next(&spp_rx_fifo);
    		u8* p = spp_rx_fifo.p + (spp_rx_fifo.wptr & (spp_rx_fifo.num-1)) * spp_rx_fifo.size;
    		reg_dma_uart_rx_addr = (u16)((u32)p);  //switch uart RX dma address
    	}
	}
	if (uart_is_parity_error(UART_MODULE_SEL))//when stop bit error or parity error.
	{
		uart_clear_parity_error(UART_MODULE_SEL);
	}
}


/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 *
 * 中文说明：程序入口。完成 32K 晶振选择、MCU 唤醒初始化(B85 与其它芯片调用参数不同)、RF 与
 * GPIO/时钟初始化，并依据是否为深度睡眠保留唤醒调用 user_init_deepRetn 或 user_init_normal，
 * 随后使能中断并进入 while(1) 主循环调用 main_loop()(可选看门狗喂狗)。
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
		#if FIRMWARES_SIGNATURE_ENABLE
			blt_firmware_signature_check();
		#endif
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
