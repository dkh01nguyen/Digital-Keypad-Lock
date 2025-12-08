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
#include "timer.h" // Includes timer API

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3
#define KEYPAD_KEYS (KEYPAD_ROWS * KEYPAD_COLS)
#define KEYPAD_SCAN_PERIOD_MS 10
#define KEYPAD_DOUBLE_TIMEOUT_MS 350
#define KEYPAD_DOUBLE_TIMEOUT 35 // Cycles for 350ms timeout

// Struct save port/pin for Keypad
typedef struct {
    GPIO_TypeDef *ROW_Port[KEYPAD_ROWS];
    uint16_t ROW_Pin[KEYPAD_ROWS];
    GPIO_TypeDef *COL_Port[KEYPAD_COLS];
    uint16_t COL_Pin[KEYPAD_COLS];
} Keypad_HandleTypeDef;
extern Keypad_HandleTypeDef hKeypad;

// LCD Handle (Defined in i2c_lcd.h)
extern I2C_LCD_HandleTypeDef lcd1;

// Hardware-level defines
#define DURATION_FOR_AUTO_INCREASING 100 // 100 cycles = 1s for long press
#define BUTTON_IS_PRESSED GPIO_PIN_RESET
#define BUTTON_IS_RELEASED GPIO_PIN_SET

// System states (FSM)
#define INIT 0
#define LOCKED_SLEEP 1
#define LOCKED_WAKEUP 2
#define BATTERY_WARNING 3
#define LOCKED_ENTRY 4
#define LOCKED_VERIFY 5

#define PENALTY_TIMER 10
#define PERMANENT_LOCKOUT 11

#define UNLOCKED_WAITOPEN 20
#define UNLOCKED_DOOROPEN 21
#define UNLOCKED_ALWAYSOPEN 22
#define ALARM_FORGOTCLOSE 23
#define UNLOCKED_WAITCLOSE 24
#define UNLOCKED_SETPASSWORD 25
#define LOCKED_RELOCK 26

// Indices for input_reading module
#define N0_OF_BUTTONS 3
#define DOOR_SENSOR_INDEX 0
#define KEY_SENSOR_INDEX 1
#define INDOOR_BUTTON_INDEX 2

// Keypad flags (Used by keypad.c)
extern uint8_t keypad_char_ready;
extern char keypad_last_char;
extern char keypad_preview_char;

/* Input state snapshot (from input_processing) */
typedef struct {
    uint8_t doorSensor;     // 1 = closed (Snapshot)
    uint8_t keySensor;      // 1 = mechanical key used (Snapshot)
    uint8_t indoorButton;   // 1 = long press event detected (Snapshot)
    uint8_t batteryLow;     // 1 if low battery status
} InputState_t;
extern InputState_t gInputState;

/* Key event (single-slot mailbox for FSM) */
typedef struct {
    char keyChar;   // '#' enter, '*' backspace, '0'..'9', 'A'..'F', 0 = none
    uint8_t isLong; // 1 if long press (>1s) for '#'
} KeyEvent_t;
extern KeyEvent_t gKeyEvent;

/* Output status */
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

// Timer Task IDs (Fixed duration timers managed by timer.c)
#define BUZZER_TASK_ID      0
#define WARNING_TASK_ID     1  // Used for 3s durations (Battery, Format error, Relock 3s)
#define ENTRY_TIMEOUT_ID    2  // Used for 30s inactivity (Locked_Entry, SetPassword)
#define UNLOCK_WINDOW_ID    3  // Used for 10s/30s durations (WaitOpen, DoorOpen, WaitClose)

/* System Timers (Long term state/timers) */
typedef struct {
    uint32_t failedAttempts;
    uint8_t  penaltyLevel;
    uint32_t penaltyEndTick;    // HAL_GetTick for long duration Penalty
    uint32_t alarmRepeatTick;   // HAL_GetTick for Alarm repeat time
} SystemTimers_t;
extern SystemTimers_t gSystemTimers;

/* Current FSM State */
typedef struct {
    uint8_t currentState;
} SystemState_t;
extern SystemState_t gSystemState;

// Password storage and input buffer
#define MAX_INPUT_LENGTH 20
#define PASSWORD_LENGTH 4
extern char gPassword[PASSWORD_LENGTH + 1];
extern char inputBuffer[MAX_INPUT_LENGTH + 1];

// Timer helper (from timer.c)
#define NUM_TASKS 10
extern int timer_counter[NUM_TASKS];
extern int timer_flag[NUM_TASKS];
extern int TIMER_CYCLE; // 10ms

// Initialization helper
void init_global_variables(void);

#endif /* INC_GLOBAL_H_ */
