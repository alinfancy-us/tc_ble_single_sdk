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
 * 模块说明（中文）：
 * 本文件是 feature_ll_state 示例工程的应用层实现，用于演示 BLE Link Layer 在不同状态
 * （单独广播、单独扫描、连接从机角色下的广播/扫描等组合场景）下的基本用法。根据
 * feature_config.h 中 FEATURE_TEST_MODE 的取值，编译期选择对应的初始化分支：设置广播/
 * 扫描参数、注册连接与断开等 Link Layer 事件回调、以及低功耗（PM）挂起策略。
 * 该文件对 B85（MCU_CORE_825x）与其他芯片共用的逻辑保持一致，仅在少量位置（如
 * DeepSleep 提前唤醒时间）按芯片型号区分参数。
 */
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app.h"
#include "../default_att.h"

#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"


#if (FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY || FEATURE_TEST_MODE == TEST_SCANNING_ONLY || FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE || \
	FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)

#define		MY_RF_POWER_INDEX					RF_POWER_P3dBm


#define RX_FIFO_SIZE	64
#define RX_FIFO_NUM		8

#define TX_FIFO_SIZE	40
#define TX_FIFO_NUM		8



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




_attribute_data_retention_	int device_in_connection_state;




/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_CONNECT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 * 中文说明：连接建立事件回调。打印对端地址信息，将连接状态标志置 1，
 * 若使能 UI LED 则点亮红灯指示已连接。
 */
void	task_connect (u8 e, u8 *p, int n)
{
    (void)e;(void)p;(void)n;
    tlk_contr_evt_connect_t *pConnEvt = (tlk_contr_evt_connect_t *)p;
	tlkapi_send_string_data(APP_CONTR_EVENT_LOG_EN, "[APP][EVT] connect, intA & advA:", pConnEvt->initA, 12);
	//bls_l2cap_requestConnParamUpdate (CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 99, CONN_TIMEOUT_4S);  // 1 S

	device_in_connection_state = 1;//

	#if (UI_LED_ENABLE)
		gpio_write(GPIO_LED_RED, LED_ON_LEVEL);  // light on
	#endif
}



/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_TERMINATE"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 * 中文说明：连接断开事件回调。将连接状态标志清零，根据不同的断开原因
 * （超时、对端主动断开、MIC 校验失败等）分支处理（当前分支体为空，仅作分类），
 * 打印断开原因，若使能 UI LED 则熄灭红灯。
 */
void 	task_terminate(u8 e,u8 *p, int n) //*p is terminate reason
{
    (void)e;(void)p;(void)n;
	device_in_connection_state = 0;

	tlk_contr_evt_terminate_t *pEvt = (tlk_contr_evt_terminate_t *)p;
	if(pEvt->terminate_reason == HCI_ERR_CONN_TIMEOUT){

	}
	else if(pEvt->terminate_reason == HCI_ERR_REMOTE_USER_TERM_CONN){

	}
	else if(pEvt->terminate_reason == HCI_ERR_CONN_TERM_MIC_FAILURE){

	}
	else{

	}
	tlkapi_printf(APP_CONTR_EVENT_LOG_EN, "[APP][EVT] disconnect, reason 0x%x\n", pEvt->terminate_reason);


#if (UI_LED_ENABLE)
	gpio_write(GPIO_LED_RED, !LED_ON_LEVEL);  // light off
#endif


}



/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_SUSPEND_EXIT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 * 中文说明：MCU 从挂起（suspend）状态退出时的回调，重新设置射频发射功率，
 * 因为挂起唤醒后部分射频寄存器可能需要重新配置。
 */
void	task_suspend_exit (u8 e, u8 *p, int n)
{
    (void)e;(void)p;(void)n;
	rf_set_power_level_index (MY_RF_POWER_INDEX);
}


/**
 * @brief      power management code for application
 * @param	   none
 * @return     none
 * 中文说明：应用层低功耗处理，根据是否使能 DeepSleep Retention，配置广播/连接态
 * 下允许进入 suspend 或 deepSleep retention 的挂起掩码。
 */
void blt_pm_proc(void)
{
#if(BLE_APP_PM_ENABLE)
	#if (PM_DEEPSLEEP_RETENTION_ENABLE)
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
	#else
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	#endif
#endif  //end of BLE_APP_PM_ENABLE
}





#if (FEATURE_TEST_MODE == TEST_SCANNING_ONLY || FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE \
	  || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE )
//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
#define DBG_ADV_REPORT_ON_RAM 				0
#if (DBG_ADV_REPORT_ON_RAM)  //debug ADV report on ram
	#define  RAM_ADV_MAX		64
	u8 AA_advRpt[RAM_ADV_MAX][48];
	u8 AA_advRpt_index = 0;
#endif

/**
 * @brief      callback function of HCI Controller Event
 * @param[in]  h - HCI Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 * 中文说明：HCI 控制器事件回调，用于扫描模式下接收 LE Advertising Report 事件。
 * 注意：DBG_ADV_REPORT_ON_RAM 调试分支默认关闭（宏值为 0），其中引用的 pa 变量
 * 声明被注释掉，若日后打开该调试宏需一并恢复 pa 的声明，否则编译报错。
 */
int controller_event_callback (u32 h, u8 *p, int n)
{
    (void)h;(void)p;(void)n;
	if (h &HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;

		if(evtCode == HCI_EVT_LE_META)
		{
			u8 subEvt_code = p[0];
			if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				DBG_CHN6_TOGGLE;
				//after controller is set to scan state, it will report all the ADV packet it received by this event

//				event_adv_report_t *pa = (event_adv_report_t *)p;
//				s8 rssi = (s8)pa->data[pa->len];//rssi has already plus 110.
				//printf("LE advertising report (rssi:%ddb, len:%d):\n", rssi, pa->len+11);

				#if (DBG_ADV_REPORT_ON_RAM)
					if(pa->len > 31){
						pa->len = 31;
					}
					memcpy( (u8 *)AA_advRpt[AA_advRpt_index++],  p, pa->len + 11);
					if(AA_advRpt_index >= RAM_ADV_MAX){
						AA_advRpt_index = 0;
					}
				#endif
			}
		}
	}
return 0;
}

#endif




/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 * 中文说明：MCU 上电或从 DeepSleep（非 retention）唤醒后的用户初始化入口。
 * 依次完成：随机数发生器初始化、调试串口初始化、Flash 尺寸自适应配置、
 * MAC 地址初始化、Controller 基础模块/广播/扫描/连接从机模块初始化，
 * 并根据 FEATURE_TEST_MODE 选择的场景设置广播参数、扫描参数及事件回调，
 * 最后配置低功耗挂起策略并做初始化自检。
 */
void user_init_normal(void)
{
	//random number generator must be initiated here( in the beginning of user_init_nromal)
	//when deepSleep retention wakeUp, no need initialize again
#if(MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
	random_generator_init();  //this is must
#endif

	#if(UART_PRINT_DEBUG_ENABLE)
		tlkapi_debug_init();
		blc_debug_enableStackLog(STK_LOG_DISABLE);
	#endif

	blc_readFlashSize_autoConfigCustomFlashSector();

	/* attention that this function must be called after "blc_readFlashSize_autoConfigCustomFlashSector" !!!*/
	blc_app_loadCustomizedParameters_normal();

////////////////// BLE stack initialization ////////////////////////////////////
	u8  mac_public[6];
	u8  mac_random_static[6];
	//for 512K Flash, flash_sector_mac_address equals to 0x76000
	//for 1M  Flash, flash_sector_mac_address equals to 0xFF000
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);
	tlkapi_send_string_data(APP_LOG_EN,"[APP][INI]Public Address", mac_public, 6);


	////// Controller Initialization  //////////
	blc_ll_initBasicMCU();                      //mandatory
	blc_ll_initStandby_module(mac_public);				//mandatory

	rf_set_power_level_index (MY_RF_POWER_INDEX);

#if (FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY || FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE \
	|| FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)
	blc_ll_initAdvertising_module(mac_public);
#endif

#if (FEATURE_TEST_MODE == TEST_SCANNING_ONLY || FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)
	blc_ll_initScanning_module(mac_public);
#endif

#if(FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE || FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)
	blc_ll_initConnection_module();				//connection module  mandatory for BLE slave/master
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,
#endif





#if (FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY)
	/**
	 * @brief	Adv Packet data
	 */
	u8 tbl_advData[] = {
		 0x08, DT_COMPLETE_LOCAL_NAME, 't', 'e', 's', 't', 'a', 'd', 'v',
		 0x02, DT_FLAGS, 0x05,
		};

	/**
	 * @brief	Scan Response Packet data
	 */
	u8	tbl_scanRsp [] = {
			 0x08, DT_COMPLETE_LOCAL_NAME, 'T', 'E', 'S', 'T', 'A', 'D', 'V',	//scan name
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 adv_param_status = BLE_SUCCESS;
	adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_100MS, ADV_INTERVAL_100MS, \
									ADV_TYPE_NONCONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
	if(adv_param_status != BLE_SUCCESS){  //ADV setting err
		tlkapi_printf(APP_LOG_EN, "[APP][INI] ADV parameters error 0x%x!!!\n", adv_param_status);
		while(1);
	}

	//blc_ll_setAdvCustomedChannel(37, 38, 39);
	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable

#elif (FEATURE_TEST_MODE == TEST_SCANNING_ONLY)
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(controller_event_callback);


	////// set scan parameter and scan enable /////
	#if 1  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_INTERVAL_90MS,
							  	  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report ADV only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_INTERVAL_90MS,
							  	  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);
	#endif

	blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

#elif (FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE)
	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	my_att_init(); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	blc_smp_peripheral_init();									//smp initialization

///////////////////// USER application initialization ///////////////////
	/**
	 * @brief	Adv Packet data
	 */
	u8 tbl_advData[] = {
		 0x09, DT_COMPLETE_LOCAL_NAME, 's', 'l', 'a', 'v', 'e', 'a', 'd', 'v',
		 0x02, DT_FLAGS, 0x05,
		};

	/**
	 * @brief	Scan Response Packet data
	 */
	u8	tbl_scanRsp [] = {
			 0x09, DT_COMPLETE_LOCAL_NAME, 'S', 'L', 'A', 'V', 'E', 'A', 'D', 'V',
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 adv_param_status = BLE_SUCCESS;
	adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(adv_param_status != BLE_SUCCESS){  //ADV setting err
		tlkapi_printf(APP_LOG_EN, "[APP][INI] ADV parameters error 0x%x!!!\n", adv_param_status);
		while(1);
	}

	bls_ll_setAdvEnable(BLC_ADV_ENABLE);;  //ADV enable


	//add advertising in connection slave role
	/**
	 * @brief	Adv test Packet data
	 */
	u8 tbl_advData_test[] = {
		 0x09, DT_COMPLETE_LOCAL_NAME, 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
		 0x02, DT_FLAGS, 0x05,
		};

	/**
	 * @brief	Scan Response test Packet data
	 */
	u8	tbl_scanRsp_test [] = {
			 0x09, DT_COMPLETE_LOCAL_NAME, 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
		};
	blc_ll_addAdvertisingInConnSlaveRole();  //ADV in conn slave role
	blc_ll_setAdvParamInConnSlaveRole(  (u8 *)tbl_advData_test, sizeof(tbl_advData_test), \
										(u8 *)tbl_scanRsp_test, sizeof(tbl_scanRsp_test), \
										ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);

	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);

#elif (FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE)
	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	my_att_init(); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	blc_smp_peripheral_init();									//smp initialization

///////////////////// USER application initialization ///////////////////
	/**
	 * @brief	Adv Packet data
	 */
	u8 tbl_advData[] = {
		 0x0A, DT_COMPLETE_LOCAL_NAME, 's', 'l', 'a', 'v', 'e', 's', 'c', 'a', 'n',
		 0x02, DT_FLAGS, 0x05,
		};

	/**
	 * @brief	Scan Response Packet data
	 */
	u8	tbl_scanRsp [] = {
			 0x0A, DT_COMPLETE_LOCAL_NAME, 'S', 'L', 'A', 'V', 'E', 'S', 'C', 'A','N'
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 adv_param_status = BLE_SUCCESS;
	adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(adv_param_status != BLE_SUCCESS){  //ADV setting err
		tlkapi_printf(APP_LOG_EN, "[APP][INI] ADV parameters error 0x%x!!!\n", adv_param_status);
		while(1);
	}


	bls_ll_setAdvEnable(BLC_ADV_ENABLE);;  //ADV enable



	//scan setting
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(controller_event_callback);

	#if 1  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report ADV only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);

	#endif

	blc_ll_addScanningInAdvState();  //add scan in ADV state
	blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role




	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);


#elif (FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)
	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	my_att_init(); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	blc_smp_peripheral_init();									//smp initialization

///////////////////// USER application initialization ///////////////////
	/**
	 * @brief	Adv Packet data
	 */
	u8	tbl_advData[] = {
		 0x05, DT_COMPLETE_LOCAL_NAME, 'f', 'h', 'i', 'd',
		 0x02, DT_FLAGS, 0x05,
		 0x03, DT_APPEARANCE, 0x80, 0x01,
		 0x05, DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID, 0x12, 0x18, 0x0F, 0x18,
	};

	/**
	 * @brief	Scan Response Packet data
	 */
	u8	tbl_scanRsp [] = {
			 0x08, DT_COMPLETE_LOCAL_NAME, 'f', 'e', 'a', 't', 'u', 'r', 'e',
		};

	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 adv_param_status = BLE_SUCCESS;
	adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
	if(adv_param_status != BLE_SUCCESS){  //ADV setting err
		tlkapi_printf(APP_LOG_EN, "[APP][INI] ADV parameters error 0x%x!!!\n", adv_param_status);
		while(1);
	}


	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable



	//add advertising in connection slave role
	/**
	 * @brief	Adv test Packet data
	 */
	u8 tbl_advData_test[] = {
			 0x09, DT_COMPLETE_LOCAL_NAME, 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
			 0x02, DT_FLAGS, 0x05,
			};

	/**
	 * @brief	Scan Response Packet data
	 */
	u8	tbl_scanRsp_test [] = {
			 0x09, DT_COMPLETE_LOCAL_NAME, 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
		};
	blc_ll_addAdvertisingInConnSlaveRole();  //ADV in conn slave role
	blc_ll_setAdvParamInConnSlaveRole(  (u8 *)tbl_advData_test, sizeof(tbl_advData_test), \
										(u8 *)tbl_scanRsp_test, sizeof(tbl_scanRsp_test), \
										ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);



	//scan setting
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(controller_event_callback);

	#if  1  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report ADV only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);
	#endif

	blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role


	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);
#endif





	///////////////////// Power Management initialization///////////////////
#if(BLE_APP_PM_ENABLE)
	blc_ll_initPowerManagement_module();        //pm module:      	 optional
	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &task_suspend_exit);

	#if (PM_DEEPSLEEP_RETENTION_ENABLE)
		blc_app_setDeepsleepRetentionSramSize(); //select DEEPSLEEP_MODE_RET_SRAM_LOW16K or DEEPSLEEP_MODE_RET_SRAM_LOW32K
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
		blc_pm_setDeepsleepRetentionThreshold(95, 95);

		#if(MCU_CORE_TYPE == MCU_CORE_825x)
			blc_pm_setDeepsleepRetentionEarlyWakeupTiming(240);
		#elif((MCU_CORE_TYPE == MCU_CORE_827x))
			blc_pm_setDeepsleepRetentionEarlyWakeupTiming(270);
		#endif
	#else
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	#endif

#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
#endif

	/* Check if any Stack(Controller & Host) Initialization error after all BLE initialization done.
	 * attention that code will stuck in "while(1)" if any error detected in initialization, user need find what error happens and then fix it */
	blc_app_checkControllerHostInitialization();

	tlkapi_printf(APP_LOG_EN, "[APP][INI] feature_ll_state init \n");
}



/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 * 中文说明：MCU 从 DeepSleep Retention 唤醒后的用户初始化入口，恢复之前保存的
 * 自定义参数与射频功率设置，并调用 Link Layer 的 retention 恢复流程。
 */
_attribute_ram_code_ void user_init_deepRetn(void)
{
#if (PM_DEEPSLEEP_RETENTION_ENABLE)
	blc_app_loadCustomizedParameters_deepRetn();
	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (MY_RF_POWER_INDEX);

	blc_ll_recoverDeepRetention();

	irq_enable();

#endif
}




/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 * 中文说明：BLE 主循环，驱动协议栈处理（blt_sdk_main_loop）及低功耗管理
 * （blt_pm_proc），本示例中无额外 UI 处理。
 */
void main_loop(void)
{

	////////////////////////////////////// BLE entry /////////////////////////////////
	blt_sdk_main_loop();


	////////////////////////////////////// UI entry /////////////////////////////////


	////////////////////////////////////// PM Process /////////////////////////////////
	blt_pm_proc();
}


#endif  //end of (FEATURE_TEST_MODE == ...)
