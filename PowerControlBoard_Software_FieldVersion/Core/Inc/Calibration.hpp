#pragma once
#include "main.h"

#define HARDWARE_ID 501

#ifndef HARDWARE_ID
#error "please specify hardware id"
#endif


//#define WPT_HARDWARE
//#define CALIBRATION_MODE


#ifdef CALIBRATION_MODE

#define HARDWARE_UID_W0 0x00000000
#define HARDWARE_UID_W1 0x00000000
#define HARDWARE_UID_W2 0x00000000

#define ADC_VA_K        0.00284025493302185f
#define ADC_VA_B        0.096382087f
#define ADC_VB_K        0.00283064245539459f
#define ADC_VB_B        0.096382087f

#define ADC_IA_K        -0.00426032707865977f
#define ADC_IA_B        34.6220566648572f
#define ADC_IB_K        0.00436961348441836f
#define ADC_IB_B        -35.442372697575f

#define ADC_IREF_K      0.00438520650402692f
#define ADC_IREF_B      -35.6851326479174f

#define WPT_HARDWARE
#define ADC_VWPT_K      0.00282862236057022f
#define ADC_VWPT_B      0.126888445762173f
#define ADC_IWPT_K      0.00421074805006724f
#define ADC_IWPT_B      -34.2917170449864f



#elif (HARDWARE_ID == 501) // 示例

#define HARDWARE_UID_W0 0x00000000
#define HARDWARE_UID_W1 0x00000000
#define HARDWARE_UID_W2 0x00000000

#define ADC_VA_K        0.00284025493302185f
#define ADC_VA_B        0.096382087f
#define ADC_VB_K        0.00283064245539459f
#define ADC_VB_B        0.096382087f

#define ADC_IA_K        -0.00426032707865977f
#define ADC_IA_B        34.6220566648572f
#define ADC_IB_K        0.00436961348441836f
#define ADC_IB_B        -35.442372697575f

#define ADC_IREF_K      0.00438520650402692f
#define ADC_IREF_B      -35.6851326479174f


#endif

