#include "Communication.hpp"


#ifdef WPT_HARDWARE

ASKData askData;

namespace ASKcomm
{


void init()
{

    HAL_GPIO_WritePin(COMM1_GPIO_Port, COMM1_Pin, GPIO_PIN_SET);
    //HAL_GPIO_WritePin(COMM2_GPIO_Port, COMM2_Pin, GPIO_PIN_SET);


    askData.askLoopIndex = 0;
}

void packData(float wptPower, uint8_t powerRequirement)
{
    static uint8_t packedPower = 0U;
    static uint16_t parity = 0U;

    packedPower = (uint8_t)(M_MAX(wptPower, 0.0f) * (255.0f/150.0f)); // 将功率转换为0-255范围

    askData.wptPowerLimit = packedPower;

    parity = (powerRequirement & 0b1) | (packedPower << 1);
    parity ^= (parity >> 8);
    parity ^= (parity >> 4);
    parity ^= (parity >> 2);
    parity ^= (parity >> 1);

    askData.txMessage = 0x0
        | (powerRequirement & 0b1)  // 功率要求
        | (packedPower << 1)        // uint8格式的功率
        | ((parity & 0b1) << 9);    // 奇偶校验
    
}

void askLoop()    // 在4k循环中调用
{
    if(!askData.enableASK)
    {
        HAL_GPIO_WritePin(COMM1_GPIO_Port, COMM1_Pin, GPIO_PIN_RESET);
        askData.askLoopIndex = 0;
        return;
    }
    
    switch(askData.askLoopIndex)
    {
    case 0:
        COMM1_GPIO_Port->BSRR = (uint32_t)COMM1_Pin;    // 起始位一定为双高
        packData(adcData.pWPTlf, askData.powerRequirement);
        askData.askLoopIndex++;
        break;
    case 1:
        askData.askLoopIndex++;
        break;
    case 22:
        COMM1_GPIO_Port->BSRR = (uint32_t)COMM1_Pin << 16U; // 停止位一定位双低
        askData.askLoopIndex++;
        break;
    case 23:
        askData.askLoopIndex++;
        break;
    case 39:
        COMM1_GPIO_Port->BSRR = (uint32_t)COMM1_Pin << 16U; // 重新开始计数
        askData.askLoopIndex = 0;
        break;
    default:
        if(askData.askLoopIndex & 0b1)
        {
            if(askData.askLoopIndex > 23 || ((askData.txMessage >> ((askData.askLoopIndex >> 1) - 1)) & 0b1))
            {
                HAL_GPIO_TogglePin(COMM1_GPIO_Port, COMM1_Pin);
            }
        }
        else
        {
            HAL_GPIO_TogglePin(COMM1_GPIO_Port, COMM1_Pin);
        }    
    
        askData.askLoopIndex++;
        break;
    } 
}

} // namespace ASKcomm



#endif // WPT_HARDWARE


RxData rxData;
RxData rxData1;
TxData txData;
TxDataNew txDataNew;

namespace CANcomm {
    
FDCAN_TxHeaderTypeDef getTxHeader(uint16_t id) 
{
    FDCAN_TxHeaderTypeDef txHeader = {
            id,
            FDCAN_STANDARD_ID,
            FDCAN_DATA_FRAME,
            FDCAN_DLC_BYTES_8,
            FDCAN_ESI_PASSIVE,
            FDCAN_BRS_OFF,
            FDCAN_CLASSIC_CAN,
            FDCAN_NO_TX_EVENTS,
            0
    };
    return txHeader;
}

static FDCAN_TxHeaderTypeDef txHeader = getTxHeader(0x051);
static FDCAN_TxHeaderTypeDef txHeaderNew = getTxHeader(0x052);

static FDCAN_RxHeaderTypeDef rxHeader = {};

void init() 
{
    static_assert(sizeof(RxData) == 8, "RxData size error");
    static_assert(sizeof(TxData) == 8, "TxData size error");

    FDCAN_FilterTypeDef filter;
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterIndex = 0;
    filter.FilterType = FDCAN_FILTER_DUAL;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1 = 0x061 << 5;
    filter.FilterID2 = 0x061 << 5;

    rxData.enableDCDC = 1;
    rxData.systemRestart = 0;
    rxData.clearError = 0;
    rxData.enableActiveChargingLimit = 0;

    HAL_FDCAN_ConfigFilter(&hfdcan3, &filter);
    HAL_FDCAN_ActivateNotification(&hfdcan3, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_Start(&hfdcan3);
}

static void generateTxData(TxData &td) 
{
    td = {};
    //PowerManager::status.errorCode | ((uint8_t) !PowerManager::status.outputEnabled) << 7;
    td.statusCode = 0x00;
    td.statusCode = (psData.outputABEnabled << 7) | (ctrlData.refLoop.useNewFeedbackMessage << 6) |
                    (ctrlData.wptStatus << 4) |
                    (((ctrlData.limitFactor >= 4) ? 0b11 : (ctrlData.limitFactor & 0x03)) << 2 ) |
                    (errorData.errorLevel & 0x03);
    td.capEnergy = (adcData.vCaplf*adcData.vCaplf * (1/(CAPARR_MAX_VOLTAGE*CAPARR_MAX_VOLTAGE))) * 250U;

    #ifdef WPT_HARDWARE
        if(psData.outputEEnabled)    
            td.chassisPower = adcData.pChassislf - adcData.pWPTlf;
        else
            td.chassisPower = adcData.pChassislf;
    #else
        td.chassisPower = adcData.pChassislf;
    #endif    
    
    td.chassisPowerLimit = CAPARR::getMaxPowerFeedback() + rxData1.refereePowerLimit; //TODO
}

static void generateTxDataNew(TxDataNew &td) 
{
    td = {};
    td.statusCode = 0x00;
    td.statusCode = (psData.outputABEnabled << 7) | (ctrlData.refLoop.useNewFeedbackMessage << 6) |
                    (ctrlData.wptStatus << 4) |
                    (((ctrlData.limitFactor >= 4) ? 0b11 : (ctrlData.limitFactor & 0x03)) << 2 ) |
                    (errorData.errorLevel & 0x03);
    td.capEnergy = (adcData.vCaplf*adcData.vCaplf * (1/(CAPARR_MAX_VOLTAGE*CAPARR_MAX_VOLTAGE))) * 250U;

    #ifdef WPT_HARDWARE
        if(psData.outputEEnabled)    
            td.chassisPower = (adcData.pChassislf - adcData.pWPTlf) * 64U + 16384U;
        else
            td.chassisPower = adcData.pChassislf * 64U + 16384U;
    #else
        td.chassisPower = adcData.pChassislf * 64U + 16384U;
    #endif  
    
    td.refereePower = adcData.pRefereelf * 64U + 16384U;    
    td.chassisPowerLimit = CAPARR::getMaxPowerFeedback() + rxData1.refereePowerLimit; 
}


void sendSCData() 
{
    if (hfdcan3.Instance->PSR & FDCAN_PSR_BO_Msk)
    {
        hfdcan3.Instance->CCCR &= ~FDCAN_CCCR_INIT;
    }

    if(!ctrlData.refLoop.useNewFeedbackMessage)
    {
        generateTxData(txData);
        HAL_FDCAN_AddMessageToTxFifoQ(
            &hfdcan3,
            &txHeader,
            reinterpret_cast<uint8_t *>(&txData)
        );
    }
    else
    {
        generateTxDataNew(txDataNew);
        HAL_FDCAN_AddMessageToTxFifoQ(
            &hfdcan3,
            &txHeaderNew,
            reinterpret_cast<uint8_t *>(&txDataNew)
        );
    }
}

void rxDataHandler(const RxData &rd)
{
    rxData1 = rd;
    
    psData.allowEnableOutput = rd.enableDCDC;
    ctrlData.refLoop.useNewFeedbackMessage = rd.useNewFeedbackMessage;

    if(!rd.enableDCDC && psData.outputABEnabled)
    {
        HRTIM::disableOutputAB();
    }
    if(rd.systemRestart)
    {
        HRTIM::disableOutputAB();
        __disable_irq();
        while (true)
            NVIC_SystemReset();
    }
    if(rd.clearError)
    {
        Protection::autoClearError();
        Protection::manualClearError();
    }
    if(rd.enableActiveChargingLimit)
    {
        ctrlData.vCapArrNormal = 
            M_CLAMP(sqrtf((rd.activeChargingLimitRatio/255.0f)) * CAPARR_MAX_VOLTAGE, CAPARR_LOW_VOLTAGE, CAPARR_MAX_VOLTAGE);    
    }
    else
    {
        ctrlData.vCapArrNormal = CAPARR_MAX_VOLTAGE;
    }
}
}

extern "C" 
{
    void FDCAN3_IT0_IRQHandler(void) 
    {
        HAL_FDCAN_IRQHandler(&hfdcan3);
        
        if ((hfdcan3.Instance->RXF0S & FDCAN_RXF0S_F0FL) == 0U)
            return;
        
        while (HAL_FDCAN_GetRxMessage(&hfdcan3, FDCAN_RX_FIFO0, &CANcomm::rxHeader, (uint8_t *) &rxData) == HAL_OK) 
        {
            if ((CANcomm::rxHeader.Identifier == 0x061) && (CANcomm::rxHeader.DataLength == 0x8) && (CANcomm::rxHeader.IdType == FDCAN_STANDARD_ID)) //
            {
                ctrlData.refLoop.isConnected = 1;
                CANcomm::rxDataHandler(rxData);
                PowerControl::updateRefereePower(rxData1, sysData.vTick);
            }
        }
    }
    // void CANManager::errorStatusCallback(CAN_HANDLE_T hfdcan, uint32_t errorStatusITs)
    // {
    //     if (hfdcan->Instance->PSR & FDCAN_PSR_BO_Msk)
    //     {
    //         pManager->busOffCount++;
    //         hfdcan->Instance->CCCR &= ~FDCAN_CCCR_INIT;
    //     }
    // }
}