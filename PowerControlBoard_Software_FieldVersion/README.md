# RM2025-PowerControlBoard

## 通讯格式

### 主控板>电容

~~~
struct RxData {
    uint8_t enableDCDC: 1;
    uint8_t systemRestart: 1;
    uint8_t resv0: 3;
    uint8_t clearError: 1;
    uint8_t enableActiveChargingLimit: 1;
    uint8_t useNewFeedbackMessage: 1;

    uint16_t refereePowerLimit;
    uint16_t refereeEnergyBuffer;
    uint8_t activeChargingLimitRatio; 
    int16_t resv2;
} __attribute__((packed));
~~~

| 变量名 | 功能 | 详细描述 |
| -- | -- | -- |
| enableDCDC | 允许启动DCDC | 如果为0：立即关闭DCDC，并且不主动重启 |
| systemRestart | 系统重启 | 触发 `NVIC_SystemReset();` |
| clearError | 清除故障 | 可清除 `ERROR_RECOVER_MANUAL` (短路保护或电容组故障)、`ERROR_RECOVER_AUTO` (过流或过压，电容组本身也会自动尝试恢复) 级别的错误；但是不可清除 `ERROR_UNRECOVERABLE` (功率级故障) 级别的错误。 <br> 建议主控板只对 `ERROR_RECOVER_MANUAL` 级别的错误进行处理，且触发方式为操作手手动|
| enableActiveChargingLimit | 启用主动充电限制 | 开启后当电容组能量达到设定值，将不会再对电容组进行主动充电（即不会从裁判系统获取电量，但可以通过能量回收等方式充电直到最大电压），此时裁判系统功率的闭环为一个略小于底盘供电网络静态功耗的值 <br> 开启关闭有0.2V的施密特触发防止震荡 <br> **使用方式：开始比赛设为1，未开始比赛或开始比赛进入虚弱模式后设为0** |
| useNewFeedbackMessage | 是否使用新的反馈消息格式 | 新旧消息格式只能同时选择一个 <br> 旧消息格式与RM2024一致以保持兼容性(0x051)，新消息(0x052)格式将底盘功率反馈的`float`拆分为底盘功率和裁判系统功率的`uint16_t`，具体见下文 <br> 上电默认反馈格式为旧消息格式，随后将按最后一次收到的的值为准 |
| refereePowerLimit | 裁判系统功率限制 | 单位W <br> 默认主控板发来的功率限制可信，建议在主控板上做好数据保护 |
| refereeEnergyBuffer | 裁判系统缓冲能量 | 单位J <br> 在外环限制为`REFEREE_POWER`时，闭环目标为50J |
| activeChargingLimitRatio | 允许启动DCDC | 主动充电目标比例（能量），范围为0-255，计算方式为 `TargetVCap = sqrtf (activeChargingLimitRatio / 255) * CAPARR_MAX_VOLTAGE`，同时对计算结果进行限制，最低 `CAPARR_LOW_VOLTAGE` (10.0V)|


### 电容>主控板(旧)
~~~
struct TxData {                     // 0x051 (useNewFeedbackMessage = 0)
    uint8_t statusCode;
    float chassisPower;
    uint16_t chassisPowerLimit;
    uint8_t capEnergy;              // 
} __attribute__((packed));
~~~

| 变量名 | 功能 | 详细描述 |
| -- | -- | -- |
| statusCode | 状态信息 | 详情见下一节 “电容>主控板(新)” |
| chassisPower | 底盘功率 | 单位W |
| chassisPowerLimit | 底盘最大可用功率 | 单位W，此功率包括裁判系统，计算方式为：[电容放电最大电流 * 电容电压 + `refereePowerLimit` (以最近收到的值为准)] <br> 考虑到功率级的效率损失，建议加入一定的安全系数（如0.9）。实际功率超过该值可能无法保证裁判系统功率闭环；由于电容控制器实际电流限制为22.5A，反馈该值时计算以CM01的16A限制为准，实际功率长时间超过该值可能导致裁判系统将底盘断电。 |
| capEnergy | 电容现有能量 | 以能量比例为准，计算方式为`(VCap/CAPARR_MAX_VOLTAGE)^2 * 250U`，满电值为250，超过250说明代表电容实际充电超过100% |

### 电容>主控板(新)

~~~
struct TxDataNew {                  // 0x052 (useNewFeedbackMessage = 1)
    uint8_t statusCode;
    uint16_t chassisPower;
    uint16_t refereePower;
    uint16_t chassisPowerLimit;
    uint8_t capEnergy;
} __attribute__((packed));
~~~

由于旧通讯格式下主控板对`errorCode`几乎没有使用，新通讯格式将其改为`statusCode`进行错误与系统状态反馈。

| Bit | 功能 | 详细描述 |
| -- | -- | -- |
|  7 | 功率级状态 | 1为启动，0为未启动（触发保护或主控板禁用）|
| 6 | 反馈信息格式 | 1为新通讯格式，0为旧通讯格式（RM2024） |
| 5:4 | RESV | OFF READY CHARGING FULL |
| 3:2 | 控制外环限制因素 | 稍后补充 |
| 1:0 | 错误等级 | `NO_ERROR = 00` `ERROR_RECOVER_AUTO = 01` `ERROR_RECOVER_MANUAL = 10` `ERROR_UNRECOVERABLE = 11` |


其余变量的定义更改如下，其余变量定义与旧通讯格式相同
| 变量名 | 功能 | 详细描述 |
| -- | -- | -- |
| chassisPower | 底盘功率 | 计算方式为: `pChassis * 64U + 16384U` 量程-256W~+768W, 分辨率0.015625W <br> 此数据在发送前进行了截止频率为1.5kHz的一阶低通滤波 |
| refereePower | 裁判系统功率 | 计算方式为: `pChassis * 64U + 16384U` 量程-256W~+768W, 分辨率0.015625W <br> 此反馈值为电容控制器读直接取到的功率值，可能与裁判系统有一定偏差，可以在外环限制为`REFEREE_POWER`且缓冲能量已经稳定闭环到50J时进行校准（此时裁判系统的功率非常接近于此时的功率限制） <br> 此数据在发送前进行了截止频率为1.5kHz的一阶低通滤波 |

## 峰值电流模式BuckBoost

频率250k，counter 21760

## 内环控制

HRTIM频率250k，counter 21760

### HRTIM

Master Timer
触发TimerA复位 (RST)
触发TimerB复位 (CMP2)
触发ADC采样 (CMP1)

Timer A B
1st RESET: 周期开始 (CMP1)
1st SET: 占空比保护 (CMP3)
2nd: SET: 外部事件触发 (External Event)
触发DAC三角波RST (RST)
触发DAC三角波STEP (CMP4)

### 峰值电流触发链

IA(+) & DAC1_OUT2(-) > COMP3 > HRTIM External Event 8 (low active) > External Event 8 Filtering (TimerB CMP4) > SET
IB(+) & DAC1_OUT1(-) > COMP2 > HRTIM External Event 6 (low active) > External Event 6 Filtering (TimerA CMP4) > SET


## 外环控制

### 控制逻辑

每次计算外环得出一个目标 deltaiLTarget

首先设为裁判系统功率PID输出

然后进行其他限制(优先级从高到低)
//已经过时
1. 电容最高电压 | IL上限
2. 反向IR为0 (动能回收) | IL下限
3. VA电压上限 (动能回收) | IL下限
4. 电容80%电压 | IL上限
5. 电容输入电流IB | IL上限
6. 电容输出电流IB | IL下限

## ADC与保护

使用ADC Watchdog硬件触发Timer输出关断和软件中断

注意：AWDG1和AWDG2/3不同，AWDG1精度为12位，AWGD2/3只能监测高8位，所以对于vA和vB使用AWDG1，对于电流使用AWDG2/3

iA > ADC1_Channel9 > ADC1_AWDG2 > HRTIM1_ExternalEvent2 > HRTIM1_FaultLine2 
iR > ADC1_Channel8 > ADC1_AWDG3 > HRTIM1_ExternalEvent3 > HRTIM1_FaultLine3
vA > ADC1_Channel12 > ADC1_AWDG1 > HRTIM1_ExternalEvent1 > HRTIM1_FaultLine1 
iB > ADC2_Channel5 > ADC2_AWDG2 > HRTIM1_ExternalEvent5 > HRTIM1_FaultLine5
vB > ADC2_Channel3 > ADC2_AWDG1 > HRTIM1_ExternalEvent4 > HRTIM1_FaultLine4

中断使用HRTIM的IER寄存器启用

中断后通过ISR寄存器判断中断来源

中断是否直接通过FaultLine禁用TimerA/TimerB输出可以通过HRTIM_FLTxR寄存器实现


## ASK数据格式

ASK通讯等效2000波特率

周期20 发送频率100Hz

单个数据包长12bit，开始停止各1bit，校验1bit，数据9bit

数据位：

需求功率5W或80W：1bit
功率反馈：8bit，量程100W
