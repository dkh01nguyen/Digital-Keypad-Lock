/*
 * global.c
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 */

#ifndef SRC_GLOBAL_C_
#define SRC_GLOBAL_C_

#include "global.h"
#include <string.h>

Keypad_HandleTypeDef hKeypad;
I2C_LCD_HandleTypeDef lcd1;

// Keymap 4x4 definition
char KEYMAP[NUMROWS][NUMCOLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'F', '0', 'E', 'D'}
};

InputState_t gInputState;
KeyEvent_t gKeyEvent;
OutputStatus_t gOutputStatus;
SystemTimers_t gSystemTimers;
SystemState_t gSystemState;

char gPassword[PASSWORD_LENGTH + 1];
char inputBuffer[MAX_INPUT_LENGTH + 1];

int timer_counter[NUM_TASKS];
int timer_flag[NUM_TASKS];
int TIMER_CYCLE = 10;

void init_global_variables(void)
{
    // Input
    gInputState.doorSensor = 1; // Default closed (Pressed)
    gInputState.keySensor = 0;
    gInputState.indoorButton = 0;
    gInputState.indoorButtonLong = 0;
    gInputState.batteryLow = 0;

    // Events
    gKeyEvent.keyChar = 0;
    gKeyEvent.isEnter = 0;
    gKeyEvent.isEnterLong = 0;
    gKeyEvent.isBackspace = 0;

    // Output
    gOutputStatus.ledGreen = LED_OFF;
    gOutputStatus.ledRed = LED_OFF;
    gOutputStatus.solenoid = SOLENOID_LOCKED;
    gOutputStatus.buzzer = BUZZER_OFF;
    memset(gOutputStatus.lcdLine1, ' ', 16); gOutputStatus.lcdLine1[16] = 0;
    memset(gOutputStatus.lcdLine2, ' ', 16); gOutputStatus.lcdLine2[16] = 0;

    // State
    gSystemState.currentState = LOCKED_SLEEP;

    // Default Password: 1234
    strcpy(gPassword, "1234");
    inputBuffer[0] = '\0';
}

#endif /* SRC_GLOBAL_C_ */
