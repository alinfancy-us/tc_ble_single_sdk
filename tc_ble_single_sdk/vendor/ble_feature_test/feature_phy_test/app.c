/********************************************************************************************************
 * @file    app.c
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
 * 本文件实现 BLE PHY 直接测试模式（Direct Test Mode, DTM）示例（feature_phy_test），
 * 支持通过 2 线 UART、HCI over UART 两种方式与上位测试仪器/主机交互，用于射频性能（RF）认证测试。
 * 主要包括：UART DMA 收发中断处理、HCI 数据收发回调、PHY 测试模式初始化。
 * 测试模式下不支持低功耗挂起（suspend/deepsleep）。
 */
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app.h"
#include "../default_att.h"

#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"


#if (FEATURE_TEST_MODE == TEST_BLE_PHY)


MYFIFO_INIT(hci_rx_fifo, HCI_RXFIFO_SIZE, HCI_RXFIFO_NUM);
MYFIFO_INIT(hci_tx_fifo, HCI_TXFIFO_SIZE, HCI_TXFIFO_NUM);

#define RX_FIFO_SIZE	288
#define RX_FIFO_NUM		4

#define TX_FIFO_SIZE	264
#define TX_FIFO_NUM		4



_attribute_data_retention_  u8 		 	blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
_attribute_data_retention_	my_fifo_t	blt_rxfifo = {
												RX_FIFO_SIZE,
												RX_FIFO_NUM,
												0,
												0,
												blt_rxfifo_b,};


_attribute_data_retention_  u8 		 	blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
_attribute_data_retention_	my_fifo_t	blt_txfifo = {
												TX_FIFO_SIZE,
												TX_FIFO_NUM,
												0,
												0,
												blt_txfifo_b,};


typedef struct{
	unsigned int len;        // data max 252
	unsigned char data[UART_DATA_LEN];
}uart_data_t;

#if (BLE_PHYTEST_MODE == PHYTEST_MODE_THROUGH_2_WIRE_UART )
	unsigned char uart_no_dma_rec_data[6] = {0x02,0, 0,0,0,0};
#elif(BLE_PHYTEST_MODE == PHYTEST_MODE_OVER_HCI_WITH_UART)
	unsigned char uart_no_dma_rec_data[72] = {0};
#endif

#define		MY_RF_POWER_INDEX					RF_POWER_P3dBm

volatile u8 isUartTxDone = 1;
#define   Tr_clrUartTxDone()    (isUartTxDone = 0)
#define   Tr_SetUartTxDone()    (isUartTxDone = 1)
#define   Tr_isUartTxDone()     (!isUartTxDone)

#if (BLE_PHYTEST_MODE == PHYTEST_MODE_OVER_HCI_WITH_UART)

	/**
	 * @brief		this function is used to process rx uart data.
	 * @param[in]	none
	 * @return      0 is ok
	 * 中文说明：从 hci_rx_fifo 取出一帧 UART 接收数据，解析长度字段后转发给 blc_hci_handler 处理，
	 * 处理完成后弹出该 fifo 项。
	 */
	int rx_from_uart_cb (void)
	{
		if(my_fifo_get(&hci_rx_fifo) == 0)
		{
			return 0;
		}

		u8* p = my_fifo_get(&hci_rx_fifo);
		u32 rx_len = p[0]; //usually <= 255 so 1 byte should be sufficient

		if (rx_len)
		{
			blc_hci_handler(&p[4], rx_len - 4);
			my_fifo_pop(&hci_rx_fifo);
		}

		return 0;
	}


	/**
	 * @brief		this function is used to process tx uart data.
	 * @param[in]	none
	 * @return      0 is ok
	 * 中文说明：将 hci_tx_fifo 中待发送的数据拷贝到本地缓冲区并通过 UART DMA 发送，
	 * 若上一次 UART 发送尚未完成则本次不发送（避免覆盖未完成的发送）。
	 */
	int tx_to_uart_cb (void)
	{
		uart_data_t T_txdata_buf;

		if(hci_tx_fifo.wptr == hci_tx_fifo.rptr){
			return 0;//have no data
		}
		if (Tr_isUartTxDone()) {
			return 0;
		}
		u8 *p =  my_fifo_get(&hci_tx_fifo);
		memcpy(&T_txdata_buf.data, p + 2, p[0] | (p[1] << 8));

		T_txdata_buf.len = p[0] | (p[1] << 8);
		Tr_clrUartTxDone();
		uart_send_dma(UART_CONVERT((unsigned char*) (&T_txdata_buf)));
		my_fifo_pop(&hci_tx_fifo);

		return 1;

	}
#endif

/**
 * @brief		this function is used to process uart irq
 * @param[in]	none
 * @return      none
 * 中文说明：UART 收发 DMA 中断处理函数：收到数据时切换到下一个接收 fifo 缓冲区，
 * 发送完成时清除中断标志并标记 UART 发送完成标志位。TC321X 与其他芯片寄存器/接口差异通过宏区分。
 */
void app_phytest_irq_proc(void)
{
	//1. UART irq
#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
	if(dma_chn_irq_status_get(FLD_DMA_CHN_UART_RX) & FLD_DMA_CHN_UART_RX)
#else
	if(dma_chn_irq_status_get() & FLD_DMA_CHN_UART_RX)
#endif
	{
		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);
		u8* w = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
		if(w[0]!=0)
		{
			my_fifo_next(&hci_rx_fifo);
			u8* p = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
			reg_dma0_addr = (u16)((u32)p);
		}
	}
#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
	if(dma_chn_irq_status_get(FLD_DMA_CHN_UART_TX) & FLD_DMA_CHN_UART_TX)
#else
	if(dma_chn_irq_status_get() & FLD_DMA_CHN_UART_TX)
#endif
	{
		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_TX);
	}
#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
	if (reg_uart_status1(UART_NUM) & FLD_UART_TX_DONE)
#else
	if (reg_uart_status1 & FLD_UART_TX_DONE)
#endif
	{
		Tr_SetUartTxDone();
		uart_clr_tx_done(UART_NUM);
	}
}


/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 * 中文说明：MCU 从深度睡眠 retention 唤醒时的初始化入口（PHY 测试模式下默认不使能 retention，
 * 此处仅作为预留分支）。
 */
_attribute_ram_code_ void user_init_deepRetn(void)
{
#if (PM_DEEPSLEEP_RETENTION_ENABLE)
	blc_app_loadCustomizedParameters_deepRetn();
	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (MY_RF_POWER_INDEX);

	blc_ll_recoverDeepRetention();

	irq_enable();

	DBG_CHN0_HIGH;    //debug
#endif
}


/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 * 中文说明：上电/唤醒后的用户初始化入口：初始化随机数、Flash 参数、MAC 地址、RF 功率等级，
 * 启用 PHY 直接测试模式（blc_phy_initPhyTest_module/blc_phy_setPhyTestEnable），并根据配置的 BLE_PHYTEST_MODE
 * 初始化 UART 并注册对应的 HCI 收发回调；测试模式下禁止进入低功耗挂起。
 */
void user_init_normal(void)
{
	//random number generator must be initiated here( in the beginning of user_init_nromal)
	//when deepSleep retention wakeUp, no need initialize again
#if(MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
	random_generator_init();  //this is must
#endif

	blc_readFlashSize_autoConfigCustomFlashSector();

	/* attention that this function must be called after "blc_readFlashSize_autoConfigCustomFlashSector" !!!*/
	blc_app_loadCustomizedParameters_normal();

	u8  mac_public[6];
	u8  mac_random_static[6];
	//for 512K Flash, flash_sector_mac_address equals to 0x76000
	//for 1M  Flash, flash_sector_mac_address equals to 0xFF000
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);

	rf_set_power_level_index (MY_RF_POWER_INDEX);

	////// Controller Initialization  //////////
	blc_ll_initBasicMCU();                      //mandatory
	blc_ll_initStandby_module(mac_public);				//mandatory


	write_reg8(0x402, 0x2b);   //set rf packet preamble for BQB
	blc_phy_initPhyTest_module();
	blc_phy_setPhyTestEnable( BLC_PHYTEST_ENABLE );
	blc_phy_preamble_length_set(11);

	#if(BLE_PHYTEST_MODE == PHYTEST_MODE_THROUGH_2_WIRE_UART || BLE_PHYTEST_MODE == PHYTEST_MODE_OVER_HCI_WITH_UART)  //uart
		uart_gpio_set(UART_CONVERT(UART_TX_PIN, UART_RX_PIN));
		uart_reset(UART_NUM);
	#endif

	uart_recbuff_init(UART_CONVERT((unsigned char*)hci_rx_fifo_b, hci_rx_fifo.size));

	uart_init_baudrate(UART_CONVERT(UART_BAUDRATE, CLOCK_SYS_CLOCK_HZ, PARITY_NONE, STOP_BIT_ONE));

	uart_dma_enable(UART_CONVERT(1,1));

	#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
		reg_uart_rx_timeout1(UART_NUM)  = UART_BW_MUL4; //packet interval timeout
	#else
		reg_uart_rx_timeout1  = UART_BW_MUL4; //packet interval timeout
	#endif
	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);   	//uart Rx/Tx dma irq enable
	uart_irq_enable(UART_CONVERT(1,0));

	isUartTxDone = 1;
	#if	(BLE_PHYTEST_MODE == PHYTEST_MODE_THROUGH_2_WIRE_UART)
		blc_register_hci_handler (phy_test_2_wire_rx_from_uart, phy_test_2_wire_tx_to_uart);
	#elif(BLE_PHYTEST_MODE == PHYTEST_MODE_OVER_HCI_WITH_UART)
		blc_register_hci_handler (rx_from_uart_cb, tx_to_uart_cb);		//default handler
	#endif

	///////////////////// Power Management initialization///////////////////

	bls_pm_setSuspendMask (SUSPEND_DISABLE); // phy test can not enter suspend/deep


}



/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 * 中文说明：BLE 主循环，仅驱动协议栈主循环，本测试工程不需要其他应用层处理。
 */
void main_loop(void)
{

	////////////////////////////////////// BLE entry /////////////////////////////////
	blt_sdk_main_loop();
}


#endif  //end of (FEATURE_TEST_MODE == ...)
