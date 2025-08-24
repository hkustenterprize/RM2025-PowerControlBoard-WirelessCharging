#pragma once

#include "main.h"
#include "tim.h"
#include "stdint.h"
#include "dma.h"
#include "Config.hpp"
#include "PowerManager.hpp"

#define LED_NUM 3
#define BIT1_WIDTH 137
#define BIT0_WIDTH 69
#define DMA_TX_BUFFER_LENGTH 500

#define WS2812_TIM htim2
#define WS2812_TIM_CHANNEL TIM_CHANNEL_3

#define BUZZER_TIM htim1
#define BUZZER_TIM_CHANNEL TIM_CHANNEL_2

#define COLOR_YELLOW    0xFFFF00
#define COLOR_GREEN     0x00FF00
#define COLOR_BLUE      0x0000FF
#define COLOR_RED       0xFF0000
#define COLOR_WHITE     0xFFFFFF
#define COLOR_BLANK     0x000000

#define NOTE_WARNING_HIGH_FREQ      1600
#define NOTE_WARNING_LOW_FREQ       400
#define NOTE_WARNING_HIGH_DURATION  100
#define NOTE_WARNING_LOW_DURATION   250
#define WARNING_PERIOD              5000

struct BuzzerNote
{
    uint16_t startTime;
    uint16_t freq;
    uint16_t duration;
};

const BuzzerNote buzzerWS_Unrecoverable[10] = 
{   {0,     NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {200,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {400,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {600,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {800,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {1300,  NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_LOW_DURATION},
    {WARNING_PERIOD,  0U, 0U}};
const BuzzerNote buzzerWS_SCPA[10] = 
{   {0,     NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {200,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {400,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {600,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {1100,  NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_LOW_DURATION},
    {WARNING_PERIOD,  0U, 0U}};

const BuzzerNote buzzerWS_SCPB[10] = 
{   {0,     NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {200,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {400,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {600,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {1100,  NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_LOW_DURATION},
    {1600,  NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_LOW_DURATION},
    {WARNING_PERIOD,  0U, 0U}};

const BuzzerNote buzzerWS_OCPA[10] = 
{   {0,     NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {200,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {400,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {900,   NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_HIGH_DURATION},
    {WARNING_PERIOD,  0U, 0U}};

const BuzzerNote buzzerWS_OCPB[10] = 
{   {0,     NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {200,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {400,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {900,   NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_HIGH_DURATION},
    {1100,  NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_HIGH_DURATION},
    {WARNING_PERIOD,  0U, 0U}};

const BuzzerNote buzzerWS_OCPR[10] = 
{   {0,     NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {200,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {400,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {900,   NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_HIGH_DURATION},
    {1100,  NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_HIGH_DURATION},
    {1300,  NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_HIGH_DURATION},
    {WARNING_PERIOD,  0U, 0U}};

const BuzzerNote buzzerWS_OVPA[10] = 
{   {0,     NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {200,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {400,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {900,   NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_LOW_DURATION},
    {WARNING_PERIOD,  0U, 0U}};

const BuzzerNote buzzerWS_OVPB[10] = 
{   {0,     NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {200,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {400,   NOTE_WARNING_HIGH_FREQ, NOTE_WARNING_HIGH_DURATION},
    {900,   NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_LOW_DURATION},
    {1400,  NOTE_WARNING_LOW_FREQ,  NOTE_WARNING_LOW_DURATION},
    {WARNING_PERIOD,  0U, 0U}};

const BuzzerNote buzzerWS_LowBattery[10] = 
{   {0,     1600, 40},
    {40,    800, 40},
    {80,    1600, 40},
    {120,    800, 40},
    {WARNING_PERIOD,  0U, 0U}};



struct InterfaceStatus
{
    uint8_t isWarning = 0U;
    uint8_t isWarningLast = 0U;
    uint16_t buzzerSequenceCnt = 0U;
    
    BuzzerNote buzzerNote[10];
    uint8_t buzzerNoteLength = 0U;
    uint8_t noteIndex = 0U;
};


extern InterfaceStatus interfaceStatus;

namespace Buzzer
{
    void init();
    void play(uint16_t freq, uint16_t duration);
    void update();
} // namespace Buzzer

typedef struct
{
    uint8_t blue;
    uint8_t red;
    uint8_t green;
} RGB;



namespace WS2812
{

void init();

void blink(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

void blink(uint8_t index, uint32_t colorCode);

void update();

void blank(int index);

void blankAll();

}





namespace Interface
{

void alert();
void updateButtonState();
void updateBuzzerSequence();
    
} // namespace Interface