#include "Interface.hpp"
#include "string.h"

//extern uint32_t vTick;

InterfaceStatus interfaceStatus;

typedef struct
{
    RGB rgbs[LED_NUM];
    unsigned char updatedFlag;
    unsigned char txFlag;
} RGBStatus;




namespace WS2812
{

static RGBStatus rgbStatus;
static int ws2812_isInit = 0;

void PWM_DMA_TransmitFinshed_Callback(TIM_HandleTypeDef *htim)
{
    if (htim == &WS2812_TIM)
    {
        HAL_TIM_PWM_Stop_DMA(htim, WS2812_TIM_CHANNEL);
        rgbStatus.txFlag = 0;
    }
}

static uint32_t CCRDMABuff[LED_NUM * sizeof(RGB) * 8 + 1];

void init()
{
    if (ws2812_isInit)
        return;
    ws2812_isInit = 1;
    HAL_TIM_RegisterCallback(&WS2812_TIM, HAL_TIM_PWM_PULSE_FINISHED_CB_ID, PWM_DMA_TransmitFinshed_Callback);
}

void update()
{
    if (!rgbStatus.updatedFlag)
        return;
    if (rgbStatus.txFlag)
        return;
    rgbStatus.txFlag = 1;
    unsigned int data;

    /*Pack the RGB Value*/
    for (unsigned int i = 0; i < LED_NUM; i++)
    {
        data = *(unsigned volatile int *)(&rgbStatus.rgbs[i]);
        for (unsigned int j = 0; j < sizeof(RGB) * 8; j++)
            CCRDMABuff[i * sizeof(RGB) * 8 + j] = ((1UL << (23 - j)) & data) ? BIT1_WIDTH : BIT0_WIDTH;
    }
    CCRDMABuff[LED_NUM * sizeof(RGB) * 8] = 0;

    /*Transmit DMA*/
    HAL_TIM_PWM_Start_DMA(&WS2812_TIM, WS2812_TIM_CHANNEL, CCRDMABuff, LED_NUM * sizeof(RGB) * 8 + 1);
    rgbStatus.updatedFlag = 0;
}

void blink(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= LED_NUM || rgbStatus.txFlag)
        return;
    rgbStatus.updatedFlag = 1;
    rgbStatus.rgbs[index].blue = b;
    rgbStatus.rgbs[index].green = g;
    rgbStatus.rgbs[index].red = r;
}

void blink(uint8_t index, uint32_t colorCode)
{
    if (index >= LED_NUM || rgbStatus.txFlag)
        return;
    rgbStatus.updatedFlag = 1;
    rgbStatus.rgbs[index].red = (colorCode >> 16) & 0xFF;
    rgbStatus.rgbs[index].green = (colorCode >> 8) & 0xFF;
    rgbStatus.rgbs[index].blue = colorCode & 0xFF;
}


}  // namespace WS2812


namespace Buzzer
{

uint32_t stopTime = 0;

void init()
{
    __HAL_TIM_SET_AUTORELOAD(&BUZZER_TIM, 10000000U / 2500U - 1);
    __HAL_TIM_SET_COMPARE(&BUZZER_TIM, BUZZER_TIM_CHANNEL, 0U);
    HAL_TIMEx_PWMN_Start(&BUZZER_TIM, BUZZER_TIM_CHANNEL);
}

void stop()
{
    __HAL_TIM_SET_COMPARE(&BUZZER_TIM, BUZZER_TIM_CHANNEL, 0U);
}

void play(uint16_t freq, uint16_t duration)
{
    __HAL_TIM_SET_AUTORELOAD(&BUZZER_TIM, 10000000U / freq - 1);
    __HAL_TIM_SET_COMPARE(&BUZZER_TIM, BUZZER_TIM_CHANNEL, 5000000U / freq);
    stopTime = sysData.vTick + duration;
}

void update()
{
    if (sysData.vTick >= stopTime)
    {
        stop();
    }
}

} // namespace Buzzer

namespace Interface
{

void updateButtonState()
{
    if(HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin))
    {
        if(sysData.buttonPressedLast && (sysData.buttonCnt > 1000 && sysData.buttonCnt < 2000))
        {
            if(errorData.errorLevel == ERROR_RECOVER_MANUAL)
            {
                Protection::manualClearError();
            }
            else if(errorData.errorLevel == ERROR_RECOVER_AUTO)
            {
                Protection::autoClearError();
            }
        }
        sysData.buttonPressed = 0;
        sysData.buttonCnt = 0;
    }
    else
    {
        sysData.buttonPressed = 1;
        sysData.buttonCnt++;
        if(sysData.buttonCnt > 3000)
        {
            HRTIM::disableOutputAB();
            __disable_irq();
            while (true)
                NVIC_SystemReset();
        }
    }
    sysData.buttonPressedLast = sysData.buttonPressed;
}


void updateBuzzerSequence()
{
    interfaceStatus.isWarningLast = interfaceStatus.isWarning;
    if(errorData.errorLevel)
    {
        interfaceStatus.isWarning = 1;
        if(!interfaceStatus.isWarningLast)
        {
            interfaceStatus.buzzerSequenceCnt = 0;
            switch (errorData.errorLevel)
            {
            case ERROR_UNRECOVERABLE:
                if(errorData.errorCode & ERROR_POWERSTAGE)
                    memcpy(interfaceStatus.buzzerNote, buzzerWS_Unrecoverable, sizeof(buzzerWS_Unrecoverable));
                break;
            case ERROR_RECOVER_MANUAL:
                if(errorData.errorCode & ERROR_SCP_B)
                    memcpy(interfaceStatus.buzzerNote, buzzerWS_SCPB, sizeof(buzzerWS_SCPB));
                else
                    memcpy(interfaceStatus.buzzerNote, buzzerWS_SCPA, sizeof(buzzerWS_SCPA));
                break;
            case ERROR_RECOVER_AUTO:
                if(errorData.errorCode & ERROR_OCP_A)
                    memcpy(interfaceStatus.buzzerNote, buzzerWS_OCPA, sizeof(buzzerWS_OCPA));
                else if(errorData.errorCode & ERROR_OCP_B)
                    memcpy(interfaceStatus.buzzerNote, buzzerWS_OCPB, sizeof(buzzerWS_OCPB));
                else if(errorData.errorCode & ERROR_OCP_R)
                    memcpy(interfaceStatus.buzzerNote, buzzerWS_OCPR, sizeof(buzzerWS_OCPR));
                else if(errorData.errorCode & ERROR_OVP_A)
                    memcpy(interfaceStatus.buzzerNote, buzzerWS_OVPA, sizeof(buzzerWS_OVPA));
                else
                    memcpy(interfaceStatus.buzzerNote, buzzerWS_OVPB, sizeof(buzzerWS_OVPB));

                break;
            case WARNING:
                if(errorData.errorCode & WARNING_LOWBATTERY)
                    memcpy(interfaceStatus.buzzerNote, buzzerWS_LowBattery, sizeof(buzzerWS_LowBattery));
                break;
            default:
                break;
            }
        }
    }   
    else
    {
        interfaceStatus.isWarning = 0;
        interfaceStatus.buzzerSequenceCnt = 0;
        interfaceStatus.noteIndex = 0;
        return;
    }
    
    if(interfaceStatus.buzzerSequenceCnt == interfaceStatus.buzzerNote[interfaceStatus.noteIndex].startTime)
    {
        Buzzer::play(interfaceStatus.buzzerNote[interfaceStatus.noteIndex].freq, 
        interfaceStatus.buzzerNote[interfaceStatus.noteIndex].duration);
        interfaceStatus.noteIndex++;
    }

    interfaceStatus.buzzerSequenceCnt ++;
    if(interfaceStatus.buzzerSequenceCnt >= WARNING_PERIOD)
    {
        Protection::autoClearError();
        interfaceStatus.buzzerSequenceCnt = 0;
        interfaceStatus.noteIndex = 0;
    }    
}


} // namespace Interface