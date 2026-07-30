#!/usr/bin/env bash
HOMEBREW_API_DOMAIN="https://mirrors.ustc.edu.cn/homebrew-bottles/api" \
HOMEBREW_BOTTLE_DOMAIN="https://mirrors.ustc.edu.cn/homebrew-bottles" \
brew install minicom


# minicom -D /dev/cu.usbmodem0000637288701 -b 1000000



我们用的模组是EWN-8258FAT1BA，模组对应的是TLSR8258F1KAT32，基于当前telink官方sdk的3.4.3版本
进行demo开发：
诉求：
在当前工程的vendor目录下，帮我实现一个只基于TLSR8258F1KAT32 串口日志打印demo（不要给我写其他芯片型号的demo，比如B87，tc321x之类），日志输出是hello world，日志输出串口银脚定义：
UART_TX：PB1，UART_RX：PB7。mac会通过串口工具以及minicom查看串口打印的hello world日志。