#pragma once

#include "main.h"
#include "stdint.h"
#include "Config.hpp"



#define M_MIN(a, b) ((a) < (b) ? (a) : (b))
#define M_MAX(a, b) ((a) > (b) ? (a) : (b))

#define M_ABS(x) ((x) < 0 ? -(x) : (x))

#define M_CLAMP(x, min, max) (M_MIN((max), M_MAX((min), (x))))

#define PEAKI_TO_DACVAL(x) ((x) * (HW_RSENSE * HW_IAMP_GAIN * 4096.0f / ADC_VREF) + 2048U) 
// dac = (pc * 0.002 * 20 + 1.45) * 4096 / 2.9
#define SLOPE_TO_DACVAL(x) ((x) * (HW_RSENSE * HW_IAMP_GAIN * 4096.0f / ADC_VREF))


//照着去年写的，不是正常的增量式PID
struct IncreasementPID
{
public:
    float kTP = 0.0f, kMP = 0.0f, kI = 0.0f, kD = 0.0f;

    float t1 = 0.0f;    //上次的target
    float m1 = 0.0f;    //上次的measure
    float e2 = 0.0f;    //上上次的误差

    float deltaOutput = 0.0f;   //输出增量

    float clamp_upper = 0.0f;
    float clamp_lower = 0.0f;
    bool clamp_enabled = false;

    IncreasementPID(float _kTargetP, float _kMeasureP, float _kI, float _kD): 
        kTP(_kTargetP), kMP(_kMeasureP), kI(_kI), kD(_kD){}
    void setParameter(float _kTargetP, float _kMeasureP, float _kI, float _kD);
    
    void computeDelta(float _target, float _current);
    void resetError();
    float getOutput();
    
    void setClamp(float _lower, float _upper);
    void disableClamp();
};