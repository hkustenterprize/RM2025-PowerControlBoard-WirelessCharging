#pragma once

#include "Calibration.hpp"
#include "Interface.hpp"
#include "Communication.hpp"

#include "main.h"
#include "tim.h"
#include "adc.h"
#include "hrtim.h"
#include "stdint.h"
#include "Config.hpp"
#include "Utility.hpp"

#include "dac.h"
#include "opamp.h"
#include "comp.h"



//#define ADC_BUFFER_SIZE     8U
//#define ADC_RANK_NUM        4U
#define ADC4_BUFFER_SIZE    2U

#define ADC_ISENSE_FILTER_ALPHA 0.9f

#define HRTIM_INT_SCALER    4U


struct RxData;
struct TxData;

struct SystemData
{
    uint32_t vTick = 0;
    bool systemInited = false;
    uint32_t hardwareUID[3] = {0};
    uint16_t buttonCnt = 0;
    bool buttonPressed = 0;
    bool buttonPressedLast = 0;

    uint8_t lfLoopIndex = 0;
};

enum DCDCMode{BUCK, BUCKBOOST, BOOSTBUCK, BOOST, CALIBRATION_A, CALIBRATION_B};
enum PCMMode{IB_VALLEY, IA_PEAK};

struct PowerStageData
{
    bool timerEnabled = 0;
    bool outputABEnabled = 0;
    bool outputEEnabled = 0;
    //bool chargePumpEnabled = true;
    bool allowEnableOutput = 1;

    float dutyByVoltage = 0.0f;
    float efficiency = 1.0f;

    float dutyE = 0.0f; //E侧占空比
    float dutyEMin = 0.0f; //E侧最小占空比
    

    uint8_t softStartCnt = SOFT_START_TIME;
    float iLLimit = MAX_INDUCTOR_CURRENT;

    DCDCMode dcdcMode = BUCK;
    PCMMode pcmMode = IB_VALLEY;
    float iLTarget = 0.0f;
    float IRQload = 0.0f;
};

struct ADCData
{
    bool adcInitialized = 0;
    uint32_t rawData12[4 * HRTIM_INT_SCALER];
    uint32_t sumData[4];

#ifdef CALIBRATION_MODE
    float tempData[7];
#endif    
    uint32_t rawData4[ADC4_BUFFER_SIZE];                //ADC4 原始数据，用于NTC和辅助电源监测
    float iA = 0.0f, iB = 0.0f, iR = 0.0f, iCap;   //电流
    float vA = 0.0f, vB = 0.0f, vCap = 0.0f;              //电压
    float iChassis = 0.0f;              //电容电流，电压
    float pReferee, pChassis;            //裁判系统功率，电容功率，底盘功率，无线充电功率
    float iCaplf = 0.0f, vCaplf = 0.0f; //电容电压，电压
    float pRefereelf = 0.0f, pChassislf = 0.0f;

    float vWPT, iWPT, pWPT, pWPTlf, vWPTlf;

    float vAux;
};



enum LimitFactor
{   
    REFEREE_POWER,
    CAPARR_VOLTAGE_MAX,
    CAPARR_VOLTAGE_NORMAL,
    IB_POSITIVE,
    IB_NEGATIVE,
};

enum ErrorLevel
{
    NO_ERROR = 0,               // 无错误
    ERROR_RECOVER_AUTO = 1,     // 错误，可通过自动恢复
    ERROR_RECOVER_MANUAL = 2,   // 错误，可通过发信息恢复
    ERROR_UNRECOVERABLE = 3,    // 错误，不可恢复
    WARNING                     // 警告
};

enum WPTStatus
{
    WPT_ERROR = 0,      // 非无线充电硬件，或发生错误
    WPT_OFF = 1,        // 无线充电关闭
    WPT_CHARGING = 2,   // 无线充电中
    WPT_FINISHED = 3    // 无线充电完成(电压>98%, 能量大于96%)
};

struct ControlData
{   
    struct RefereeData
    {
        float kP = 1.0f, kI=0.04f, kD=1.5f; //积分增益
        uint16_t lastError = 0.0f; //上次误差
        float integral = 0.0f; //积分值
        int16_t error = 0U;
        uint32_t lastTimestamp = 0;
        float pRefereeBias = 0.0f;
        bool isConnected = 0;
        bool useNewFeedbackMessage = 0;
    };
    
    LimitFactor limitFactor;
    
    RefereeData refLoop;
    
    float pRefereeTarget = REFEREE_DEFUALT_POWER;

    float vCapArrNormal = CAPARR_MAX_VOLTAGE;

    bool allowCharge = false; //是否允许充电

    WPTStatus wptStatus = WPT_OFF; //无线充电状态
    
};

struct LoopControlData
{
    IncreasementPID iRPID {0.1f, 0.2f, 0.10f, 0.01f};
    //IncreasementPID vCapPID {0.0f, 0.0f, 0.02f, 0.0f};

    float currentLimitKI = 0.8f;
    float voltageLimitKI = 0.01f;
    float burstKI = 2.0f;

    float vWPTTarget = 26.2f; //无线充电目标电压
    float wptVoltageKI = 0.001f;

    float deltaIL;
    float dIL_VCap_Max;
    //float dIL_VCap_MaxBurst;
    float dIL_IB_Positive;
    float dIL_IB_Negative;

    float dIL_recoverBurst;


    float deltaDutyE2 = 0.0f;


};


struct CAPARRStatus
{
    struct CapacityEstimateData
    {
        float dQ = 0.0f;
        float lastVCap = 0.0f;
        uint32_t lastTick = 0;
        float dQtodV = CAPARR_DEFUALT_CAPACITY;
        float dVtodQ = (1.0f / CAPARR_DEFUALT_CAPACITY);
        
        float maxIB = 0.0f;
        float minIB = 0.0f;
    };
    
    float maxOutCurrent = 2.0f;
    float maxInCurrent = 2.0f;
    CapacityEstimateData capEstData;

    uint16_t warningCnt = 0; //警告计数
};

struct ErrorData
{
    uint16_t errorCode = 0;
    uint16_t shortCircuitCnt = 0;
    uint16_t overVoltageCnt = 0;
    uint16_t overCurrentCnt = 0;
    uint8_t lowBattery = 0;
    uint16_t lowBatteryCnt = 0; //低电压计数
    ErrorLevel errorLevel     = NO_ERROR;       // 错误等级
    uint32_t powerOffCnt = 0; //关机计数

    float errorVoltage = 0.0f;
    float errorCurrent = 0.0f;
};



extern SystemData sysData;
extern ErrorData errorData;
extern ControlData ctrlData;
extern LoopControlData mfLoop;
extern ADCData adcData; 
extern CAPARRStatus capStatus;             //电容状态
extern PowerStageData psData;


namespace HRTIM
{

void startTimer();

void stopTimer();

bool enableOutputAB();

bool enableOutputE(float dutyE);

void disableOutputAB();

void disableOutputE();


void setOutputAB();

} // namespace HRTIM

namespace ADC
{

void initAnalog();

void initADC();

void processData();

void processADC4Data();

void updateADClf();

void updateADCmf();

}

namespace PowerControl
{

void updateMFLoop();

void setInductorCurrent();

void powerOnOffControl();

void updateRefereePower(const RxData &rd, const uint32_t& currentTick);

void checkRxDataTimeout(const uint32_t& currentTick);

} // namespace PowerControl


namespace Protection
{

void errorCheckHF();
void errorCheckLF();

void checkShortCircuit();
void errorHandlerLF();
void hrtimFaultHandler();

void configAWDG();

void checkHardwareUID();

void checkEfficiency();

void autoClearError();
void manualClearError();

void checkLowBattery();


} // namespace Protection


namespace CAPARR
{

void updateMaxCurrent();

void estimateCapacity(const uint32_t& _currentTick);

void updateCurrentforEstimation();

uint16_t getMaxPowerFeedback();

} // namespace CAPARR
