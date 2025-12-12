/*
 * global.h
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 */
#ifndef INC_GLOBAL_H_
#define INC_GLOBAL_H_

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "i2c_lcd.h"
#include "timer.h"

// --- 1. Keypad Configuration ---
#define NUMROWS 4
#define NUMCOLS 4
// Key mapping: Matrix 4x4
extern char KEYMAP[NUMROWS][NUMCOLS];

typedef struct {
	uint32_t RowPins[NUMROWS];
	uint32_t ColPins[NUMCOLS];
	GPIO_TypeDef* RowPort[NUMROWS];
	GPIO_TypeDef* ColPort[NUMCOLS];
	char MAP[NUMROWS][NUMCOLS];
	char Value;
} Keypad_HandleTypeDef;

extern Keypad_HandleTypeDef hKeypad;
extern I2C_LCD_HandleTypeDef lcd1;

// --- 2. Button & Sensor Config ---
#define N0_OF_BUTTONS 					5
#define DURATION_FOR_AUTO_INCREASING 	100 // 100 * 10ms = 1s for long press

// Logic level definitions
#define BUTTON_IS_PRESSED 			GPIO_PIN_RESET
#define BUTTON_IS_RELEASED 			GPIO_PIN_SET

// Button Indices (Must match input_reading.c switch-case)
#define DOOR_SENSOR_INDEX 		0
#define KEY_SENSOR_INDEX 		1
#define INDOOR_BUTTON_INDEX		2
#define ENTER_BUTTON_INDEX 		3
#define BACKSPACE_BUTTON_INDEX 	4

// --- 3. System States (14 States) ---
#define LOCKED_SLEEP 			1
#define LOCKED_WAKEUP 			2
#define BATTERY_WARNING 		3
#define LOCKED_ENTRY 			4
#define LOCKED_VERIFY 			5
#define PENALTY_TIMER 			6
#define PERMANENT_LOCKOUT 		7
#define UNLOCKED_WAITOPEN 		8
#define UNLOCKED_SETPASSWORD	9
#define UNLOCKED_DOOROPEN 		10
#define ALARM_FORGOTCLOSE 		11
#define UNLOCKED_WAITCLOSE 		12
#define UNLOCKED_ALWAYSOPEN 	13
#define LOCKED_RELOCK 			14

// --- 4. Data Structures ---

/* Input Snapshot: Holds processing results for FSM */
typedef struct {
    uint8_t doorSensor;      // 1 = CLOSED (Pressed), 0 = OPEN
    uint8_t keySensor;       // 1 = Active
    uint8_t indoorButton;    // 1 = Active (Short press)
    uint8_t indoorButtonLong;// 1 = Active (Long press > 1s)
    uint8_t batteryLow;      // 1 = Low Battery
} InputState_t;
extern InputState_t gInputState;

/* Key Event Mailbox: Keypad & Discrete Action Buttons */
typedef struct {
    char keyChar;       	// '0'-'9', 'A'-'F' from Keypad
    uint8_t isEnter;    	// 1 = Enter Pressed
    uint8_t isEnterLong;	// 1 = Enter Held > 1s
    uint8_t isBackspace;	// 1 = Backspace Pressed
} KeyEvent_t;
extern KeyEvent_t gKeyEvent;

/* Output Status Control */
typedef enum { LED_OFF = 0, LED_ON = 1 } Led_t;
typedef enum { SOLENOID_LOCKED = 0, SOLENOID_UNLOCKED = 1 } Solenoid_t;
typedef enum { BUZZER_OFF = 0, BUZZER_ON = 1 } Buzzer_t;

typedef struct {
    Led_t ledGreen;
    Led_t ledRed;
    Solenoid_t solenoid;
    Buzzer_t buzzer;
    char lcdLine1[17];
    char lcdLine2[17];
} OutputStatus_t;
extern OutputStatus_t gOutputStatus;

// --- 5. Timers & Logic ---
// Timer Task IDs
#define BUZZER_TASK_ID      0
#define WARNING_TASK_ID     1  // 3s timers
#define ENTRY_TIMEOUT_ID    2  // 30s timers
#define UNLOCK_WINDOW_ID    3  // 10s/30s timers
#define MASK_TIMER_ID       4  // 1s timer for masking password char

/* System Timers (Long term) */
typedef struct {
    uint32_t failedAttempts;
    uint8_t  penaltyLevel;
    uint32_t penaltyEndTick;
    uint32_t alarmRepeatTick;
} SystemTimers_t;
extern SystemTimers_t gSystemTimers;

/* Current FSM State */
typedef struct {
    uint8_t currentState;
} SystemState_t;
extern SystemState_t gSystemState;

// Password & Buffer
#define MAX_INPUT_LENGTH 20
#define PASSWORD_LENGTH 4
extern char gPassword[PASSWORD_LENGTH + 1];
extern char inputBuffer[MAX_INPUT_LENGTH + 1];

// Timer helper
#define NUM_TASKS 5
extern int timer_counter[NUM_TASKS];
extern int timer_flag[NUM_TASKS];
extern int TIMER_CYCLE; // 10ms

void init_global_variables(void);

#endif /* INC_GLOBAL_H_ */
