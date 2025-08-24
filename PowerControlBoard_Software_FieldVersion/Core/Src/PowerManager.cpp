

#include "PowerManager.hpp"
#include "hrtim.h"


SystemData sysData;
ADCData adcData; 
ControlData ctrlData;
CAPARRStatus capStatus;
LoopControlData mfLoop;
ErrorData errorData;
PowerStageData psData;



namespace HRTIM
{

void startTimer()
{
    __HAL_HRTIM_MASTER_ENABLE_IT(&hhrtim1, HRTIM_MASTER_IT_MREP);
    HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_MASTER);
    HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_A);
    HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_B);
    HAL_HRTIM_WaveformCountStart_DMA(&hhrtim1, HRTIM_TIMERID_TIMER_E);
    psData.timerEnabled = 1;
}

void stopTimer()
{
    disableOutputAB();
    __HAL_HRTIM_MASTER_DISABLE_IT(&hhrtim1, HRTIM_MASTER_IT_MREP);
    HAL_HRTIM_WaveformCounterStop(&hhrtim1, HRTIM_TIMERID_MASTER);
    HAL_HRTIM_WaveformCounterStop(&hhrtim1, HRTIM_TIMERID_TIMER_A);
    HAL_HRTIM_WaveformCounterStop(&hhrtim1, HRTIM_TIMERID_TIMER_B);
    psData.timerEnabled = 0;
}

bool enableOutputAB()
{
    if(!psData.timerEnabled || !psData.allowEnableOutput)
        return false;   
    HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 + HRTIM_OUTPUT_TA2);
    HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TB1 + HRTIM_OUTPUT_TB2);
    psData.outputABEnabled = 1;
    Buzzer::play(1250, 200);
    return true;
}

bool enableOutputE(float dutyE)
{
    if(!psData.timerEnabled)//
        return false;   
    __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_E, HRTIM_COMPAREUNIT_1, HRTIM_PERIOD * dutyE);
    HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TE1 + HRTIM_OUTPUT_TE2);
    psData.outputEEnabled = 1;
    Buzzer::play(2500, 100);
    return true;
}

void disableOutputE()
{
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TE1 + HRTIM_OUTPUT_TE2);
    psData.outputEEnabled = 0;
    Buzzer::play(400, 200);
}


void disableOutputAB()
{
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 + HRTIM_OUTPUT_TA2);
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1 + HRTIM_OUTPUT_TB2);
    psData.outputABEnabled = 0;
    Buzzer::play(200, 200);
}

__attribute__((section(".code_in_ram"))) void modeStateMachine()
{
    //根据电压计算占空比
    psData.dutyByVoltage = M_MAX(adcData.vB, 0.01f) / adcData.vA;
    
    //根据占空比进行状态切换
    switch(psData.dcdcMode)
    {
    case BUCK:
        if(psData.dutyByVoltage > 0.84f)
            psData.dcdcMode = BUCKBOOST;
        break;
    case BUCKBOOST:
        if(psData.dutyByVoltage < 0.80f)
            psData.dcdcMode = BUCK;
        else if(psData.dutyByVoltage > 1.02f)
            psData.dcdcMode = BOOSTBUCK;
        break;
    case BOOSTBUCK:
        if(psData.dutyByVoltage < 0.82f)
            psData.dcdcMode = BUCK;    
        else if(psData.dutyByVoltage < 0.98f)
            psData.dcdcMode = BUCKBOOST;
        else if(psData.dutyByVoltage > 1.25f)
            psData.dcdcMode = BOOST;
        break;
    case BOOST:
        if(psData.dutyByVoltage < 0.82f)
            psData.dcdcMode = BUCK;     
        else if(psData.dutyByVoltage < 1.19f)
            psData.dcdcMode = BOOSTBUCK;
        break;
    default:
        break;
    }

    //根据状态操作HRTIM寄存器，并分别计算A和B的占空比
    switch(psData.dcdcMode)
    {
    case BUCK:
        //A侧限制94%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_4, HRTIM_PERIOD * 0.06f);
        //A侧限制0.5%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * 0.998f);
        //B侧常开
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_3, 0U);
        //更新状态和AB占空比
        psData.pcmMode = IB_VALLEY;
        // psData.dutyA = psData.dutyByVoltage;
        // psData.dutyB = 1.0f;
        break;
    case BUCKBOOST:
        //A侧限制94%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_4, HRTIM_PERIOD * 0.06f);
        //A侧限制5%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * 0.95f);
        //B侧固定84%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * 0.16f);
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_4, HRTIM_PERIOD * 0.24f);
        //更新状态和AB占空比
        psData.pcmMode = IB_VALLEY;
        break;
    case BOOSTBUCK:
        //A侧固定84%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * 0.16f);
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_4, HRTIM_PERIOD * 0.24f);
        //B侧限制94%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_4, HRTIM_PERIOD * 0.06f);
        //B侧限制25%占空比，当电压过大时限制占空比
        if(adcData.vB < (CAPARR_MAX_VOLTAGE * 1.01f))
            __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * 0.75f);
        else
            __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * (1-(adcData.vA*0.84f)/VB_LIMIT_BY_DUTY));
        //更新状态和AB占空比
        psData.pcmMode = IA_PEAK;
        break;
    case BOOST:
        //A侧常开
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_3, 0U);
        //B侧限制94%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_4, HRTIM_PERIOD * 0.06f);
        //B侧限制25%占空比，当电压过大时限制占空比
        if(adcData.vB < (CAPARR_MAX_VOLTAGE * 1.01f))
            __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * 0.75f);
        else
            __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * (1-(adcData.vA)/VB_LIMIT_BY_DUTY));
        //更新状态和AB占空比
        psData.pcmMode = IA_PEAK;
        break;
    case CALIBRATION_B:
        //A侧固定80%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * 0.20f);
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_4, HRTIM_PERIOD * 0.35f);
        //B侧固定100%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_3, 0U);
        break; 
    case CALIBRATION_A:
        //B侧固定80%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_3, HRTIM_PERIOD * 0.20f);
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_4, HRTIM_PERIOD * 0.5f);
        //A侧固定100%占空比
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_3, 0U);
        break;
    default:
        break;
    }
}

} // namespace HRTIM


namespace ADC
{

void initAnalog()
{
    HAL_OPAMP_SelfCalibrate(&hopamp1);  // 采样电压跟随
    HAL_OPAMP_SelfCalibrate(&hopamp2);  // 采样电压跟随
    HAL_OPAMP_SelfCalibrate(&hopamp3);  // 采样电压跟随
    HAL_OPAMP_SelfCalibrate(&hopamp4);  // 参考电压跟随

    HAL_Delay(50);
    
    HAL_OPAMP_Start(&hopamp1);
    HAL_OPAMP_Start(&hopamp2);
    HAL_OPAMP_Start(&hopamp3);
    HAL_OPAMP_Start(&hopamp4);
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);   
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    HAL_COMP_Start(&hcomp2);
    HAL_COMP_Start(&hcomp3); 
}

void initADC()
{
    //校准ADC，ADC1和ADC2用于同步采样，ADC4用于监测温度和辅助电源
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc4, ADC_SINGLE_ENDED);
    HAL_Delay(50);

    //双ADC同步采样
    HAL_ADCEx_MultiModeStart_DMA(&hadc1, adcData.rawData12, 4 * HRTIM_INT_SCALER);
    HAL_ADC_Start(&hadc2);
    HAL_ADC_Start_DMA(&hadc4, (uint32_t*)adcData.rawData4, ADC4_BUFFER_SIZE);
}

__attribute__((section(".code_in_ram"))) void updateADCmf()
{
    for(uint8_t i = 0; i < HRTIM_INT_SCALER; i++){
        adcData.sumData[0] += adcData.rawData12[i * 4];
        adcData.sumData[1] += adcData.rawData12[i * 4 + 1];
        adcData.sumData[2] += adcData.rawData12[i * 4 + 2];
        adcData.sumData[3] += adcData.rawData12[i * 4 + 3];
    }

#ifdef CALIBRATION_MODE
    //for(uint8_t i = 0; i < 5; i++)
        //adcData.tempData[i] = adcData.tempData[i] * 0.99f + adcData.sumData[i] * 0.01f;
    adcData.tempData[0] = adcData.tempData[0] * (1-ADC_CALI_ALPHA) 
                    + (uint16_t)adcData.sumData[0] * ADC_CALI_ALPHA;
    adcData.tempData[1] = adcData.tempData[1] * (1-ADC_CALI_ALPHA) 
                    + (uint16_t)adcData.sumData[1] * ADC_CALI_ALPHA;
    adcData.tempData[2] = adcData.tempData[2] * (1-ADC_CALI_ALPHA) 
                    + (uint16_t)adcData.sumData[2] * ADC_CALI_ALPHA;
    adcData.tempData[3] = adcData.tempData[3] * (1-ADC_CALI_ALPHA) 
                    + *(uint16_t *)((uint8_t *)&adcData.sumData[0] + 2) * ADC_CALI_ALPHA;
    adcData.tempData[4] = adcData.tempData[4] * (1-ADC_CALI_ALPHA) 
                    + *(uint16_t *)((uint8_t *)&adcData.sumData[2] + 2) * ADC_CALI_ALPHA;
    
    adcData.tempData[5] = adcData.tempData[5] * (1-ADC_CALI_ALPHA) 
                    + *(uint16_t *)((uint8_t *)&adcData.sumData[3] + 2) * ADC_CALI_ALPHA;
    adcData.tempData[6] = adcData.tempData[6] * (1-ADC_CALI_ALPHA)
                    + (uint16_t)adcData.sumData[3] * ADC_CALI_ALPHA;

    //iA iR vA iB vB iWPT vWPT  

#endif
    //  ADC1   iA  iR  vA   vWPT
    //  ADC2   iB  iB  vB   iWPT

    adcData.iA = (1-ADC_ISENSE_ALPHA) * adcData.iA + 
            ADC_ISENSE_ALPHA * ((uint16_t)adcData.sumData[0] * ADC_IA_K + ADC_IA_B);
    adcData.iR = (1-ADC_ISENSE_ALPHA) * adcData.iR - 
            ADC_ISENSE_ALPHA * ((uint16_t)adcData.sumData[1] * ADC_IREF_K + ADC_IREF_B);
    adcData.vA = (1-ADC_VSENSE_ALPHA) * adcData.vA +
            ADC_VSENSE_ALPHA * (uint16_t)adcData.sumData[2] * ADC_VA_K + ADC_VA_B;
    adcData.iB = (1-ADC_ISENSE_ALPHA) * adcData.iB + 
            ADC_ISENSE_ALPHA * (*(uint16_t *)((uint8_t *)&adcData.sumData[0] + 2) * ADC_IB_K + ADC_IB_B);
    adcData.vB =  (1-ADC_VSENSE_ALPHA) * adcData.vB +
            ADC_VSENSE_ALPHA * *(uint16_t *)((uint8_t *)&adcData.sumData[2] + 2) * ADC_VB_K + ADC_VB_B;

    

#ifdef WPT_HARDWARE
    adcData.vWPT = (uint16_t)adcData.sumData[3] * ADC_VWPT_K + ADC_VWPT_B;
    adcData.iWPT = (1-ADC_ISENSE_ALPHA) * adcData.iWPT + 
            ADC_ISENSE_ALPHA * (*(uint16_t *)((uint8_t *)&adcData.sumData[3] + 2) * ADC_IWPT_K + ADC_IWPT_B); 

    adcData.pWPT = adcData.vB * adcData.iWPT;
    
    adcData.vWPTlf = MF_TO_LF_ALPHA * adcData.vWPT + 
            (1.0f - MF_TO_LF_ALPHA) * adcData.vWPTlf;

    adcData.pWPTlf = MF_TO_LF_ALPHA * adcData.pWPT + 
            (1.0f - MF_TO_LF_ALPHA) * adcData.pWPTlf;
    adcData.iCap = adcData.iB + adcData.iWPT;
#else
    adcData.iCap = adcData.iB;
    adcData.iWPT = 0.0f;
    adcData.vWPT = 0.0f;
#endif

    adcData.vCap = adcData.vB - adcData.iCap * CAPARR_DCR;
    adcData.iChassis = adcData.iR - adcData.iA;
    adcData.pReferee = adcData.vA * adcData.iR;
    adcData.pChassis = adcData.vA * adcData.iChassis;

    // 对62.5kHz的采样数据进行一阶滤波，截止频率1.5kHz
    adcData.iCaplf = MF_TO_LF_ALPHA * adcData.iCap + 
            (1.0f - MF_TO_LF_ALPHA) * adcData.iCaplf;
    adcData.vCaplf = MF_TO_LF_ALPHA * adcData.vCap + 
            (1.0f - MF_TO_LF_ALPHA) * adcData.vCaplf;
    adcData.pChassislf = MF_TO_LF_ALPHA * adcData.pChassis + 
            (1.0f - MF_TO_LF_ALPHA) * adcData.pChassislf;
    adcData.pRefereelf = MF_TO_LF_ALPHA * adcData.pReferee + 
            (1.0f - MF_TO_LF_ALPHA) * adcData.pRefereelf;
    

    for(uint8_t j = 0; j < 4; j++)
       adcData.sumData[j] = 0;

}


void updateADClf()
{
    adcData.vAux = adcData.rawData4[1] * (2.9f/4096.0f);
    HAL_ADC_Start_DMA(&hadc4, (uint32_t*)adcData.rawData4, ADC4_BUFFER_SIZE);

    // if(psData.outputABEnabled && (adcData.vAux > 1.5f || adcData.vAux < 1.3f))
    // {
    //     Buzzer::play(6400, 20);
    // }
}

}// namespace ADC

namespace PowerControl
{

void powerOnOffControl()
{
    if(psData.outputABEnabled)
    {
        if(adcData.vA < REFEREE_UVLO_LIMIT)
        {
            HRTIM::disableOutputAB();
            psData.softStartCnt = SOFT_START_TIME;
        }
        if(psData.softStartCnt > 0)
        {   
            psData.softStartCnt--;
        }

        
    }
    else
    {
        if(adcData.vA > REFEREE_UVLO_RECOVERY && (errorData.errorLevel == NO_ERROR || errorData.errorLevel == WARNING) && psData.allowEnableOutput)//TODO
        {
            //psData.iLLimit = 1.0f;
            HRTIM::enableOutputAB();
             
        }
        psData.softStartCnt = SOFT_START_TIME;
            
    }
    
}
//VCAP/VA=84/x
__attribute__((section(".code_in_ram"))) inline void setInductorCurrent()
{
    HAL_DACEx_SawtoothWaveGenerate(&hdac1, DAC_CHANNEL_2, DAC_SAWTOOTH_POLARITY_INCREMENT, 
        PEAKI_TO_DACVAL((psData.iLTarget - 1.25f)), 180);
    HAL_DACEx_SawtoothWaveGenerate(&hdac1, DAC_CHANNEL_1, DAC_SAWTOOTH_POLARITY_INCREMENT, 
        PEAKI_TO_DACVAL(-(psData.iLTarget + 1.25f)), 180);
}

__attribute__((section(".code_in_ram"))) void updateMFLoop()
{
    // 计算B侧电流限制
    CAPARR::updateMaxCurrent();
    

    if(adcData.vCap > ctrlData.vCapArrNormal + 0.1f)
    {
        ctrlData.allowCharge = false;
    }
    else if(adcData.vCap < ctrlData.vCapArrNormal - 0.1f)
    {
        ctrlData.allowCharge = true;
    }


    if((ctrlData.allowCharge || !rxData1.enableActiveChargingLimit) && !psData.softStartCnt)//
    {
        mfLoop.iRPID.computeDelta((ctrlData.pRefereeTarget / adcData.vA), adcData.iR);
        ctrlData.limitFactor = REFEREE_POWER;
    }
    else
    {
        mfLoop.iRPID.computeDelta(6.0f / adcData.vA, adcData.iR);
        ctrlData.limitFactor = CAPARR_VOLTAGE_NORMAL;
    }
        
    

    // 默认设为裁判系统功率PID的输出
    mfLoop.deltaIL = mfLoop.iRPID.getOutput();

    mfLoop.dIL_VCap_Max = mfLoop.voltageLimitKI * (CAPARR_MAX_VOLTAGE - adcData.vCap);
    mfLoop.dIL_IB_Positive = mfLoop.currentLimitKI * (capStatus.maxInCurrent - adcData.iCap);
    mfLoop.dIL_IB_Negative = mfLoop.currentLimitKI * (-adcData.iCap - capStatus.maxOutCurrent);

    //mfLoop.dIL_recoverBurst = M_CLAMP((0.1f - adcData.iChassis)* mfLoop.burstKI, 0.0f, 8.0f);
    
    if((adcData.vCap > CAPARR_MAX_VOLTAGE * 0.95f) && (mfLoop.dIL_VCap_Max < mfLoop.deltaIL))
    {
        mfLoop.deltaIL = mfLoop.dIL_VCap_Max;
        ctrlData.limitFactor = CAPARR_VOLTAGE_MAX;
    }
    else if(adcData.iCap > capStatus.maxInCurrent && mfLoop.dIL_IB_Positive < mfLoop.deltaIL)
    {
        mfLoop.deltaIL = mfLoop.dIL_IB_Positive;
        ctrlData.limitFactor = IB_POSITIVE;
    }
    else if(adcData.iCap < -capStatus.maxOutCurrent && mfLoop.dIL_IB_Negative > mfLoop.deltaIL)
    {
        mfLoop.deltaIL = mfLoop.dIL_IB_Negative;
        ctrlData.limitFactor = IB_NEGATIVE;
    }
    psData.iLTarget += mfLoop.deltaIL; // + mfLoop.dIL_recoverBurst);
    psData.iLTarget = M_CLAMP(psData.iLTarget, -psData.iLLimit, psData.iLLimit);
    
}

void updateRefereePower(const RxData &rd, const uint32_t& currentTick)
{  
    if(ctrlData.limitFactor == REFEREE_POWER && psData.outputABEnabled)
    {
        ctrlData.refLoop.error = (rd.refereeEnergyBuffer - REFEREE_ENERGY_BUFFER);
        ctrlData.refLoop.pRefereeBias = ctrlData.refLoop.kP*ctrlData.refLoop.error + 
            ctrlData.refLoop.kI*ctrlData.refLoop.integral + 
            ctrlData.refLoop.kD*(ctrlData.refLoop.error - ctrlData.refLoop.lastError);
        ctrlData.refLoop.lastError = ctrlData.refLoop.error;
        ctrlData.refLoop.integral += ctrlData.refLoop.error;

        ctrlData.refLoop.pRefereeBias = M_CLAMP(ctrlData.refLoop.pRefereeBias, -REFEREE_POWER_BIAS_LIMIT, REFEREE_POWER_BIAS_LIMIT);
    }
    else
    {
        ctrlData.refLoop.lastError = 0.0f;
        ctrlData.refLoop.integral = 0.0f;
    }

    ctrlData.pRefereeTarget = M_CLAMP(ctrlData.refLoop.pRefereeBias + rd.refereePowerLimit, 5.0f, 135.0f);
    ctrlData.refLoop.lastTimestamp = currentTick;
}

void checkRxDataTimeout(const uint32_t& currentTick)
{
    if(ctrlData.refLoop.isConnected && (currentTick - ctrlData.refLoop.lastTimestamp > RXDATA_TIMEOUT))//
    {
        ctrlData.pRefereeTarget = REFEREE_DEFUALT_POWER;
        ctrlData.refLoop.lastError = 0.0f;
        ctrlData.refLoop.integral = 0.0f;
        ctrlData.refLoop.isConnected = 0;
        ctrlData.vCapArrNormal = CAPARR_MAX_VOLTAGE;

        rxData1.enableDCDC = 1;
        rxData1.systemRestart = 0;
        rxData1.clearError = 0;
        rxData1.enableActiveChargingLimit = 0;
        rxData1.refereePowerLimit = REFEREE_DEFUALT_POWER;

    }
}


} // namespace PowerControl


namespace Protection
{

void errorHandlerLF()
{
    if(errorData.errorLevel == WARNING)
    {
        //逐渐减小errorCnt
        if(errorData.overCurrentCnt > 0)
            errorData.overCurrentCnt--;
        if(errorData.overVoltageCnt > 0)
            errorData.overVoltageCnt--;
        if(errorData.shortCircuitCnt > 0)
            errorData.shortCircuitCnt--;
        //如果Cnt都为0则解除警告状态
        if(!(errorData.overCurrentCnt || errorData.overVoltageCnt || errorData.shortCircuitCnt || errorData.lowBattery))
            errorData.errorLevel = NO_ERROR;
    }

    if(adcData.vA < REFEREE_UVLO_LIMIT)
        errorData.powerOffCnt++;
    else
        errorData.powerOffCnt = 0;
        
    if(errorData.powerOffCnt > 2000 && errorData.errorLevel != NO_ERROR)
    {
        Protection::autoClearError();
        Protection::manualClearError();
    }
}

void checkLowBattery()
{
    // 检测低电压保护
    if(errorData.lowBattery)
    {
        if(adcData.vA > BATTERY_LOW_RECOVERY || adcData.vA < REFEREE_UVLO_LIMIT)
        {
            errorData.lowBattery = 0;
            errorData.errorCode &= ~WARNING_LOWBATTERY;
            if(!errorData.errorCode)
                errorData.errorLevel = NO_ERROR;
        }
    }
    else
    {
        if(adcData.vA < BATTERY_LOW_LIMIT && adcData.vA > REFEREE_UVLO_RECOVERY)
        {
            errorData.lowBatteryCnt++;
            if(errorData.lowBatteryCnt > 1000) // 低电压持续1
            {
                if(errorData.errorLevel == NO_ERROR)
                {
                    errorData.errorLevel = WARNING;
                }
                errorData.lowBattery = 1;
                errorData.errorCode |= WARNING_LOWBATTERY;
                errorData.lowBatteryCnt = 0;
            }
        }
    }
}


__attribute__((section(".code_in_ram"))) void checkShortCircuit()
{
    // 检测短路保护，在ADC解码后立刻调用
    if(adcData.vA <= SCP_VOLTAGE && -adcData.iA >= SCP_CURRENT)
    {
        //裁判系统端或底盘端短路
        errorData.errorLevel = WARNING;
        errorData.shortCircuitCnt +=600;
        if(errorData.shortCircuitCnt > 1700){
            HRTIM::disableOutputAB();
            errorData.errorCode |= ERROR_SCP_A;
            errorData.errorLevel = ERROR_RECOVER_MANUAL;
            errorData.errorVoltage = adcData.vA;
            errorData.errorCurrent = -adcData.iA;
        }
    }
    if(adcData.vB <= SCP_VOLTAGE && adcData.iB >= SCP_CURRENT)
    { 
        //电容端或无线充电端短路（无线充电端短路会通过buck上管短路B端）
        errorData.errorLevel = WARNING;
        errorData.shortCircuitCnt +=300;
        // if(adcData.vB <= 0.08f)
        //     errorData.shortCircuitCnt -= 600;
        // else if(adcData.vB <= 0.2f)
        //     errorData.shortCircuitCnt -= 550;

        if(errorData.shortCircuitCnt > 1700){//
            HRTIM::disableOutputAB();
            errorData.errorCode |= ERROR_SCP_B;
            errorData.errorLevel = ERROR_RECOVER_MANUAL;
            errorData.errorVoltage = psData.softStartCnt;//adcData.vB;
            errorData.errorCurrent = adcData.iB;
        }
    }
}

__attribute__((section(".code_in_ram"))) void checkEfficiency()
{
    if(adcData.iA > 0.5f)
        psData.efficiency = (adcData.vB * adcData.iB) / (adcData.vA * adcData.iA);
    else if(adcData.iA < -0.5f)
        psData.efficiency = (adcData.vA * adcData.iA) / (adcData.vB * adcData.iB);
}

void hrtimFaultHandler() // 过压/过流保护触发
{
    if(HRTIM1->sCommonRegs.ISR & HRTIM_FLAG_FLT1)   //vA过压保护触发
    {
        errorData.errorCode |= ERROR_OVP_A;
        errorData.errorLevel = ERROR_RECOVER_AUTO;
        HRTIM::disableOutputAB();
    }
    if(HRTIM1->sCommonRegs.ISR & HRTIM_FLAG_FLT2)   //iA过流保护触发
    {   
        errorData.errorCode |= ERROR_OCP_A;
        errorData.errorLevel = ERROR_RECOVER_AUTO;
        HRTIM::disableOutputAB();
    }
    if(HRTIM1->sCommonRegs.ISR & HRTIM_FLAG_FLT3)   //iR过流保护触发
    {
        errorData.errorCode |= ERROR_OCP_R;
        errorData.errorLevel = ERROR_RECOVER_AUTO;
        HRTIM::disableOutputAB();
    }
    if(HRTIM1->sCommonRegs.ISR & HRTIM_FLAG_FLT4)   //vB过压保护触发
    {
        errorData.errorCode |= ERROR_OVP_B;
        errorData.errorLevel = ERROR_RECOVER_AUTO;
        HRTIM::disableOutputAB();
    }
    if(HRTIM1->sCommonRegs.ISR & HRTIM_FLAG_FLT5)   //iB过流保护触发
    {
        errorData.errorCode |= ERROR_OCP_B;
        errorData.errorLevel = ERROR_RECOVER_AUTO;
        HRTIM::disableOutputAB();
    }
}

void configAWDG()
{
    //ADC1->TR3 = 0x00E00020;
    ADC1->TR2 = 0x00E00020;
    ADC2->TR2 = 0x00E00020;
    ADC1->TR1 = (uint32_t)(OVP_A/2.9f/16.0f*4095.0f) << 16;
    ADC2->TR1 = (uint32_t)(OVP_B/2.9f/16.0f*4095.0f) << 16;
    HRTIM1->sCommonRegs.IER |= 0b11111;
}

void checkHardwareUID()
{
    // 读取寄存器中的UID
    sysData.hardwareUID[0] = READ_REG(*((uint32_t *)UID_BASE));
    sysData.hardwareUID[1] = READ_REG(*((uint32_t *)(UID_BASE + 4U)));
    sysData.hardwareUID[2] = READ_REG(*((uint32_t *)(UID_BASE + 8U)));

#ifndef CALIBRATION_MODE

    if( (sysData.hardwareUID[0] != HARDWARE_UID_W0) 
        || (sysData.hardwareUID[1] != HARDWARE_UID_W1) 
        || (sysData.hardwareUID[2] != HARDWARE_UID_W2))
    {
        while(1)
        {
            Buzzer::play(200, 100);
            HAL_Delay(1000);
        }
    }

#endif

}

void autoClearError()
{
    if(errorData.errorLevel == ERROR_RECOVER_AUTO)
    {
        errorData.errorCode = 0;
        errorData.overCurrentCnt = 0;
        errorData.overVoltageCnt = 0;
        errorData.errorLevel = NO_ERROR;
        HRTIM::enableOutputAB();
    }
}

void manualClearError()
{
    if(errorData.errorLevel == ERROR_RECOVER_MANUAL)
    {
        errorData.errorCode = 0;
        errorData.shortCircuitCnt = 0;

        errorData.errorLevel = NO_ERROR;
        HRTIM::enableOutputAB();
    }
}

} // namespace Protection


namespace CAPARR
{


__attribute__((section(".code_in_ram"))) void updateMaxCurrent()
{
    if(adcData.vCap > CAPARR_LOW_VOLTAGE)
    {
        capStatus.maxOutCurrent = CAPARR_MAX_CURRENT;
        capStatus.maxInCurrent = CAPARR_MAX_CURRENT;
    }
    else if(adcData.vCap > CAPARR_CUTOFF_VOLTAGE)
    {
        capStatus.maxOutCurrent = (CAPARR_MAX_CURRENT - 1.0f)/(CAPARR_LOW_VOLTAGE - CAPARR_CUTOFF_VOLTAGE) * (adcData.vCap - CAPARR_CUTOFF_VOLTAGE) + 1.0f;
        capStatus.maxInCurrent = capStatus.maxOutCurrent;
    }
    else
    {
        capStatus.maxOutCurrent = 0.2f + (0.8f / 5.0f) * adcData.vCap;
        capStatus.maxInCurrent = 1.0f;
    }
}

__attribute__((section(".code_in_ram"))) inline void updateCurrentforEstimation()
{
    capStatus.capEstData.maxIB = M_MAX(capStatus.capEstData.maxIB, adcData.iB);
    capStatus.capEstData.minIB = M_MIN(capStatus.capEstData.minIB, adcData.iB);
}

uint16_t getMaxPowerFeedback()
{
    if(adcData.vCap > CAPARR_LOW_VOLTAGE)
        return (uint16_t)(CM01_CURRENT_LIMIT * adcData.vCaplf);
    else if(adcData.vCap > CAPARR_CUTOFF_VOLTAGE)
        return (uint16_t)((CM01_CURRENT_LIMIT - 1.0f)/(CAPARR_LOW_VOLTAGE - CAPARR_CUTOFF_VOLTAGE) * (adcData.vCaplf - CAPARR_CUTOFF_VOLTAGE) + 1.0f) * adcData.vCaplf;
    else
        return (uint16_t)(0.2f + (0.8f / 5.0f) * adcData.vCaplf) * adcData.vCaplf;
}


void restartEstimation(const uint32_t& _currentTick)
{
    capStatus.capEstData.dQ = 0.0f;
    capStatus.capEstData.lastVCap = adcData.vCaplf;
    capStatus.capEstData.maxIB = adcData.iB;
    capStatus.capEstData.minIB = adcData.iB;
    capStatus.capEstData.lastTick = _currentTick;
}

void estimateCapacity(const uint32_t& _currentTick)
{
    capStatus.capEstData.dQ += adcData.iCaplf;
    
    if(M_ABS(adcData.vCaplf - capStatus.capEstData.lastVCap) > 0.7f) // 电压变化超过阈值
    {
        if(M_ABS(capStatus.capEstData.maxIB - capStatus.capEstData.minIB) < 4.5f)
        {
            capStatus.capEstData.dQtodV = capStatus.capEstData.dQ * (1.0f/1000.0f) / (adcData.vCaplf - capStatus.capEstData.lastVCap);
            
            if(capStatus.capEstData.dQtodV > CAPARR_CAPACITY_HT || capStatus.capEstData.dQtodV < CAPARR_CAPACITY_LT)
                capStatus.warningCnt += 9;
            else if(capStatus.warningCnt > 0)
                capStatus.warningCnt--;
        }
        restartEstimation(_currentTick);
    }
    else if(M_ABS(capStatus.capEstData.dQ) > 600.0f) // 累计电荷变化超过阈值
    {
        if(M_ABS(capStatus.capEstData.maxIB - capStatus.capEstData.minIB) < 4.5f)
        {
            capStatus.capEstData.dVtodQ = (adcData.vCaplf - capStatus.capEstData.lastVCap) / (capStatus.capEstData.dQ * (1.0f/1000.0f));
            
            if(capStatus.capEstData.dVtodQ < (1.0f/CAPARR_CAPACITY_HT) || capStatus.capEstData.dVtodQ > (1.0f/CAPARR_CAPACITY_LT))
                capStatus.warningCnt += 4;
            else if(capStatus.warningCnt > 0)
                capStatus.warningCnt--;
        }
        restartEstimation(_currentTick);
    }
    else if(_currentTick - capStatus.capEstData.lastTick > 1000) //<这个timeout控制触发阈值，现在0.4A漏电稳定触发，0.3A稳定不触发
    {
        if(capStatus.warningCnt > 0)
            capStatus.warningCnt--;
        restartEstimation(_currentTick);    // 如果过了timeout，重新开始估算
    }

    if(capStatus.warningCnt > 15)
    {
        capStatus.warningCnt = 0;
        Buzzer::play(2000, 20);
    }

}



} // namespace CAPARR


extern "C"
{
    __attribute__((section(".code_in_ram"))) void HRTIM1_Master_IRQHandler(void)  // 136kHz/8 sample
    {
        __HAL_HRTIM_MASTER_CLEAR_IT(&hhrtim1, HRTIM_MASTER_IT_MREP);
        //GPIOB->BSRR = (uint32_t)GPIO_PIN_5;
        
        // 清零中断负载检测Timer
        __HAL_TIM_SET_COUNTER(&htim16, 0);

        // 计算ADC采样值
        ADC::updateADCmf();
        
        // 更新BuckBoost功率级模式
        HRTIM::modeStateMachine();
        
        if(psData.outputABEnabled)
        {
            Protection::checkShortCircuit();
    
            PowerControl::updateMFLoop();
            
            PowerControl::setInductorCurrent();

            Protection::checkEfficiency();

            CAPARR::updateCurrentforEstimation();
            
        }
        else
        {
            // if(adcData.vB < 1.0f)
            //     psData.iLTarget = -1.38f;
            // else
            //    psData.iLTarget = 0.0f;

            psData.iLTarget = -2.0f;

            mfLoop.deltaIL = 0.0f;
            mfLoop.iRPID.resetError();
        }
        
        #ifdef WPT_HARDWARE

        
        if(psData.outputEEnabled)
        {   
            psData.dutyE += mfLoop.wptVoltageKI * (adcData.vWPT - 29.5f);
            psData.dutyEMin = adcData.vB * (1/VWPT_LIMIT_BY_DUTY);
            psData.dutyE = M_CLAMP(psData.dutyE, psData.dutyEMin, 0.99f);
            
            
            if(psData.dutyE < 0.96f)
            {
                timerE_Duty_DMA_Buffer[0] = (HRTIM_PERIOD * psData.dutyE);
                timerE_Duty_DMA_Buffer[1] = (HRTIM_PERIOD * psData.dutyE);
                timerE_Duty_DMA_Buffer[2] = (HRTIM_PERIOD * psData.dutyE);
                timerE_Duty_DMA_Buffer[3] = (HRTIM_PERIOD * psData.dutyE);
            }
            else if(psData.dutyE < 0.98f)
            {
                timerE_Duty_DMA_Buffer[0] = HRTIM_PERIOD * (2.0f * psData.dutyE - 1.0f);
                timerE_Duty_DMA_Buffer[1] = HRTIM_PERIOD;
                timerE_Duty_DMA_Buffer[2] = HRTIM_PERIOD * (2.0f * psData.dutyE - 1.0f);
                timerE_Duty_DMA_Buffer[3] = HRTIM_PERIOD;
            }
            else
            {
                timerE_Duty_DMA_Buffer[0] = HRTIM_PERIOD * (4.0f * psData.dutyE - 3.0f);
                timerE_Duty_DMA_Buffer[1] = HRTIM_PERIOD;
                timerE_Duty_DMA_Buffer[2] = HRTIM_PERIOD;
                timerE_Duty_DMA_Buffer[3] = HRTIM_PERIOD;
            }
            
            
            //__HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_E, HRTIM_COMPAREUNIT_1, (HRTIM_PERIOD * psData.dutyE));

            // if(adcData.vWPT > 29.9f || adcData.pWPTlf > 145.0f)
            // {
            //     askData.enableASK = 0;
            //     Buzzer::play(NOTE_WARNING_HIGH_FREQ, 20);
            // }
        }
        else
        {
            psData.dutyE = 0.97f;
            //__HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_E, HRTIM_COMPAREUNIT_1, (HRTIM_PERIOD * psData.dutyE));
            timerE_Duty_DMA_Buffer[0] = (HRTIM_PERIOD * psData.dutyE);
            timerE_Duty_DMA_Buffer[1] = (HRTIM_PERIOD * psData.dutyE);
            timerE_Duty_DMA_Buffer[2] = (HRTIM_PERIOD * psData.dutyE);
            timerE_Duty_DMA_Buffer[3] = (HRTIM_PERIOD * psData.dutyE);
            
        }
        #endif

        

        psData.IRQload = __HAL_TIM_GET_COUNTER(&htim16) *(1.0f/2720.0f);

        //GPIOB->BRR = (uint32_t)GPIO_PIN_5;
    }

    void HRTIM1_FLT_IRQHandler(void)
    {
        Protection::hrtimFaultHandler();
        HAL_HRTIM_IRQHandler(&hhrtim1,HRTIM_TIMERINDEX_COMMON);
    }
}  // extern "C"


//(uint32_t)&(HRTIM1->HRTIM_TIMERx[HRTIM_TIMERINDEX_TIMER_E].CMP1xR)