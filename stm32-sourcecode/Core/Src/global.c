/*
 * global.c
 *
 *  Created on: Nov 20, 2025
 *      Author: nguye
 */

#ifndef SRC_GLOBAL_C_
#define SRC_GLOBAL_C_

#include "global.h"

/* ---------------------------------------------------------
 *  Hardware handles (defined here, used everywhere)
 * --------------------------------------------------------- */
Keypad_HandleTypeDef hKeypad;
I2C_LCD_HandleTypeDef lcd1;

/* ---------------------------------------------------------
 *  Keypad flags
 * --------------------------------------------------------- */
uint8_t keypad_char_ready = 0;
char    keypad_last_char = 0;
char    keypad_preview_char = 0;

/* ---------------------------------------------------------
 *  Input state
 * --------------------------------------------------------- */
InputState_t gInputState = {
    .latestKey    = 0,
    .doorSensor   = 0,
    .keySensor    = 0,
    .indoorButton = 0,
    .batteryADC   = 0,
    .batteryLow   = 0
};

/* ---------------------------------------------------------
 *  Key Event (single-slot mailbox)
 * --------------------------------------------------------- */
KeyEvent_t gKeyEvent = {
    .keyChar = 0,
    .isLong  = 0
};

/* ---------------------------------------------------------
 *  Output state
 * --------------------------------------------------------- */
OutputStatus_t gOutputStatus = {
    .ledGreen = LED_OFF,
    .ledRed   = LED_OFF,
    .solenoid = SOLENOID_LOCKED,
    .buzzer   = BUZZER_OFF,
    .lcdLine1 = "                ",
    .lcdLine2 = "                "
};

/* ---------------------------------------------------------
 *  System timers
 * --------------------------------------------------------- */
SystemTimers_t gSystemTimers = {
    .failedAttempts   = 0,
    .penaltyLevel     = 0,
    .penaltyEndTick   = 0,
    .unlockExpireTick = 0,
    .alarmRepeatTick  = 0
};

/* ---------------------------------------------------------
 *  System state
 * --------------------------------------------------------- */
SystemState_t gSystemState = {
    .currentState = INIT
};

/* ---------------------------------------------------------
 *  Password storage
 * --------------------------------------------------------- */
char gPassword[MAX_PASSWORD_LENGTH + 1] = "123A"; // default or loaded later

/* ---------------------------------------------------------
 *  Scheduler / timer arrays (from timer.c)
 * --------------------------------------------------------- */
int timer_counter[NUM_TASKS];
int timer_flag[NUM_TASKS];
int TIMER_CYCLE = 10;   // default tick = 10ms

/* ---------------------------------------------------------
 *  Initialization helper
 * --------------------------------------------------------- */
void init_global_variables(void)
{
    keypad_char_ready  = 0;
    keypad_last_char   = 0;
    keypad_preview_char = 0;

    gInputState.latestKey    = 0;
    gInputState.doorSensor   = 0;
    gInputState.keySensor    = 0;
    gInputState.indoorButton = 0;
    gInputState.batteryADC   = 0;
    gInputState.batteryLow   = 0;

    gKeyEvent.keyChar = 0;
    gKeyEvent.isLong  = 0;

    gOutputStatus.ledGreen = LED_OFF;
    gOutputStatus.ledRed   = LED_OFF;
    gOutputStatus.solenoid = SOLENOID_LOCKED;
    gOutputStatus.buzzer   = BUZZER_OFF;
    for (int i = 0; i < 16; i++) {
        gOutputStatus.lcdLine1[i] = ' ';
        gOutputStatus.lcdLine2[i] = ' ';
    }
    gOutputStatus.lcdLine1[16] = 0;
    gOutputStatus.lcdLine2[16] = 0;

    gSystemTimers.failedAttempts   = 0;
    gSystemTimers.penaltyLevel     = 0;
    gSystemTimers.penaltyEndTick   = 0;
    gSystemTimers.unlockExpireTick = 0;
    gSystemTimers.alarmRepeatTick  = 0;

    gSystemState.currentState = INIT;

    /* Default password - can be changed in State_Process */
    gPassword[0] = '1';
    gPassword[1] = '2';
    gPassword[2] = '3';
    gPassword[3] = 'A';
    gPassword[4] = 0;
}

#endif /* SRC_GLOBAL_C_ */
