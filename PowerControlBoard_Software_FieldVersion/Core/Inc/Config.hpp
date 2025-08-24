#pragma once




/*-------- HARDWARE CONFIG --------*/
#define HW_VSENSE_RATIO     16.0f   // 33k:2.2k
#define HW_ISENSE_RATIO     25.0f   // INA240A1 1/(20*0.002)
#define ADC_VREF            2.9f   // VREFBUF

#define ADC_VSENSE_RES      ADC_VREF * HW_VSENSE_RATIO / 4096.0f
#define ADC_ISENSE_RES      ADC_VREF * HW_ISENSE_RATIO / 4096.0f
#define ADC_ISENSE_OFFSET   ADC_VREF * HW_ISENSE_RATIO / 2.0f


#define HW_RSENSE           0.002f
#define HW_IAMP_GAIN        20.0f





#define ADC_ISENSE_ALPHA    0.8f
#define ADC_VSENSE_ALPHA    0.8f
#define ADC_CALI_ALPHA      0.001f
#define MF_TO_LF_ALPHA      0.092f


#define HRTIM_PERIOD        21760U

#define VB_LIMIT_BY_DUTY        29.8f
#define VWPT_LIMIT_BY_DUTY      30.8f

/*-------- PROTECTION --------*/
// 过压保护阈值
#define OVP_A                   29.0f
#define OVP_B                   30.5f

// 过流保护阈值
#define OCP_CAPARR              25.5f
#define OCP_CHASSIS             20.0f
#define OCP_REFEREE             6.5f
// // Over Temperature Protection
// #define OTP_LIMIT               80.0f
// #define OTP_RECOVERY            70.0f
// 短路保护阈值
#define SCP_VOLTAGE             5.0f
#define SCP_CURRENT             5.0f
#define SCP_RECOVER_TIME        1000
// 裁判系统欠压关断
#define REFEREE_UVLO_LIMIT      18.0f //狗腿特殊阈值15V，正常阈值18V
#define REFEREE_UVLO_RECOVERY   20.0f
// Low Efficiency Protection
#define LOW_EFFICIENCY_RATIO    0.75f

#define BATTERY_LOW_LIMIT       20.92f
#define BATTERY_LOW_RECOVERY    21.6f

#define MAX_INDUCTOR_CURRENT    25.0f
#define SOFT_START_TIME         8


/*-------- DEFUALT --------*/
#define REFEREE_DEFUALT_POWER   37.0f
#define REFEREE_ENERGY_BUFFER   57U
#define REFEREE_POWER_BIAS_LIMIT    15.0f
#define REFEREE_POWER_BIAS_WARNING  10.0f
#define RXDATA_TIMEOUT          500U

/*-------- SuperCapacitor Array --------*/
// 电容组异常保护阈值
#define CAPARR_DEFUALT_CAPACITY 4.4f
#define CAPARR_CAPACITY_HT      10.0f
#define CAPARR_CAPACITY_LT      0.2f
// 电容组内阻补偿
#define CAPARR_DCR              0.1f
// 电容组低电量限流
#define CAPARR_CUTOFF_VOLTAGE   5.0f
#define CAPARR_LOW_VOLTAGE      10.0f
#define CAPARR_MAX_VOLTAGE      28.8f//
#define CAPARR_MAX_CURRENT      15.0f
#define CM01_CURRENT_LIMIT      15.0f


// ERROR_UNRECOVERABLE
#define ERROR_POWERSTAGE        0b0000000000000001
#define ERROR_CAPARR            0b0000000000000010
// ERROR_RECOVER_MANUAL
#define ERROR_SCP_A             0b0000000000000100
#define ERROR_SCP_B             0b0000000000001000
// ERROR_RECOVER_AUTO
#define ERROR_OCP_A             0b0000000000010000
#define ERROR_OCP_B             0b0000000000100000
#define ERROR_OCP_R             0b0000000001000000
#define ERROR_OVP_A             0b0000000010000000
#define ERROR_OVP_B             0b0000000100000000
// WARNING
#define WARNING_LOWBATTERY      0b0000001000000000
#define REFEREE_INACCURATE      0b0000010000000000




#define WPT_CUTOFF_VOLTAGE      29.4f
