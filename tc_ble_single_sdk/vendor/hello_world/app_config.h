/********************************************************************************************************
 * @file    app_config.h
 *
 * @brief   TLSR8258F1KAT32 (EWN-8258FAT1BA 模组) 硬件串口打印 hello world Demo —— 配置文件
 *
 *          仅针对 TLSR8258F1KAT32（B85 系列 / CHIP_TYPE_825x），不包含 B87 / TC321X 等其它芯片。
 *
 *          串口定义（与模组引脚一一对应）：
 *              UART_TX = PB1  -> 模组 pin9
 *              UART_RX = PB7  -> 模组 pin10
 *              波特率 115200，8 数据位，无校验，1 停止位，无流控
 *
 *******************************************************************************************************/
#pragma once


///////////////////////// Chip / Board Configuration //////////////////////////////////////////
/* 中文：本 Demo 只支持 TLSR8258（B85 系列）。若工程宏选成了别的芯片，直接编译期报错，
 *       避免误编出 B87 / TC321X 的固件。 */
#if (CHIP_TYPE != CHIP_TYPE_825x)
	#error "This demo only supports TLSR8258 (CHIP_TYPE_825x) !!!"
#endif

/* 中文：板级引脚定义选 825x EVK。本 Demo 的 UART 引脚在代码里显式指定，不依赖板级头文件。 */
#define BOARD_SELECT                                BOARD_825X_EVK_C1T139A30


///////////////////////// UART Configuration //////////////////////////////////////////////////
/* 中文：主打印口用 TLSR8258 的硬件 UART 外设（不是 SDK 默认的 GPIO 位翻转模拟串口）。
 *
 * 为什么主打印口不用 SDK 自带的 tlkapi_printf？
 *   SDK 自带的调试打印是 GPIO 软件模拟串口，波特率被写死为 1000000
 *   （见 vendor/common/tlkapi_debug.h，改成别的值会直接触发 #error），
 *   很多 USB 转串口工具和 minicom 无法稳定收 1Mbps，
 *   所以主打印口改用硬件 UART，波特率可以自由设成通用的 115200。
 *
 * 为什么又把它打开：
 *   Telink 调试器的 SWS 单线调试/烧录和 Web BDT 的 Debug 面板是两根独立的线，
 *   Debug 面板只解析 tlkapi_printf 这一路固定 1Mbps 的 GPIO 协议，看不到上面的硬件 UART。
 *   main.c 里 uart_put_char() 打开时会把同一份日志镜像一份过去，
 *   只需把调试器的 Log/RX 线额外接到 DEBUG_INFO_TX_PIN（本板默认 PB2，与 PB1/PB7 不冲突），
 *   就能在 Web BDT 的 Debug 面板看到和硬件 UART 一样的输出。
 *   不需要 BDT 查看日志时，改回 0 即可省掉这部分体积和逐字节镜像开销。 */
#define UART_PRINT_DEBUG_ENABLE                     1

#define DEMO_UART_BAUDRATE                          115200	//中文：波特率，可按需改成 9600 / 19200 / 460800 等


///////////////////////// System Clock Configuration //////////////////////////////////////////
/* 中文：系统时钟 16MHz，uart_init_baudrate 会依据该值自动算分频系数 */
#define CLOCK_SYS_CLOCK_HZ                          16000000


///////////////////////// Feature Configuration ///////////////////////////////////////////////
/* 中文：不使用电源管理，芯片保持全速运行，方便持续观察串口输出 */
#define BLE_APP_PM_ENABLE                           0
#define PM_DEEPSLEEP_RETENTION_ENABLE               0

/* 中文：不使用按键 / LED 等 UI 功能 */
#define UI_KEYBOARD_ENABLE                          0
#define UI_LED_ENABLE                               0


/* 中文：其余未显式定义的宏，统一从 SDK 默认配置中取默认值 */
#include "vendor/common/default_config.h"
