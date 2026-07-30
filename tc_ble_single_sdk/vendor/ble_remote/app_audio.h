/********************************************************************************************************
 * @file    app_audio.h
 *
 * @brief   This is the header file for BLE SDK
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
 * 本文件声明了 BLE Remote 音频（语音遥控）相关的对外接口，包括麦克风 GPIO 初始化、麦克风开关、
 * 语音按键状态处理、音频编码任务、连接参数更新请求及音频状态检测等，具体实现依据 TL_AUDIO_MODE
 * 配置的音频传输模式（GATT/HID、ADPCM/SBC 等）有所不同，详见 app_audio.c。
 */
#ifndef APP_AUDIO_H_
#define APP_AUDIO_H_

#define APP_AUDIO_BT_OPEN    0x01
#define APP_AUDIO_BT_CLOSE   0x00
#define APP_AUDIO_BT_CONFIG  0x02

extern 	unsigned int 		key_voice_pressTick;
extern	unsigned char		ui_mic_enable;
extern	unsigned char 		key_voice_press;
extern	int     			ui_mtu_size_exchange_req;

/**
 * @brief      the func serve to init dmic
 * @param[in]  none
 * @return     none
 */
/* 中文说明：初始化数字麦克风（DMIC）相关 GPIO。 */
void dmic_gpio_reset (void);

/**
 * @brief      the func serve to init amic
 * @param[in]  none
 * @return     none
 */
/* 中文说明：初始化模拟麦克风（AMIC）相关 GPIO，将其恢复为默认（关闭）状态。 */
void amic_gpio_reset (void);

/**
 * @brief      for open the audio and mtu size exchange
 * @param[in]  en   0:close the micphone  1:open the micphone
 * @return     none
 */
/* 中文说明：打开或关闭麦克风采集，并按需触发 MTU 尺寸交换；B85 下走 audio_amic_init 初始化流程。 */
void ui_enable_mic (int en);

/**
 * @brief      for open the audio and mtu size exchange
 * @param[in]  none
 * @return     none
 */
/* 中文说明：语音按键触发后打开麦克风，并按需发起 MTU 尺寸交换请求（仅 TL_AUDIO_RCU_ADPCM_GATT_TLEINK 模式）。 */
void voice_press_proc(void);

/**
 * @brief      audio task in loop for encode and transmit encode data
 * @param[in]  none
 * @return     none
 */
/* 中文说明：音频编码与发送任务，在主循环中周期性调用，将麦克风采集数据编码后通过 GATT Notify 或
 * HID 上报发送给主机。 */
void task_audio (void);

/**
 * @brief      This function serves to Request ConnParamUpdate
 * @param[in]  none
 * @return     none
 */
/* 中文说明：语音传输场景下检测并在合适时机发起连接参数更新请求，以获得更短的连接间隔。 */
void blc_checkConnParamUpdate(void);

/**
 * @brief      audio proc in main loop
 * @param[in]  none
 * @return     none
 */
/* 中文说明：主循环中的音频状态机处理入口，根据麦克风使能状态与超时情况调度 task_audio()。 */
void proc_audio(void);

/**
 * @brief      this function is call back function of audio measurement from server to client
 * @param[in]  p:data pointer.
 * @return     0
 */
/* 中文说明：处理主机（Server）下发的音频控制命令（开启/关闭），仅在 HID Service Channel 音频模式下使用。 */
int server2client_auido_proc(void* p);

/**
 * @brief      this function is used to check audio state
 * @param[in]  none
 * @return     none
 */
/* 中文说明：周期性检查音频开始/结束通知是否发送成功，发送失败则重试。 */
void audio_state_check(void);

/**
 * @brief      this function is used to define what to do when voice key is pressed
 * @param[in]  none
 * @return     none
 */
/* 中文说明：语音键按下时的处理，根据不同音频模式记录按键时间或直接开始/切换语音采集状态。 */
void key_voice_is_press(void);

/**
 * @brief      this function is used to define what to do when voice key is released
 * @param[in]  none
 * @return     none
 */
/* 中文说明：语音键释放时的处理，根据不同音频模式关闭麦克风或发送语音结束通知。 */
void key_voice_is_release(void);

/**
 * @brief      This function is the microphone delay function.
 * @param[in]  delay_time: microphone delay duration, unit is us.
 * @return     none
 */
/* 中文说明：设置麦克风采集数据的延迟清零时间，用于滤除开启麦克风瞬间的噪声数据。 */
void audio_proc_delay(u32 delay_time_us);

#endif /* APP_AUDIO_H_ */
