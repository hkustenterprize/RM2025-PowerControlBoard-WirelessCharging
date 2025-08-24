#include "Utility.hpp"


void IncreasementPID::setParameter(float _kTargetP, float _kMeasureP, float _kI, float _kD)
{
    kTP = _kTargetP;
    kMP = _kMeasureP;
    kI = _kI;
    kD = _kD;
}
void IncreasementPID::setClamp(float _lower, float _upper)
{
    clamp_lower = _lower;
    clamp_upper = _upper;
    clamp_enabled = true;
}
void IncreasementPID::disableClamp()
{
    clamp_enabled = false;
}

void IncreasementPID::computeDelta(float _target, float _current)
{
    this->deltaOutput = kTP * (_target - this->t1) + kMP * (_current - this->m1) 
        + kI * (_target - _current) + kD * ((_target - _current) - 2 * (this->t1 - this->m1) + this->e2);
    
    this->e2 = this->t1 - this->m1;
    this->t1 = _target;
    this->m1 = _current;

    if(clamp_enabled)
        this->deltaOutput = M_CLAMP(this->deltaOutput, clamp_lower, clamp_upper);

}
void IncreasementPID::resetError()
{
    this->deltaOutput = 0.0f;
    this->t1 = 0.0f;
    this->m1 = 0.0f;
    this->e2 = 0.0f;
}

float IncreasementPID::getOutput()
{
    return this->deltaOutput;
}
    
    


