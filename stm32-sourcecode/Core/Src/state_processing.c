/*
 * state_processing.c
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 * Description: Implements the 14-state Finite State Machine (FSM) logic.
 */
#include "state_processing.h"
#include "global.h"
#include "kmp.h"
#include "main.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

// --- Logic Constants and Variables ---

static const uint32_t penalty_minutes[] = { 1, 5, 25, 125 };
static const uint8_t penalty_levels = sizeof(penalty_minutes) / sizeof(penalty_minutes[0]);
static uint16_t inputLen;

#define WARNING_DURATION_MS     3000
#define INACTIVITY_TIMEOUT_MS   30000
#define RELOCK_WINDOW_MS        10000
#define DOOR_OPEN_MAX_MS        30000
#define ALARM_REPEAT_MS         (5U * 60U * 1000U)
#define BUZZER_DURATION_MS      10000

// --- Helper Functions ---

static void setLCD(const char *l1, const char *l2)
{
    if (l1) {
        strncpy((char*)gOutputStatus.lcdLine1, l1, 16);
        gOutputStatus.lcdLine1[16] = '\0';
    }
    if (l2) {
        strncpy((char*)gOutputStatus.lcdLine2, l2, 16);
        gOutputStatus.lcdLine2[16] = '\0';
    }
}

static void start_penalty(uint8_t level)
{
    if (level == 0) return;
    uint32_t minutes = penalty_minutes[(level - 1) < penalty_levels ? (level - 1) : (penalty_levels - 1)];
    uint32_t ms = minutes * 60U * 1000U;

    gSystemTimers.penaltyLevel = level;
    gSystemTimers.penaltyEndTick = HAL_GetTick() + ms; // Long duration timer
    gOutputStatus.buzzer = BUZZER_ON;
    setTimer(BUZZER_TASK_ID, BUZZER_DURATION_MS); // 10s buzzer
}

static void input_clear(void)
{
    inputLen = 0;
    inputBuffer[0] = '\0';
}

static void input_append(char c)
{
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))
    {
        if (inputLen < MAX_INPUT_LENGTH) {
            inputBuffer[inputLen++] = c;
            inputBuffer[inputLen] = '\0';
        }
    }
}

static bool verify_password()
{
    size_t len = inputLen;
    if (len < PASSWORD_LENGTH || len > MAX_INPUT_LENGTH) return false;
    return KMP_FindPassword((const uint8_t*)inputBuffer, (uint16_t)len);
}

// Hàm check_battery ĐÃ ĐƯỢC XÓA khỏi FSM.

// --- FSM API Implementations ---

/**
 * @brief Initializes the FSM state and global variables.
 */
void State_Init(void)
{
    gSystemState.currentState = LOCKED_SLEEP;
    gSystemTimers.failedAttempts = 0;
    gSystemTimers.penaltyLevel = 0;
    gSystemTimers.penaltyEndTick = 0;
    gSystemTimers.alarmRepeatTick = 0;

    // Output DO for LOCKED_SLEEP
    gOutputStatus.solenoid = SOLENOID_LOCKED;
    gOutputStatus.ledRed = LED_OFF;
    gOutputStatus.ledGreen = LED_OFF;
    gOutputStatus.buzzer = BUZZER_OFF;
    input_clear();
}

/**
 * @brief The main periodic FSM processing loop.
 */
void State_Process(void)
{
    uint32_t now = HAL_GetTick();

    // --- Global Timer and Event Consumption ---

    // 1. Buzzer OFF after 10s timer expires
    if (timer_flag[BUZZER_TASK_ID] == 1) {
        gOutputStatus.buzzer = BUZZER_OFF;
        timer_flag[BUZZER_TASK_ID] = 0;
    }

    // 2. Penalty Timer Expiration
    if (gSystemState.currentState == PENALTY_TIMER) {
        if (gSystemTimers.penaltyEndTick != 0 && gSystemTimers.penaltyEndTick != UINT32_MAX) {
            if ((int32_t)(gSystemTimers.penaltyEndTick - now) <= 0) {
                gSystemTimers.penaltyEndTick = 0;
                gSystemState.currentState = LOCKED_ENTRY;
                input_clear();
                setTimer(ENTRY_TIMEOUT_ID, INACTIVITY_TIMEOUT_MS);
                return;
            }
        }
    }

    // 3. Consume Input Event
    char key_char = gKeyEvent.keyChar;
    uint8_t is_long = gKeyEvent.isLong;
    gKeyEvent.keyChar = 0;
    gKeyEvent.isLong = 0;

    // 4. Handle Immediate Transitions (Mechanical Key / Long Press Indoor)

    // Mechanical Key Override
    if (gInputState.keySensor) {
        if (gSystemState.currentState <= LOCKED_VERIFY || gSystemState.currentState == PENALTY_TIMER || gSystemState.currentState == PERMANENT_LOCKOUT) {
             gSystemState.currentState = UNLOCKED_WAITOPEN;
             setTimer(UNLOCK_WINDOW_ID, RELOCK_WINDOW_MS); // 10s window
             gSystemTimers.penaltyEndTick = 0;
             gSystemTimers.failedAttempts = 0;
             return;
        }
    }

    // Long Press Indoor Button
    if (gInputState.indoorButton) {
        if (gSystemState.currentState == UNLOCKED_DOOROPEN || gSystemState.currentState == UNLOCKED_WAITCLOSE) {
            gSystemState.currentState = UNLOCKED_ALWAYSOPEN;
            gInputState.indoorButton = 0; // Consume event
            return;
        } else if (gSystemState.currentState == UNLOCKED_ALWAYSOPEN) {
            gSystemState.currentState = UNLOCKED_WAITCLOSE;
            setTimer(UNLOCK_WINDOW_ID, RELOCK_WINDOW_MS); // Start 10s relock
            gInputState.indoorButton = 0; // Consume event
            return;
        }
    }

    // --- FSM SWITCH-CASE (14 STATES) ---

    switch (gSystemState.currentState) {

        case LOCKED_SLEEP:
            // DO: Solenoid Lock, LED OFF, Buzzer OFF.
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_OFF;
            gOutputStatus.ledGreen = LED_OFF;
            gOutputStatus.buzzer = BUZZER_OFF;
            // Transitions: Handled by immediate logic (key_char != 0 or gInputState.keySensor)
            break;

        case LOCKED_WAKEUP:
            // DO: LED Red ON, Solenoid Lock.
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_ON;

            // Transitions:
            // Pin yếu? (Kiểm tra cờ đã được Input_Process cập nhật)
            if (gInputState.batteryLow) {
                gSystemState.currentState = BATTERY_WARNING;
                setTimer(WARNING_TASK_ID, WARNING_DURATION_MS); // 3s
            } else {
                gSystemState.currentState = LOCKED_ENTRY;
                input_clear();
                setTimer(ENTRY_TIMEOUT_ID, INACTIVITY_TIMEOUT_MS); // 30s
            }
            break;

        case BATTERY_WARNING:
            // DO: LED Red ON, Solenoid Lock.
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_ON;
            // Transitions:
            if (timer_flag[WARNING_TASK_ID] == 1) { // After 3s
                timer_flag[WARNING_TASK_ID] = 0;
                gSystemState.currentState = LOCKED_ENTRY;
                input_clear();
                setTimer(ENTRY_TIMEOUT_ID, INACTIVITY_TIMEOUT_MS); // 30s timeout
            }
            break;

        case LOCKED_ENTRY:
            // DO: LED Red ON, Solenoid Lock.
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_ON;

            // Transitions:
            if (timer_flag[ENTRY_TIMEOUT_ID] == 1) { // Timeout 30s
                timer_flag[ENTRY_TIMEOUT_ID] = 0;
                gSystemState.currentState = LOCKED_RELOCK;
            }
            else if (key_char == '#') { // Press Enter
                gSystemState.currentState = LOCKED_VERIFY;
            }
            else if (key_char != 0) { // Input handling
                if (key_char == '*') { // backspace
                    if (inputLen > 0) { inputLen--; inputBuffer[inputLen] = '\0'; }
                } else { // append
                    input_append(key_char);
                }
                setTimer(ENTRY_TIMEOUT_ID, INACTIVITY_TIMEOUT_MS); // reset timer
            }
            break;

        case LOCKED_VERIFY:
            // DO: Solenoid Lock, LED Red.
            // Transitions & logic:
            if (inputLen < PASSWORD_LENGTH || inputLen > MAX_INPUT_LENGTH) {
                // Lỗi định dạng (3s warning)
                setTimer(WARNING_TASK_ID, WARNING_DURATION_MS);
                gSystemState.currentState = BATTERY_WARNING;
                input_clear();
            } else if (verify_password()) {
                // Success
                gSystemState.currentState = UNLOCKED_WAITOPEN;
                setTimer(UNLOCK_WINDOW_ID, RELOCK_WINDOW_MS); // 10s window
                gSystemTimers.failedAttempts = 0;
            } else {
                // Failure
                gSystemTimers.failedAttempts++;
                input_clear();

                if ((gSystemTimers.failedAttempts % 3) == 0) { // Multiple of 3
                    if (gSystemTimers.failedAttempts >= 15) {
                        gSystemState.currentState = PERMANENT_LOCKOUT;
                        gSystemTimers.penaltyEndTick = UINT32_MAX;
                        gOutputStatus.buzzer = BUZZER_ON;
                        setTimer(BUZZER_TASK_ID, BUZZER_DURATION_MS);
                    } else {
                        uint8_t level = (uint8_t)(gSystemTimers.failedAttempts / 3);
                        if (level == 0) level = 1;
                        if (level > penalty_levels) level = penalty_levels;
                        start_penalty(level); // Activate penalty and buzzer
                        gSystemState.currentState = PENALTY_TIMER;
                    }
                } else {
                    // Normal failure (3s warning)
                    setTimer(WARNING_TASK_ID, WARNING_DURATION_MS);
                    gSystemState.currentState = BATTERY_WARNING;
                }
            }
            break;

        case PENALTY_TIMER:
            // DO: Solenoid Lock, LED Red, Buzzer ON.
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_ON;
            gOutputStatus.buzzer = BUZZER_ON;
            // Transitions: Handled by global logic (Penalty end) or immediate logic (Key Sensor)
            break;

        case PERMANENT_LOCKOUT:
            // DO: Solenoid Lock, LED Red, Buzzer ON.
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_ON;
            gOutputStatus.buzzer = BUZZER_ON;
            // Transitions: Handled by immediate logic (Key Sensor)
            break;

        case UNLOCKED_WAITOPEN:
            // DO: Solenoid UNLOCK, LED Green.
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;

            // Transitions:
            if (gInputState.doorSensor == 0) { // Door opens
                gSystemState.currentState = UNLOCKED_DOOROPEN;
                setTimer(UNLOCK_WINDOW_ID, DOOR_OPEN_MAX_MS); // 30s door open limit
            }
            else if (timer_flag[UNLOCK_WINDOW_ID] == 1) { // End of 10s window
                timer_flag[UNLOCK_WINDOW_ID] = 0;
                gSystemState.currentState = LOCKED_RELOCK;
            }
            else if (key_char == '#' && is_long) { // Long press Enter (#)
                gSystemState.currentState = UNLOCKED_SETPASSWORD;
                input_clear();
                setTimer(ENTRY_TIMEOUT_ID, INACTIVITY_TIMEOUT_MS); // 30s timeout
            }
            break;

        case UNLOCKED_SETPASSWORD:
            // DO: Solenoid UNLOCK, LED Green.
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;

            // Transitions:
            if (timer_flag[ENTRY_TIMEOUT_ID] == 1) { // Timeout 30s
                timer_flag[ENTRY_TIMEOUT_ID] = 0;
                gSystemState.currentState = UNLOCKED_WAITOPEN;
                setTimer(UNLOCK_WINDOW_ID, RELOCK_WINDOW_MS); // 10s window
            }
            else if (key_char != 0) { // Input handling
                if (key_char == '#') { // Enter (confirm)
                    if (inputLen == PASSWORD_LENGTH) {
                        State_SetPassword(inputBuffer);
                        gSystemState.currentState = LOCKED_RELOCK;
                    } else {
                        // Format error (3s warning)
                        setTimer(WARNING_TASK_ID, WARNING_DURATION_MS);
                        gSystemState.currentState = BATTERY_WARNING;
                    }
                    input_clear();
                } else if (key_char == '*') { // Backspace
                    if (inputLen > 0) { inputLen--; inputBuffer[inputLen] = '\0'; }
                } else { // Append (limit to 4 characters)
                    if (inputLen < PASSWORD_LENGTH) input_append(key_char);
                }
                setTimer(ENTRY_TIMEOUT_ID, INACTIVITY_TIMEOUT_MS); // reset timer
            }
            break;

        case UNLOCKED_DOOROPEN:
            // DO: Solenoid UNLOCK, LED Green.
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;

            // Transitions:
            if (gInputState.doorSensor == 1) { // Door closes
                // Long Press Indoor Button (handled by immediate logic)
                gSystemState.currentState = UNLOCKED_WAITCLOSE;
                setTimer(UNLOCK_WINDOW_ID, RELOCK_WINDOW_MS); // 10s relock timer
            }
            else if (timer_flag[UNLOCK_WINDOW_ID] == 1) { // End of 30s door open limit
                timer_flag[UNLOCK_WINDOW_ID] = 0;
                gSystemState.currentState = ALARM_FORGOTCLOSE;
                gSystemTimers.alarmRepeatTick = now + ALARM_REPEAT_MS; // 5 minute repeat
                gOutputStatus.buzzer = BUZZER_ON;
                setTimer(BUZZER_TASK_ID, BUZZER_DURATION_MS);
            }
            break;

        case ALARM_FORGOTCLOSE:
            // DO: Solenoid UNLOCK, LED Green, Buzzer ON.
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;
            gOutputStatus.buzzer = BUZZER_ON;

            // Transitions:
            if (gInputState.doorSensor == 1) { // Door closes
                gSystemState.currentState = UNLOCKED_WAITCLOSE;
                setTimer(UNLOCK_WINDOW_ID, RELOCK_WINDOW_MS);
            }
            else if ((int32_t)(gSystemTimers.alarmRepeatTick - now) <= 0) { // Repeat alarm every 5 min
                 gSystemTimers.alarmRepeatTick = now + ALARM_REPEAT_MS;
                 gOutputStatus.buzzer = BUZZER_ON; // Activate buzzer again
                 setTimer(BUZZER_TASK_ID, BUZZER_DURATION_MS);
            }
            break;

        case UNLOCKED_WAITCLOSE:
            // DO: Solenoid UNLOCK, LED Green.
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;

            // Transitions:
            if (gInputState.doorSensor == 0) { // Door opens again
                gSystemState.currentState = UNLOCKED_DOOROPEN;
                setTimer(UNLOCK_WINDOW_ID, DOOR_OPEN_MAX_MS);
            }
            else if (timer_flag[UNLOCK_WINDOW_ID] == 1) { // End of 10s relock
                timer_flag[UNLOCK_WINDOW_ID] = 0;
                gSystemState.currentState = LOCKED_RELOCK;
            }
            break;

        case UNLOCKED_ALWAYSOPEN:
            // DO: Solenoid UNLOCK, LED Green.
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;

            // Transitions: Long Press Indoor Button (handled by immediate logic)
            break;

        case LOCKED_RELOCK:
            // DO: Solenoid LOCK, LED Red.
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_ON;

            // Transitions: Transition to SLEEP after 3s
            if (timer_flag[WARNING_TASK_ID] == 1) {
                 timer_flag[WARNING_TASK_ID] = 0;
                 gSystemState.currentState = LOCKED_SLEEP;
            } else {
                 setTimer(WARNING_TASK_ID, WARNING_DURATION_MS); // Start 3s timer
            }
            break;

        default:
            gSystemState.currentState = LOCKED_SLEEP;
            break;
    }
}

// --- Event Handlers and Public API ---

void State_Event_KeypadChar(char c)
{
    gKeyEvent.keyChar = c;
    gKeyEvent.isLong = 0;
}

void State_Event_KeypadChar_Long(char c)
{
    gKeyEvent.keyChar = c;
    gKeyEvent.isLong = 1;
}

void State_Event_IndoorButton_Long(void)
{
    gInputState.indoorButton = 1;
}

void State_Event_KeySensor(void)
{
    gInputState.keySensor = 1;
}

void State_Event_DoorSensor_Open(void)
{
    gInputState.doorSensor = 0;
}

void State_Event_DoorSensor_Close(void)
{
    gInputState.doorSensor = 1;
}

bool State_SetPassword(const char *newPass)
{
    if (!newPass) return false;
    size_t len = strlen(newPass);
    if (len != PASSWORD_LENGTH) return false;

    memcpy(gPassword, newPass, PASSWORD_LENGTH);
    gPassword[PASSWORD_LENGTH] = '\0';
    return true;
}

const char* State_GetPassword(void)
{
    return (const char*)gPassword;
}

uint8_t State_GetState(void)
{
    return gSystemState.currentState;
}

void State_ForceUnlock(void)
{
    gSystemState.currentState = UNLOCKED_WAITOPEN;
    gOutputStatus.solenoid = SOLENOID_UNLOCKED;
}
