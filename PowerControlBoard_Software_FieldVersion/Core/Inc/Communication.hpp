#pragma once

#include "main.h"
#include "tim.h"
#include "adc.h"
#include "stdint.h"
#include "Config.hpp"
#include "fdcan.h"
#include "dma.h"


#include "Calibration.hpp"
#include "math.h"
#include "PowerManager.hpp"


#ifdef WPT_HARDWARE

#define ASK_COMM_FREQ           2000U
#define ASK_COMM_TIM            &htim8
#define ASK_COMM_TIM_CHANNEL1   TIM_CHANNEL_1
#define ASK_COMM_TIM_CHANNEL2   TIM_CHANNEL_2

#define ASK_COMM_BIT_HIGH       0x8000
#define ASK_COMM_BIT_LOW        0x0000

#define ASK_COMM_PERIOD         20U


struct ASKData
{
    bool enableASK = true; // 是否启用ASK通信
    uint16_t txMessage;
    uint8_t askLoopIndex;
    uint32_t txBitSequence; // 当前发送的bit序列


    uint8_t wptPowerLimit; // WPT功率，单位W
    uint8_t powerRequirement = 1U; // 功率要求，0-1

    uint16_t lowPowerCnt = 0;
    uint32_t lastPowerOnTime = 0;

    bool allowRestart = 0;
};

extern ASKData askData;

namespace ASKcomm
{
    void init();
    
    void askLoop();    // 在1k循环中跑
} // namespace ASKcomm

#endif // WPT_HARDWARE


struct RxData {
    uint8_t enableDCDC: 1;                  // 允许启动DCDC
    uint8_t systemRestart: 1;               // 系统重启
    uint8_t resv0: 3;
    uint8_t clearError: 1;                  // 手动清除可清除的错误
    uint8_t enableActiveChargingLimit: 1;   // 是否启用主动充电限制
    uint8_t useNewFeedbackMessage: 1;       // 是否使用新的反馈消息格式

    uint16_t refereePowerLimit;             // 裁判限制功率，单位W
    uint16_t refereeEnergyBuffer;           // 裁判能量缓冲，单位J
    uint8_t activeChargingLimitRatio;       // 主动充电限制比例（能量），0-255
    int16_t resv2;
} __attribute__((packed));

extern RxData rxData;
extern RxData rxData1;

struct TxData {                     // 0x051 (useNewFeedbackMessage = 0)
    uint8_t statusCode;             // 状态信息
    float chassisPower;             // 底盘功率，单位W
    uint16_t chassisPowerLimit;     // 底盘最大可用功率（包括裁判系统）
    uint8_t capEnergy;              // 电容现有能量，0-255
} __attribute__((packed));

struct TxDataNew {                  // 0x052 (useNewFeedbackMessage = 1)
    uint8_t statusCode;             // 状态信息
    uint16_t chassisPower;          // 底盘功率，功率*64+16384 (-256W~+768W, 精度0.015625)
    uint16_t refereePower;          // 裁判系统功率，功率*64+16384 (-256W~+768W, 精度0.015625)
    uint16_t chassisPowerLimit;     // 底盘最大可用功率（包括裁判系统）
    uint8_t capEnergy;              // 电容现有能量，0-255
} __attribute__((packed));


// 开启DCDC:1 错误状态:2 

extern TxData txData;
extern TxDataNew txDataNew;

namespace CANcomm 
{
    void init();

    void sendSCData();

    void rxDataHandler(const RxData &rd);

}  // namespace Communication