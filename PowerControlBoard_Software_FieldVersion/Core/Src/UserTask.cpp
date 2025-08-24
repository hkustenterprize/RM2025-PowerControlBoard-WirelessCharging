#include "main.h"
#include "hrtim.h"
#include "stdint.h"
#include "adc.h"
#include "tim.h"
#include "dac.h"
#include "math.h"

#include "Calibration.hpp"
#include "PowerManager.hpp"
#include "Utility.hpp"
#include "Communication.hpp"
#include "Config.hpp"
#include "Interface.hpp"


// uint16_t deadTime = 50;
// uint32_t dtxr = deadTime | (deadTime << 16);
// hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_E].DTxR = dtxr;

void tickCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim6) // 4kHz tick
    {
        // 以下每个任务以1kHz运行
        // HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, GPIO_PIN_SET);
        // HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, GPIO_PIN_RESET);
        switch (sysData.lfLoopIndex)
        {
        case 0:
            sysData.vTick++;
            Buzzer::update();
            //WS2812::update();
            Protection::errorHandlerLF();
            sysData.lfLoopIndex++;
            break;
        case 1:
            if(sysData.systemInited)
            {
                CANcomm::sendSCData();
                PowerControl::checkRxDataTimeout(sysData.vTick);
                Interface::updateButtonState();
            }
            sysData.lfLoopIndex++;
            break;
        case 2:
            if(psData.outputABEnabled)
            {
                CAPARR::estimateCapacity(sysData.vTick);
            }
            ADC::updateADClf();
            sysData.lfLoopIndex++;
            break;
        case 3:
            Protection::checkLowBattery();
            Interface::updateBuzzerSequence();

            #ifdef WPT_HARDWARE
            
            if(psData.outputEEnabled)
            {
                askData.lastPowerOnTime = sysData.vTick;
                if(adcData.pWPTlf < 3.0f)
                    askData.lowPowerCnt++;
                else
                    askData.lowPowerCnt = 0U;
            }
            else 
            {
                askData.enableASK = 0;
                askData.lowPowerCnt = 0;
                if(adcData.vWPT < adcData.vB + 0.5f && adcData.vCaplf > CAPARR_CUTOFF_VOLTAGE)
                    askData.allowRestart = 1U; // 允许重新启动WPT
            }
            
            #endif // WPT_HARDWARE

            sysData.lfLoopIndex = 0;
            break;
        default:
            break;
        }
        
        // 以下每个任务以4kHz运行
        
        if(sysData.systemInited)
        {
            PowerControl::powerOnOffControl();
            
            #ifdef WPT_HARDWARE

            if(psData.outputEEnabled)
            {
                
                if(adcData.vCaplf > (CAPARR_MAX_VOLTAGE * 1.01f))
                {
                    askData.enableASK = 0;
                    askData.powerRequirement = 0U;

                    ctrlData.wptStatus = WPT_ERROR;
                }
                else if(adcData.vCaplf > (CAPARR_MAX_VOLTAGE *0.99f))
                {
                    askData.powerRequirement = 0U;
                    ctrlData.wptStatus = WPT_FINISHED;
                }
                else
                {
                    if(adcData.vCap * capStatus.maxInCurrent > adcData.pRefereelf + 120.0f)
                        askData.powerRequirement = 1U;
                    else
                        askData.powerRequirement = 0U;
                    ctrlData.wptStatus = WPT_CHARGING;
                }
                
                if(adcData.pWPTlf > 145.0f)
                {
                    askData.enableASK = 0;
                    askData.powerRequirement = 0U;
                    ctrlData.wptStatus = WPT_ERROR;
                } 

                if(!psData.outputABEnabled || adcData.vWPT < adcData.vB || askData.lowPowerCnt > 150U) //adcData.pWPT < 3.0f //!psData.outputABEnabled || 
                {
                    HRTIM::disableOutputE();
                    askData.enableASK = 0;
                    askData.lowPowerCnt = 0U;
                    ctrlData.wptStatus = WPT_OFF;
                }
            }
            else
            {  
                if(psData.outputABEnabled && (adcData.vWPT > 29.55f)) // && sysData.vTick - askData.lastPowerOnTime > 400U//(adcData.vWPT > adcData.vCap + 0.8f && askData.allowRestart) || 
                {
                    HRTIM::enableOutputE(0.96f);
                    askData.allowRestart = 0U;
                    
                    askData.powerRequirement = 0U;
                    askData.enableASK = 1;
                    ctrlData.wptStatus = WPT_CHARGING;
                }
            } 
            ASKcomm::askLoop();
            #endif // WPT_HARDWARE
        }
        
    }
}

void init()
{
    Protection::configAWDG();

    ADC::initAnalog();
    ADC::initADC();

    CANcomm::init();
    //WS2812::init();
    Buzzer::init();
    
    HAL_TIM_RegisterCallback(&htim6, HAL_TIM_PERIOD_ELAPSED_CB_ID, tickCallback);
    HAL_TIM_Base_Start_IT(&htim6);
    
    HAL_TIM_Base_Start(&htim16);
    
    #ifdef WPT_HARDWARE

    ASKcomm::init();

    #endif // WPT_HARDWARE

    Protection::checkHardwareUID();
    Buzzer::play(800, 150);
    
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 + HRTIM_OUTPUT_TA2);
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1 + HRTIM_OUTPUT_TB2);
    psData.outputABEnabled = 0;
    HRTIM::startTimer();

    HAL_Delay(400);
    sysData.systemInited = true;
}


static void loop()
{
    while (true)
    {
        HAL_Delay(1);
        // WS2812::blink(0, COLOR_BLANK);
        // WS2812::blink(1, COLOR_BLANK);
        // WS2812::blink(2, COLOR_BLANK);
    }
}

extern "C"
{
    void systemStart()
    {
        init();
        loop();
    }
}
