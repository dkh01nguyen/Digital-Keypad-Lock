/*
 * global.h
 *
 *  Created on: Nov 20, 2025
 *      Author: nguye
 */

#ifndef INC_GLOBAL_H_
#define INC_GLOBAL_H_

//#include <stdint.h>
//#include <stdbool.h>
//
//#define BUTTON_IS_PRESSED 		GPIO_PIN_RESET
//#define BUTTON_IS_RELEASED 		GPIO_PIN_SET
//
//// Keypad 3x4 and indoor unlock button, door, key sensor
//#define N0_OF_BUTTONS			15
//extern uint8_t keypad_char_ready;     // 1 nếu đã có ký tự hoàn chỉnh
//extern char keypad_last_char;         // Ký tự cuối cùng đã hoàn chỉnh
//extern char keypad_preview_char;      // Ký tự đang preview
//
//// System states
//#define INIT					0
//#define LOCKED_SLEEP			1
//#define LOCKED_WAKEUP			2
//#define BATTERY_WARNING			3
//#define LOCKED_ENTRY			4
//#define LOCKED_VERIFY			5
//#define PENALTY_TIMER			10
//#define PERMANENT_LOCKOUT		11
//#define UNLOCKED_WAITOPEN		20
//#define UNLOCKED_DOOROPEN		21
//#define UNLOCKED_ALWAYSOPEN		22
//#define ALARM_FORGOTCLOSE		23
//#define UNLOCKED_WAITCLOSE		24
//#define UNLOCKED_SETPASSWORD	25
//#define LOCKED_RELOCK			26
//
//// Input Processing
//typedef struct {
//    uint8_t keypadChar;      // Last confirmed key
//    uint8_t indoorButton;    // 1 = pressed, 0 = released
//    uint8_t keySensor;       // 1 = turned, 0 = normal
//    uint8_t doorSensor;      // 1 = open, 0 = closed
//} InputState_t;
//
//extern InputState_t gInputState;
//extern char gKeyEvent;       // Last keypad event for state_processing
//
//// Output Processing
//typedef struct {
//    uint8_t ledRed;          // 0 = off, 1 = on
//    uint8_t ledGreen;        // 0 = off, 1 = on
//    uint8_t buzzer;          // 0 = off, 1 = on
//    uint8_t solenoid;        // 0 = locked, 1 = unlocked
//    char lcdLine1[17];       // LCD line 1 (16 chars + null)
//    char lcdLine2[17];       // LCD line 2
//} OutputStatus_t;
//
//extern OutputStatus_t gOutputStatus;
//
//// State Processing
//typedef struct {
//    uint8_t currentState;    // Current system state (INIT, LOCKED_SLEEP, ...)
//    uint16_t timers[10];     // Generic timers for penalties, auto-relock, etc.
//} SystemState_t;
//
//extern SystemState_t gSystemState;
//
//// Password storage
//#define MAX_PASSWORD_LENGTH 20
//extern char gPassword[MAX_PASSWORD_LENGTH + 1];  // Null-terminated
//
//// TIM SETUP
//#define NUM_TASKS			10
//
//extern int timer_counter[NUM_TASKS];
//extern int timer_flag[NUM_TASKS];
//extern int TIMER_CYCLE;



#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "i2c_lcd.h"


#define KEYPAD_ROWS     4
#define KEYPAD_COLS     3
#define KEYPAD_KEYS     (KEYPAD_ROWS * KEYPAD_COLS)

#define KEYPAD_SCAN_PERIOD_MS   	10
#define KEYPAD_DOUBLE_TIMEOUT_MS 	350
#define KEYPAD_DOUBLE_TIMEOUT    	35


// Struct lưu port/pin
typedef struct {
    GPIO_TypeDef *ROW_Port[KEYPAD_ROWS];
    uint16_t ROW_Pin[KEYPAD_ROWS];

    GPIO_TypeDef *COL_Port[KEYPAD_COLS];
    uint16_t COL_Pin[KEYPAD_COLS];
} Keypad_HandleTypeDef;

extern Keypad_HandleTypeDef hKeypad;
extern I2C_LCD_HandleTypeDef lcd1;


/* -------------------- Hardware-level defines -------------------- */
#define DURATION_FOR_AUTO_INCREASING 	100
#define BUTTON_IS_PRESSED 				GPIO_PIN_RESET
#define BUTTON_IS_RELEASED 				GPIO_PIN_SET

/* Keypad flags (optional globals) */
extern uint8_t keypad_char_ready;     // 1 nếu đã có ký tự hoàn chỉnh
extern char    keypad_last_char;      // Ký tự cuối cùng đã hoàn chỉnh
extern char    keypad_preview_char;   // Ký tự đang preview

/* -------------------- System states -------------------- */
#define INIT                    0
#define LOCKED_SLEEP            1
#define LOCKED_WAKEUP           2
#define BATTERY_WARNING         3
#define LOCKED_ENTRY            4
#define LOCKED_VERIFY           5
#define PENALTY_TIMER           10
#define PERMANENT_LOCKOUT       11
#define UNLOCKED_WAITOPEN       20
#define UNLOCKED_DOOROPEN       21
#define UNLOCKED_ALWAYSOPEN     22
#define ALARM_FORGOTCLOSE       23
#define UNLOCKED_WAITCLOSE      24
#define UNLOCKED_SETPASSWORD    25
#define LOCKED_RELOCK           26

/* -------------------- Input state -------------------- */
typedef struct {
    /* Last finalized keypad char (0 if none) - filled by input_processing */
    char latestKey;

    /* Discrete sensors (updated by input_reading.c or input_processing) */
    uint8_t doorSensor;      /* 1 = closed (pressed) or as wired */
    uint8_t keySensor;       /* 1 = mechanical key turned */
    uint8_t indoorButton;    /* 1 = inside unlock button pressed */

    /* Battery / ADC snapshot */
    uint16_t batteryADC;     /* ADC reading */
    uint8_t  batteryLow;     /* 1 if low battery */
} InputState_t;

extern InputState_t gInputState;

/* Key event: single-slot event produced by input_processing, consumed by state_processing */
typedef struct {
    char keyChar;   /* '#' enter, '*' backspace, '0'..'9', 'A'..'F', 0 = none */
    uint8_t isLong; /* 1 if long press (>1s) */
} KeyEvent_t;

extern KeyEvent_t gKeyEvent;

/* -------------------- Output status -------------------- */
typedef enum {
    LED_OFF = 0,
    LED_ON  = 1,
} Led_t;

typedef enum {
    SOLENOID_LOCKED = 0,
    SOLENOID_UNLOCKED = 1,
} Solenoid_t;

typedef enum {
    BUZZER_OFF = 0,
    BUZZER_ON  = 1,
} Buzzer_t;

typedef struct {
    Led_t ledGreen;
    Led_t ledRed;
    Solenoid_t solenoid;
    Buzzer_t buzzer;
    char lcdLine1[17];
    char lcdLine2[17];
} OutputStatus_t;

extern OutputStatus_t gOutputStatus;

/* -------------------- System timers / meta -------------------- */
typedef struct {
    uint32_t failedAttempts;      /* total incorrect attempts */
    uint8_t  penaltyLevel;        /* 0 = none, 1..n */
    uint32_t penaltyEndTick;      /* tick when penalty ends (HAL_GetTick), 0 = none, UINT32_MAX = permanent */
    uint32_t unlockExpireTick;    /* tick for auto relock */
    uint32_t alarmRepeatTick;     /* tick for repeating alarms */
} SystemTimers_t;

extern SystemTimers_t gSystemTimers;

/* -------------------- System state -------------------- */
typedef struct {
    uint8_t currentState;         /* e.g. LOCKED_SLEEP, ... */
} SystemState_t;

extern SystemState_t gSystemState;

/* -------------------- Password storage -------------------- */
#define MAX_PASSWORD_LENGTH 20
extern char gPassword[MAX_PASSWORD_LENGTH + 1];  /* null terminated */

/* -------------------- Timer helper (from timer.c) -------------------- */
/* If you use scheduler timers you may keep these, else use HAL_GetTick() */
#define NUM_TASKS 10
extern int timer_counter[NUM_TASKS];
extern int timer_flag[NUM_TASKS];
extern int TIMER_CYCLE;

/* -------------------- Initialization helper -------------------- */
void init_global_variables(void);


#endif /* INC_GLOBAL_H_ */
