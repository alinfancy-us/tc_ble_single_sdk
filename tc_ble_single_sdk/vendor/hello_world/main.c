/********************************************************************************************************
 * @file    main.c
 *
 * @brief   TLSR8258F1KAT32（EWN-8258FAT1BA 模组）硬件串口打印 hello world Demo
 *
 *          本 Demo 只针对 TLSR8258F1KAT32（B85 系列），代码中不包含 B87 / TC321X 等其它芯片分支。
 *
 *          功能：不启动 BLE 协议栈，芯片上电后每隔 1 秒通过硬件 UART 打印一次 "hello world"，
 *                用于快速判断模组的供电 / 时钟 / 烧录 / 串口链路是否正常。
 *
 *          串口参数：
 *              UART_TX = PB1（模组 pin9 ） -> USB 转串口适配器的 RX
 *              UART_RX = PB7（模组 pin10） <- USB 转串口适配器的 TX（本 Demo 会把收到的字符回显）
 *              GND               （模组 pin2 ） -- 适配器 GND（必须共地）
 *              115200 8N1，无硬件流控
 *
 *          macOS 查看日志：
 *              ls /dev/cu.*                                   # 先确认串口设备名
 *              minicom -D /dev/cu.usbserial-xxxx -b 115200    # 或 picocom -b 115200 /dev/cu.usbserial-xxxx
 *          注：minicom 进去后需关闭硬件流控（Ctrl-A O -> Serial port setup -> F 改为 No）。
 *
 *          Telink 调试器 + Web BDT 查看日志：
 *              app_config.h 中 UART_PRINT_DEBUG_ENABLE=1，日志会额外镜像到 SDK 自带的
 *              GPIO 模拟调试口（DEBUG_INFO_TX_PIN，本板默认 PB2，固定 1Mbps）。
 *              SWS（模组 pin11）只管烧录/寄存器调试，不传日志；要在 Web BDT 的 Debug
 *              面板看到打印，必须把调试器的 Log/RX 引脚额外接到 PB2。
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"


#if (__PROJECT_8258_HELLO_WORLD__)

/* 中文：硬件 UART 引脚定义。
 * TLSR8258 的 UART TX 可选 PA2/PB1/PC2/PD0/PD3/PD7，RX 可选 PA0/PB0/PB7/PC3/PC5/PD6；
 * EWN-8258FAT1BA 模组引出的正好是 PB1(TX) 和 PB7(RX)。 */
#define DEMO_UART_TX_PIN            UART_TX_PB1
#define DEMO_UART_RX_PIN            UART_RX_PB7


/**
 * @brief   发送一个字节
 *
 * 中文：uart_ndma_send_byte 内部已经做了 TX FIFO 满的等待（非 DMA 轮询发送方式）。
 *       UART_PRINT_DEBUG_ENABLE 打开时，同步镜像一份到 SDK 的 GPIO 模拟调试口，
 *       供 Telink 调试器 + Web BDT 的 Debug 面板查看（引脚见 app_config.h 顶部说明）。
 */
static void uart_put_char(unsigned char c)
{
	uart_ndma_send_byte(c);

#if (UART_PRINT_DEBUG_ENABLE)
	tlkapi_printf(1, "%c", c);
#endif
}

/**
 * @brief   发送一个以 '\0' 结尾的字符串
 */
static void uart_put_string(const char *s)
{
	while (*s) {
		uart_put_char((unsigned char)*s++);
	}
}

/**
 * @brief   以十进制发送一个无符号整数
 *
 * 中文：本 Demo 不引入 printf（会大幅增加固件体积），自己做一个最简的整数转字符串。
 */
static void uart_put_u32(unsigned int v)
{
	char buf[11];					//中文：32 位无符号最大 4294967295，共 10 位
	int  i = 0;

	if (v == 0) {
		uart_put_char('0');
		return;
	}

	while (v && i < (int)sizeof(buf)) {
		buf[i++] = (char)('0' + (v % 10));
		v /= 10;
	}

	while (i--) {					//中文：上面是从低位开始取的，这里逆序输出
		uart_put_char(buf[i]);
	}
}


/**
 * @brief   中断处理入口
 *
 * 中文：本 Demo 采用轮询方式收发，不开启任何中断；
 *       这里保留一个空实现，满足启动文件 (cstartup_825x.S) 对 irq_handler 符号的引用。
 */
_attribute_ram_code_ void irq_handler(void)
{
}


/**
 * @brief   程序入口
 */
_attribute_ram_code_ int main(void)
{
	/* 1. CPU 上电/唤醒初始化，TLSR8258 上无参数，必须最先调用 */
	cpu_wakeup_init();

	/* 2. GPIO 初始化（参数 1：按默认配置初始化所有引脚）。
	 *    注意顺序：先 gpio_init 再 3. 里的 uart_gpio_set，
	 *    否则 PB1/PB7 的 UART 复用功能会被 gpio_init 重新改回普通 GPIO。 */
	gpio_init(1);

	/* 3. 系统时钟初始化（app_config.h 中 CLOCK_SYS_CLOCK_HZ = 16MHz），
	 *    UART 波特率分频依赖系统时钟，所以必须在 uart_init 之前调用 */
	clock_init(SYS_CLK_TYPE);

	/* 4. UART 引脚复用配置：TX -> PB1，RX -> PB7
	 *    （uart_gpio_set 内部会自动打开 input_en 并设置 10K 上拉，防漏电并保证空闲态高电平） */
	uart_gpio_set(DEMO_UART_TX_PIN, DEMO_UART_RX_PIN);

	/* 5. 复位 UART 模块，并同步清零软件侧的收/发索引
	 *    （uart_reset 会把硬件读写指针清零，软件指针必须跟着清，否则数据会错位） */
	uart_reset();
	uart_ndma_clear_tx_index();
	uart_ndma_clear_rx_index();

	/* 6. 波特率初始化：115200 / 8 数据位 / 无校验 / 1 停止位
	 *    （16MHz 下也可等价写成 uart_init(9, 13, PARITY_NONE, STOP_BIT_ONE)） */
	uart_init_baudrate(DEMO_UART_BAUDRATE, CLOCK_SYS_CLOCK_HZ, PARITY_NONE, STOP_BIT_ONE);

	/* 7. 关闭 UART 的 DMA 和中断，使用最简单的非 DMA 轮询收发模式 */
	uart_dma_enable(0, 0);
	uart_irq_enable(0, 0);

#if (UART_PRINT_DEBUG_ENABLE)
	/* 中文：使能 SDK 自带的 GPIO 模拟调试口（固定引脚 DEBUG_INFO_TX_PIN，固定 1Mbps），
	 *       uart_put_char() 会把同样的日志镜像过来，供 Web BDT 的 Debug 面板查看 */
	tlkapi_debug_init();
#endif

	/* 8. 上电横幅，方便区分“刚复位”和“循环中的打印” */
	uart_put_string("\r\n");
	uart_put_string("========================================\r\n");
	uart_put_string("  TLSR8258F1KAT32 UART demo start\r\n");
	uart_put_string("  TX=PB1  RX=PB7  baud=");
	uart_put_u32(DEMO_UART_BAUDRATE);
	uart_put_string(" 8N1\r\n");
	uart_put_string("========================================\r\n");

	unsigned int count   = 0;		//中文：打印计数，持续递增说明芯片没有复位/跑飞
	unsigned int tick_1s = 0;		//中文：1 秒周期的时间基准

	tick_1s = clock_time();

	while (1) {
		/* 中文：每 1 秒打印一次 "hello world <n>" */
		if (clock_time_exceed(tick_1s, 1000 * 1000)) {
			tick_1s = clock_time();

			uart_put_string("hello world ");
			uart_put_u32(count++);
			uart_put_string("\r\n");
		}

		/* 中文：顺便做个 RX 回显，方便验证 PB7 接收链路。
		 * 非 DMA 模式下，FLD_UART_RX_BUF_IRQ_STATUS 为 1 表示 RX FIFO 里有数据。 */
		if (reg_uart_buf_cnt & FLD_UART_RX_BUF_CNT) {
			unsigned char rx = uart_ndma_read_byte();
			uart_put_char(rx);
		}
	}
}

#endif  //end of (__PROJECT_8258_HELLO_WORLD__)
