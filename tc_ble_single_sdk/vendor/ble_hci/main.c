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

extern my_fifo_t hci_rx_fifo;
extern void user_init_normal(void);
extern void main_loop(void);


/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 */
/* 中文说明：中断服务函数。
 * 1. 先调用 irq_blt_sdk_handler() 处理 BLE 协议栈相关中断（连接、广播等）。
 * 2. 当 HCI 通过 UART 传输（HCI_ACCESS==HCI_USE_UART）时，处理 UART 的
 *    接收 DMA 中断、发送完成中断以及接收错误（校验位/停止位错误）中断。
 * 3. 对于 B85 芯片（MCU_CORE_TYPE == MCU_CORE_825x），本文件中未特别区分
 *    寄存器访问方式的分支即为 B85（及 B87 共用）路径，直接使用 reg_uart_status0/1。
 */
_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler();

#if (HCI_ACCESS==HCI_USE_UART)
	#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
		if (dma_chn_irq_status_get(FLD_DMA_CHN_UART_RX) & FLD_DMA_CHN_UART_RX)
	#else
		if (dma_chn_irq_status_get(UART_NUM) & FLD_DMA_CHN_UART_RX)
	#endif
		{
			dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);
			u8* w = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num - 1))
					* hci_rx_fifo.size;
			if (w[0] != 0) {
				hci_rx_fifo.wptr++;
				u8* p = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num - 1))
						* hci_rx_fifo.size;
				reg_dma_uart_rx_addr = (u16) ((u32) p); //switch uart RX dma address
			}
		}
	#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
		if (reg_uart_status1(UART_NUM) & FLD_UART_TX_DONE)
	#else
		if (reg_uart_status1 & FLD_UART_TX_DONE)
	#endif
		{
			extern volatile u8 isUartTxDone;
			isUartTxDone = 1;
			uart_clr_tx_done(UART_NUM);
		}
	#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
		if (reg_uart_status0(UART_NUM) & FLD_UART_RX_ERR_FLAG)//when stop bit error or parity error.
		{
			reg_uart_status0(UART_NUM) = FLD_UART_CLEAR_RX_FLAG;
		}
	#else
		if (reg_uart_status0 & FLD_UART_RX_ERR_FLAG)//when stop bit error or parity error.
		{
			reg_uart_status0 = FLD_UART_CLEAR_RX_FLAG;
		}
	#endif
#endif
}


/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
/* 中文说明：程序入口（必须运行在 RAM code 中）。
 * 主要流程：
 * 1. 若使能低功耗（BLE_APP_PM_ENABLE），选择内部 32K 晶振作为低功耗时钟源；
 * 2. 唤醒 CPU 初始化：B85（MCU_CORE_825x）使用 cpu_wakeup_init() 无参版本，
 *    B87/TC321X 需要传入 LDO 模式和晶振电容参数；
 * 3. 初始化射频、GPIO、系统时钟；
 * 4. 如果使能了看门狗模块，配置看门狗超时时间并启动；
 * 5. 根据是否从 deep retention 唤醒，分别调用 user_init_deepRetn() 或
 *    user_init_normal() 完成协议栈初始化；
 * 6. 打开总中断，进入主循环，循环中喂狗并调用 main_loop() 处理协议栈事务。
 */
_attribute_ram_code_ int main (void)    //must run in ramcode
{
#if (BLE_APP_PM_ENABLE)
	blc_pm_select_internal_32k_crystal();
#endif

#if(MCU_CORE_TYPE == MCU_CORE_825x)
	cpu_wakeup_init();
#else
	cpu_wakeup_init(LDO_MODE,INTERNAL_CAP_XTAL24M);
#endif

#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
	wd_32k_stop();
#endif

	rf_drv_ble_init();

	gpio_init(1);

	clock_init(SYS_CLK_TYPE);

#if (MODULE_WATCHDOG_ENABLE)
	wd_set_interval_ms(WATCHDOG_INIT_TIMEOUT,CLOCK_SYS_CLOCK_1MS);
	wd_start();
#endif

#if	(PM_DEEPSLEEP_RETENTION_ENABLE)
	if( pm_is_MCU_deepRetentionWakeup() ){
		user_init_deepRetn();
	}
	else
#endif
	{
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
