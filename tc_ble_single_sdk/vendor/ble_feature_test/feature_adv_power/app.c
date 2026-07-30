/********************************************************************************************************
 * @file    app.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "app_config.h"
#include "app.h"
#include "../default_att.h"

/* 本文件是 BLE 广播(ADV)功耗测试样例：
 * 通过切换下面的 APP_ADV_POWER_TEST_TYPE 宏，可以测试不同 ADV 参数组合
 * （广播数据长度 / 是否可连接 / 广播间隔 / 广播信道数）下芯片的功耗表现，
 * 便于用户结合万用表/功耗分析仪实测电流，评估自己产品的广播策略。
 */
#if (FEATURE_TEST_MODE == TEST_POWER_ADV)

//TEST_POWER_ADV config start
/* ------------------- 可连接广播(connectable ADV) 测试类型 ------------------- *
 * 特点：需要在发送 ADV 包后尝试接收 master 的 scan_req/connect_req，功耗比不可连接广播大 */
#define CONNECT_12B_1S_1CHANNEL      	0	//ADV数据12字节，广播间隔1秒，仅使用1个广播信道(37)
#define CONNECT_12B_1S_3CHANNEL      	1	//ADV数据12字节，广播间隔1秒，使用3个广播信道(37/38/39)
#define CONNECT_12B_500MS_3CHANNEL   	2	//ADV数据12字节，广播间隔500毫秒，3个广播信道
#define CONNECT_12B_30MS_3CHANNEL    	3	//ADV数据12字节，广播间隔30毫秒，3个广播信道(功耗最大的一档)

/* ------------------- 不可连接广播(non-connectable ADV) 测试类型，16字节数据 ------------------- *
 * 特点：只发送数据，不等待/不响应 scan_req，功耗最小 */
#define UNCONNECTED_16B_1S_3CHANNEL       4	//ADV数据16字节，广播间隔1秒，3个广播信道
#define UNCONNECTED_16B_1_5S_3CHANNEL     5	//ADV数据16字节，广播间隔1.5秒，3个广播信道
#define UNCONNECTED_16B_2S_3CHANNEL   	6	//ADV数据16字节，广播间隔2秒，3个广播信道

/* ------------------- 不可连接广播(non-connectable ADV) 测试类型，31字节(满长)数据 ------------------- */
#define UNCONNECTED_31B_1S_3CHANNEL      	7	//ADV数据31字节(BLE广播包最大长度)，广播间隔1秒，3个广播信道
#define UNCONNECTED_31B_1_5S_3CHANNEL     8	//ADV数据31字节，广播间隔1.5秒，3个广播信道
#define UNCONNECTED_31B_2S_3CHANNEL   	9	//ADV数据31字节，广播间隔2秒，3个广播信道(功耗最小的一档)

/* 当前实际生效的测试类型：可在上面10种类型中任选一种赋值，重新编译后即可测试对应功耗场景 */
#define APP_ADV_POWER_TEST_TYPE        UNCONNECTED_31B_2S_3CHANNEL





/* ------------------------- LinkLayer 收发 FIFO 配置 ------------------------- *
 * RX_FIFO：接收缓冲区，每个包大小 RX_FIFO_SIZE，共 RX_FIFO_NUM 个包；
 * TX_FIFO：发送缓冲区，每个包大小 TX_FIFO_SIZE，共 TX_FIFO_NUM 个包。
 * 数量必须是 2 的幂次(4/8/16/32...)，大小需按芯片手册的公式对齐 */
#define RX_FIFO_SIZE	64	//单个接收包缓冲区大小(字节)
#define RX_FIFO_NUM		8	//接收缓冲区个数，必须是2的幂次

#define TX_FIFO_SIZE	40	//单个发送包缓冲区大小(字节)
#define TX_FIFO_NUM		8	//发送缓冲区个数，必须是2的幂次


/* 接收 FIFO 实际数据缓冲区，放在 retention 区(深度睡眠恢复时数据不丢失) */
_attribute_data_retention_  u8 		 	blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
/* 接收 FIFO 控制结构体：描述缓冲区大小/个数/读写指针/数据区地址 */
_attribute_data_retention_	my_fifo_t	blt_rxfifo = {
												RX_FIFO_SIZE,
												RX_FIFO_NUM,
												0,
												0,
												blt_rxfifo_b,};

/* 发送 FIFO 实际数据缓冲区 */
_attribute_data_retention_  u8 		 	blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
/* 发送 FIFO 控制结构体 */
_attribute_data_retention_	my_fifo_t	blt_txfifo = {
												TX_FIFO_SIZE,
												TX_FIFO_NUM,
												0,
												0,
												blt_txfifo_b,};

/* 射频发射功率档位：这里固定使用 0dBm 做功耗基准测试，数值越大功耗越高 */
#define		MY_RF_POWER_INDEX					RF_POWER_P0dBm






/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_SUSPEND_EXIT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 *
 * 中文说明：suspend(挂起)/deepSleep retention(深度休眠保留) 唤醒退出后的回调。
 * 芯片每次从 suspend 或 deepSleep retention 唤醒后，之前设置的射频发射功率
 * 寄存器会被复位，所以必须在这里重新设置一次功率档位，否则唤醒后广播功率会不对。
 */
void task_suspend_exit (u8 e, u8 *p, int n)
{
	(void)e;(void)p;(void)n;
	rf_set_power_level_index (MY_RF_POWER_INDEX);	//重新设置射频发射功率档位
	tlkapi_printf(APP_LOG_EN, "[APP][PM] suspend/deepSleep exit, restore RF power index\n");	//中文：新增日志，每次从suspend/深度休眠唤醒退出时打印一条日志，方便通过UART观察唤醒节奏
}





/**
 * @brief      power management code for application
 * @param	   none
 * @return     none
 *
 * 中文说明：应用层电源管理处理函数，在 main_loop 中每次循环都会调用。
 * 这里只是设置协议栈的睡眠掩码(suspend mask)，告诉底层协议栈：
 * 广播态(ADV)和连接态(CONN)都允许进入 suspend，如果开启了深度休眠保留
 * (PM_DEEPSLEEP_RETENTION_ENABLE)，还允许在这两种状态下进入更省电的
 * deepSleep retention 模式(带 SRAM 保留，唤醒更快)。
 * 本样例只关注广播功耗测试，未做空闲态(idle)超时进入 deepSleep 的逻辑。
 */
void blt_pm_proc(void)
{
#if(BLE_APP_PM_ENABLE)
	#if (PM_DEEPSLEEP_RETENTION_ENABLE)
		//广播和连接状态都允许 suspend，并且允许深度休眠保留(功耗最低)
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
	#else
		//仅允许普通 suspend，不使用深度休眠保留
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	#endif
#endif  //end of BLE_APP_PM_ENABLE
}




/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 */
/**
 * 中文说明：user_init_normal —— 芯片正常上电(冷启动)或从深度休眠(非retention模式)
 * 唤醒时调用的用户初始化函数，是本样例最核心的初始化流程，主要做：
 * 1) 基础硬件初始化(随机数发生器/调试串口/Flash容量识别/用户参数加载)
 * 2) BLE Controller(链路层)初始化
 * 3) 配置广播数据、广播参数(间隔/信道/是否可连接)——这是本功耗测试样例的重点
 * 4) 电源管理(PM)配置
 */
_attribute_no_inline_ void user_init_normal(void)
{
	//random number generator must be initiated here( in the beginning of user_init_nromal)
	//when deepSleep retention wakeUp, no need initialize again
	//中文：随机数发生器必须在 user_init_normal 最开始就初始化(BLE协议栈内部用它生成随机地址/加密相关随机数等)，
	//     深度休眠 retention 唤醒时因为 SRAM 数据还在，不需要重新初始化，所以走 user_init_deepRetn 分支
#if(MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
	random_generator_init();  //this is must　//中文：必须调用，825x/827x平台的随机数发生器初始化
#endif

	//	debug init　//中文：调试功能初始化
	#if(UART_PRINT_DEBUG_ENABLE)
		tlkapi_debug_init();	//中文：初始化调试串口打印功能(本样例默认UART_PRINT_DEBUG_ENABLE=0，未启用)
		blc_debug_enableStackLog(STK_LOG_DISABLE);	//中文：关闭协议栈内部调试日志，避免打印过多影响功耗测试准确性
	#endif


	//中文：自动读取 Flash 容量并据此配置用户自定义 Flash 扇区地址(如 MAC 地址、SMP绑定信息等存储区)
	blc_readFlashSize_autoConfigCustomFlashSector();

	/* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
	//中文：加载出厂/用户自定义参数(如射频校准值)，必须在上面的 Flash 容量配置之后调用
	blc_app_loadCustomizedParameters_normal();



//////////////////////////// basic hardware Initialization  End //////////////////////////////////




//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////
	//////////// Controller Initialization  Begin ///////////////////////// //中文：BLE Controller(链路层)初始化开始
	u8  mac_public[6];			//公有(Public)MAC地址，6字节
	u8  mac_random_static[6];	//随机静态(Random Static)MAC地址，6字节，本样例默认未使用(见下方仅用 mac_public)
	//for 512K Flash, flash_sector_mac_address equals to 0x76000
	//for 1M  Flash, flash_sector_mac_address equals to 0xFF000
	//中文：从 Flash 的 flash_sector_mac_address 扇区读取(若未烧录则随机生成并写入)公有地址和随机静态地址；
	//     512K容量Flash该扇区地址是0x76000，1M容量Flash是0xFF000，具体地址由 blc_readFlashSize_autoConfigCustomFlashSector 自动计算
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);
	tlkapi_send_string_data(APP_LOG_EN,"[APP][INI]Public Address", mac_public, 6);	//打印公有地址(需要打开APP_LOG_EN和UART调试)
	tlkapi_printf(APP_LOG_EN, "[APP][INI] ADV power test type = %d\n", (int)APP_ADV_POWER_TEST_TYPE);	//中文：新增日志，打印当前生效的ADV功耗测试档位编号，方便确认当前烧录的固件是哪种测试配置


	blc_ll_initBasicMCU();                      //mandatory　//中文：链路层基础MCU初始化，必须调用
	blc_ll_initStandby_module(mac_public);		//mandatory　//中文：待机(standby)模块初始化，传入公有地址，必须调用
	blc_ll_initAdvertising_module(mac_public); 	//legacy advertising module: mandatory for BLE slave　//中文：传统广播(legacy ADV)模块初始化，BLE从机(Slave)必须调用


	//////////// Controller Initialization  End ///////////////////////// //中文：Controller初始化结束
	//when debugging, if long time deepSleep retention or suspend happens quickly after power on, it will make "ResetMCU" very hard, so add some time here
	//中文：仅用于调试！如果上电后很快进入深度休眠或长时间suspend，会导致后续用调试器复位芯片变得很困难，
	//     这里加一段延时方便开发调试时能及时按下复位；正式量产版本应删除此行
	sleep_us(2000000);  //only for debug



	//////////// HCI Initialization  Begin ///////////////////////// //中文：本样例不使用独立HCI层(单芯片方案)，此处为空

	//////////// HCI Initialization  End   /////////////////////////


	//////////// Host Initialization  Begin ///////////////////////// //中文：Host层初始化开始(本样例主要是广播参数配置)
	/******************************************************************************************************
	 * Here are just some ADV power example
	 * 中文说明：下面是几种典型 ADV 功耗测试示例，实际功耗受多个 ADV 参数影响：
	 * The actual measured power is affected by several ADV parameters, such as:
	 * 1. ADV data length: long ADV data means bigger power
	 *
	 * 2. ADV type:   non_connectable undirected: ADV power is small, cause only data sending involved, no
	 *                                           need receiving any packet from master
	 *                connectable ADV: must try to receive scan_req/scan_conn from master after sending adv
	 *                                           data, so power is bigger.
	 *                                               And if needing send scan_rsp to master's scan_req,
	 *                                           power will increase. Here we can use whiteList to disable scan_rsp.
	 *											     With connectable ADV, user should test power under a clean
	 *											 and shielded environment to avoid receiving scan_req/conn_req
	 *
	 * 3. ADV power index: We use 0dBm in examples, higher power index will cause poser to increase
	 *
	 * 4. ADV interval: Bigger ADV interval lead to smaller power, cause more timing for suspend/deepSleep retention
	 *
	 * 5. ADV channel: Power with 3 channel is bigger than power with 1 or 2 channel
	 *
	 *
	 * If you want test ADV power with different ADV parameters from our examples, you should modify these
	 *      parameters in code, and re_test by yourself.
	 * 中文：如果想测试其他参数组合，直接在代码里修改对应参数后重新编译测试即可。
	 *****************************************************************************************************/

		//set to special ADV channel can avoid master's scan_req to get a very clean power,
		// but remember that special channel ADV packet can not be scanned by BLE master and captured by BLE sniffer
		//中文：如果设置成非标准广播信道(如33/34/35)，可以避免master发scan_req来干扰功耗测试，得到最“干净”的电流曲线，
		//     但要注意：非标准信道的广播包手机/抓包工具(sniffer)是收不到的，仅用于纯功耗测量
	//	blc_ll_setAdvCustomizedChannel(33,34,35);
	u8 adv_param_status = BLE_SUCCESS;	//广播参数设置返回值，用于错误检测
	#if APP_ADV_POWER_TEST_TYPE < UNCONNECTED_16B_1S_3CHANNEL  // connectable undirected ADV　//中文：可连接非定向广播(索引0~3)分支
		/**
		 * @brief	Adv Packet data
		 */
		//ADV data length: 12 byte　//中文：广播数据包，长度12字节(含本地名称"testadv"和Flags字段)
		u8 tbl_advData[12] = {
			 0x08, DT_COMPLETE_LOCAL_NAME, 't', 'e', 's', 't', 'a', 'd', 'v',	//完整本地名称: "testadv"
			 0x02, DT_FLAGS, 0x05,												//Flags: 有限可发现 + 不支持BR/EDR
			};

		/**
		 * @brief	Scan Response Packet data
		 */
		//中文：扫描响应包(Scan Response)，当ADV是可连接类型时，master发scan_req后本机会回复此包
		u8	tbl_scanRsp [] = {
				 0x08, DT_COMPLETE_LOCAL_NAME, 'T', 'E', 'S', 'T', 'A', 'D', 'V',	//scan name　//扫描响应中的本地名称: "TESTADV"
			};

		bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );			//设置广播数据
		bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));		//设置扫描响应数据

		#if APP_ADV_POWER_TEST_TYPE == CONNECT_12B_1S_1CHANNEL
			// ADV data length:	12 byte
			// ADV type: 		connectable undirected ADV
			// ADV power index: 3 dBm
			// ADV interval: 	1S
			// ADV channel: 	1 channel
			//中文：可连接非定向广播，12字节数据，1秒间隔，仅用37信道 —— 功耗最小的“可连接”档位
			adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_1S, ADV_INTERVAL_1S,ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL);  //no scan, no connect　//中文：白名单模式，实测时不被外部scan/connect打断

		#elif APP_ADV_POWER_TEST_TYPE == CONNECT_12B_1S_3CHANNEL
			// ADV data length:	12 byte
			// ADV type: 		connectable undirected ADV
			// ADV power index: 3 dBm
			// ADV interval: 	1S
			// ADV channel: 	3 channel
			//中文：可连接非定向广播，12字节数据，1秒间隔，3个信道(37/38/39全用) —— 比上面多2个信道，功耗略增
			adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_1S, ADV_INTERVAL_1S, \
											ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
											 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL);  //no scan, no connect

		#elif APP_ADV_POWER_TEST_TYPE == CONNECT_12B_500MS_3CHANNEL
			// ADV data length:	12 byte
			// ADV type: 		connectable undirected ADV
			// ADV power index: 3 dBm
			// ADV interval: 	500 mS
			// ADV channel: 	3 channel
			//中文：可连接非定向广播，12字节数据，500毫秒间隔，3个信道 —— 广播频率翻倍，功耗随之上升
			adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_500MS, ADV_INTERVAL_500MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL);  //no scan, no connect

		#elif APP_ADV_POWER_TEST_TYPE == CONNECT_12B_30MS_3CHANNEL
			// ADV data length:	12 byte
			// ADV type: 		connectable undirected ADV
			// ADV power index: 3 dBm
			// ADV interval: 	30 mS
			// ADV channel: 	3 channel
			//中文：可连接非定向广播，12字节数据，30毫秒间隔(近似快速重连场景)，3个信道 —— 本组功耗最大的一档
			adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL);  //no scan, no connect
		#endif

	#else  // non_connectable undirected ADV, no need scanRsp　//中文：不可连接非定向广播分支(索引4~9)，不需要scanRsp

		/**
		 * @brief	Adv Packet data
		 */
		#if APP_ADV_POWER_TEST_TYPE < UNCONNECTED_31B_1S_3CHANNEL
			//ADV data length: 16 byte　//中文：16字节广播数据(本地名称"testadv89ABCDE")
			u8 tbl_advData[] = {
				 0x0F, DT_COMPLETE_LOCAL_NAME, 't', 'e', 's', 't', 'a', 'd', 'v', '8', '9', 'A', 'B', 'C', 'D', 'E',
				};
		#else  	//ADV data length: max 31 byte　//中文：31字节广播数据(BLE广播包允许的最大长度)
			u8 tbl_advData[] = {
				 0x1E, DT_COMPLETE_LOCAL_NAME, 't', 'e', 's', 't', 'a', 'd', 'v', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D'
			};
		#endif


		bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );	//设置广播数据(不可连接广播无需设置scanRsp)

		#if APP_ADV_POWER_TEST_TYPE == UNCONNECTED_16B_1S_3CHANNEL || APP_ADV_POWER_TEST_TYPE == UNCONNECTED_31B_1S_3CHANNEL
			// ADV type: non_connectable undirected ADV
			// ADV power index: 3 dBm
			// ADV interval: 1S
			// ADV channel: 3 channel
			//中文：不可连接非定向广播，1秒间隔，3个信道 —— 对应16字节或31字节两种数据长度
			adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_1S, ADV_INTERVAL_1S, ADV_TYPE_NONCONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);

		#elif APP_ADV_POWER_TEST_TYPE == UNCONNECTED_16B_1_5S_3CHANNEL || APP_ADV_POWER_TEST_TYPE == UNCONNECTED_31B_1_5S_3CHANNEL
			// ADV type: non_connectable undirected ADV
			// ADV power index: 3 dBm
			// ADV interval: 1.5S
			// ADV channel: 3 channel
			//中文：不可连接非定向广播，1.5秒间隔，3个信道 —— 广播频率降低，功耗比1S档更低
			adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_1S5, ADV_INTERVAL_1S5, ADV_TYPE_NONCONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);

		#elif APP_ADV_POWER_TEST_TYPE == UNCONNECTED_16B_2S_3CHANNEL || APP_ADV_POWER_TEST_TYPE == UNCONNECTED_31B_2S_3CHANNEL
			// ADV type: non_connectable undirected ADV
			// ADV power index: 3 dBm
			// ADV interval: 2S
			// ADV channel: 3 channel
			//中文：不可连接非定向广播，2秒间隔，3个信道 —— 本文件默认使用的档位(APP_ADV_POWER_TEST_TYPE=UNCONNECTED_31B_2S_3CHANNEL)，
			//     配合31字节满长数据，是“不可连接+最长数据+最长间隔”的组合，代表广播平均功耗最低的一类典型场景
			adv_param_status = bls_ll_setAdvParam( ADV_INTERVAL_2S, ADV_INTERVAL_2S, ADV_TYPE_NONCONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
		#endif
	#endif


	//中文：广播参数设置出错则打印错误信息并卡死在 while(1)，方便开发阶段第一时间发现参数配置错误(如间隔超范围)
	if(adv_param_status != BLE_SUCCESS) {
		tlkapi_printf(APP_LOG_EN, "[APP][INI] ADV parameters error 0x%x!!!\n", adv_param_status);
		while(1);
	}

	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable　//中文：使能广播，设置完成后芯片开始按上面配置的参数发送广播包
	tlkapi_printf(APP_LOG_EN, "[APP][INI] ADV enabled, param_status = 0x%x\n", adv_param_status);	//中文：新增日志，确认广播已使能及参数设置返回状态

	/* set RF power index, user must set it after every suspend wake_up, because relative setting will be reset in suspend */
	//中文：设置射频发射功率档位。注意：每次从suspend唤醒后该寄存器设置会丢失，所以这里初始化时也要设置一次，
	//     唤醒后的重新设置则由 task_suspend_exit 回调函数负责
	rf_set_power_level_index (MY_RF_POWER_INDEX);



	///////////////////// Power Management(电源管理) initialization ///////////////////////
#if(BLE_APP_PM_ENABLE)
	blc_ll_initPowerManagement_module();	//中文：初始化电源管理模块，必须先调用才能使用suspend/deepSleep功能

	#if (PM_DEEPSLEEP_RETENTION_ENABLE)
    	blc_app_setDeepsleepRetentionSramSize(); //select DEEPSLEEP_MODE_RET_SRAM_LOW16K or DEEPSLEEP_MODE_RET_SRAM_LOW32K　//中文：选择深度休眠保留时SRAM保留大小(低16K或低32K)
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);	//广播/连接态都允许suspend和深度休眠保留
		blc_pm_setDeepsleepRetentionThreshold(50, 50);	//中文：设置进入深度休眠保留的最短剩余时间阈值(单位:%)，广播/连接各50
		blc_pm_setDeepsleepRetentionEarlyWakeupTiming(200);	//中文：提前200us唤醒，预留芯片恢复运行所需的时间余量
	#else
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);	//中文：仅使用普通suspend，不使用深度休眠保留
	#endif

	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &task_suspend_exit);	//中文：注册“挂起退出”事件回调，用于唤醒后恢复射频功率设置
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);	//中文：不使用电源管理功能，禁止一切低功耗模式(便于纯功能调试)
#endif

	/* Check if any Stack(Controller & Host) Initialization error after all BLE initialization done.
	 * attention that code will stuck in "while(1)" if any error detected in initialization, user need find what error happens and then fix it */
	//中文：检查前面所有Controller和Host初始化是否有错误，如果检测到错误代码会卡在while(1)里，
	//     需要根据错误类型排查具体原因(例如没有正确初始化某个必须的模块)
	blc_app_checkControllerHostInitialization();

	tlkapi_printf(APP_LOG_EN, "[APP][INI] feature_adv_power init \n");	//中文：打印初始化完成日志(需打开APP_LOG_EN和UART调试)

}



/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 */
/**
 * 中文说明：user_init_deepRetn —— 芯片从深度休眠保留(deepSleep retention)模式唤醒时
 * 调用的初始化函数。与 user_init_normal 不同，因为SRAM数据在深度休眠保留期间被保留，
 * 所以这里只需要做少量的恢复性初始化，速度更快、更省电，不需要重新加载所有参数、
 * 也不需要重新初始化随机数发生器/调试串口等。
 * 注意：本函数运行在 RAM 中(_attribute_ram_code_)，因为深度休眠唤醒早期 Flash 可能还不可访问。
 */
_attribute_ram_code_ void user_init_deepRetn(void)
{
#if (PM_DEEPSLEEP_RETENTION_ENABLE)
	blc_app_loadCustomizedParameters_deepRetn();	//中文：加载深度休眠保留场景下需要恢复的参数
	blc_ll_initBasicMCU();   //mandatory　//中文：链路层基础MCU初始化，必须调用(每次唤醒都要重新执行)
	rf_set_power_level_index (MY_RF_POWER_INDEX);	//中文：恢复射频发射功率档位设置(深度休眠会导致该设置丢失)

	blc_ll_recoverDeepRetention();	//中文：恢复链路层深度休眠前保存的状态(如广播/连接上下文)

	irq_enable();	//中文：使能中断

	DBG_CHN0_HIGH;    //debug　//中文：调试用GPIO电平翻转，可用逻辑分析仪测量深度休眠到此处的唤醒耗时
#endif
}


/////////////////////////////////////////////////////////////////////s
// main loop flow
/////////////////////////////////////////////////////////////////////




/**
 * @brief		This is main_loop function
 * @param[in]	none
 * @return      none
 */
/**
 * 中文说明：main_loop —— 主循环函数，在 main.c 的 while(1) 中被反复调用。
 * 本样例逻辑非常简单，因为重点是测试纯广播功耗，没有按键/LED等UI逻辑。
 */
_attribute_no_inline_ void main_loop(void)
{
	////////////////////////////////////// BLE entry ///////////////////////////////// //中文：BLE协议栈主循环入口，处理协议栈内部事务(必须每次循环都调用)
	blt_sdk_main_loop();


	////////////////////////////////////// UI entry ///////////////////////////////// //中文：UI处理入口，本样例无按键/LED等UI逻辑，故为空


	////////////////////////////////////// PM Process ///////////////////////////////// //中文：电源管理处理，负责设置suspend/深度休眠掩码
	blt_pm_proc();



}


#endif  //end of (FEATURE_TEST_MODE == ...)
