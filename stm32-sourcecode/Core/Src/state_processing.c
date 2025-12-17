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
#include "timer.h"
#include <string.h>

// --- Constants & Config ---
static const uint32_t penalty_minutes[] = {1, 5, 25, 125};
static const uint8_t MAX_PENALTY_LEVEL = 4;

#define TIMEOUT_3S_CYCLES   3000  				// 300
#define TIMEOUT_10S_CYCLES  10000 				// 1000
#define TIMEOUT_30S_CYCLES  30000				// 3000
#define ALARM_REPEAT_MS     (5 * 60 * 1000)     // 5 minutes

// --- Internal Variables ---
static uint16_t inputLen = 0;
static bool isShowingError = false; // Flag to hold VERIFY state for 3s error display

// --- Helper Functions ---

/* Clear input buffer */
static void input_clear(void) {
    inputLen = 0;
    inputBuffer[0] = '\0';
}

/* Append char to buffer (limit 20 chars) */
static void input_append(char c) {
    if (inputLen < MAX_INPUT_LENGTH) {
        inputBuffer[inputLen++] = c;
        inputBuffer[inputLen] = '\0';
    }
}

/* Remove last char */
static void input_backspace(void) {
    if (inputLen > 0) {
        inputLen--;
        inputBuffer[inputLen] = '\0';
    }
}

/* Verify password using KMP */
static bool verify_password(void) {
    if (inputLen < PASSWORD_LENGTH) return false;
    return KMP_FindPassword((const uint8_t*)inputBuffer, (uint16_t)inputLen);
}

/* Calculate penalty end time based on level */
static void activate_penalty(uint8_t level) {
    uint32_t minutes = 0;
    if (level > 0 && level <= MAX_PENALTY_LEVEL) {
        minutes = penalty_minutes[level - 1];
    } else {
        minutes = penalty_minutes[MAX_PENALTY_LEVEL - 1];
    }
    gSystemTimers.penaltyEndTick = HAL_GetTick() + (minutes * 60 * 1000);
}

// --- Main API ---

void State_Init(void) {
    gSystemState.currentState = LOCKED_SLEEP;
    gSystemTimers.failedAttempts = 0;
    gSystemTimers.penaltyLevel = 0;
    input_clear();
}

void State_Process(void) {
    // uint32_t now = HAL_GetTick();

    // --- GLOBAL OVERRIDES (Highest Priority) ---

    /* Mechanical Key OR Indoor Button (Short Press)
     * Acts as Master Unlock in locked states.
     */
    if (gInputState.keySensor == 1 || gInputState.indoorButton == 1)
    {
        // Override conditions: Not already open logic (to prevent state hopping)
        if (gSystemState.currentState <= PERMANENT_LOCKOUT ||
            gSystemState.currentState == LOCKED_RELOCK ||
            gSystemState.currentState == ALARM_FORGOTCLOSE) // Emergency open even if alarming
        {
            gSystemState.currentState = UNLOCKED_WAITOPEN;
            setTimer(UNLOCK_WINDOW_ID, TIMEOUT_10S_CYCLES);

            // Reset Penalties
            gSystemTimers.failedAttempts = 0;
            gSystemTimers.penaltyEndTick = 0;

            // Consume events
            gInputState.keySensor = 0;
            gInputState.indoorButton = 0;
            return; // Exit immediately to next cycle
        }
    }

    // --- STATE MACHINE LOGIC ---

    switch (gSystemState.currentState) {
        case LOCKED_SLEEP:
            // Transitions:
            // Any Key -> Wakeup
            if (gKeyEvent.keyChar != 0)
            {
                gSystemState.currentState = LOCKED_WAKEUP;
                gKeyEvent.keyChar = 0;
            }
            break;

        case LOCKED_WAKEUP:
        	// Ensuring entry safety
        	input_clear();
            // Check Battery
            if (gInputState.batteryLow)
            {
                gSystemState.currentState = BATTERY_WARNING;
                setTimer(WARNING_TASK_ID, TIMEOUT_3S_CYCLES);
            } else {
                gSystemState.currentState = LOCKED_ENTRY;
                setTimer(ENTRY_TIMEOUT_ID, TIMEOUT_30S_CYCLES);
            }
            break;

        case BATTERY_WARNING:
            // Transitions: After 3s -> Locked Entry
            if (timer_flag[WARNING_TASK_ID] == 1)
            {
                gSystemState.currentState = LOCKED_ENTRY;
                setTimer(ENTRY_TIMEOUT_ID, TIMEOUT_30S_CYCLES);
            }
            break;

        case LOCKED_ENTRY:
            // Transitions:
            // 1. Timeout 30s -> Sleep
            if (timer_flag[ENTRY_TIMEOUT_ID] == 1)
            {
                gSystemState.currentState = LOCKED_SLEEP;
            }
            // 2. Enter Pressed -> Verify
            else if (gKeyEvent.isEnter)
            {
                gSystemState.currentState = LOCKED_VERIFY;
                isShowingError = false; // Reset error flag
                gKeyEvent.isEnter = 0;
            }
            // 3. Input Handling
            else if (gKeyEvent.isBackspace)
            {
                input_backspace();
                setTimer(ENTRY_TIMEOUT_ID, TIMEOUT_30S_CYCLES); // Reset timeout
                gKeyEvent.isBackspace = 0;
            }
            else if (gKeyEvent.keyChar != 0)
            {
                input_append(gKeyEvent.keyChar);
                setTimer(ENTRY_TIMEOUT_ID, TIMEOUT_30S_CYCLES); // Reset timeout
                gKeyEvent.keyChar = 0;
            }
            break;

        case LOCKED_VERIFY:
            // Logic: Check password
            // If just entered state (isShowingError == false)
            if (!isShowingError)
            {
                bool isCorrect = verify_password();

                // Case A: Format Error (Len < 4 or > 20)
                if (inputLen < PASSWORD_LENGTH || inputLen > MAX_INPUT_LENGTH)
                {
                    isShowingError = true;
                    setTimer(WARNING_TASK_ID, TIMEOUT_3S_CYCLES); // Show error 3s
                    // Output processing will check "isShowingError" to display text
                }
                // Case B: Correct Password
                else if (isCorrect)
                {
                    gSystemState.currentState = UNLOCKED_WAITOPEN;
                    setTimer(UNLOCK_WINDOW_ID, TIMEOUT_10S_CYCLES);
                    gSystemTimers.failedAttempts = 0;
                    input_clear();
                }
                // Case C: Wrong Password
                else {
                    gSystemTimers.failedAttempts++;
                    isShowingError = true;
                    setTimer(WARNING_TASK_ID, TIMEOUT_3S_CYCLES);
//                    input_clear();

                    // Check Modulo 3
                    if ((gSystemTimers.failedAttempts % 3) == 0)
                    {
                        // Max attempts reached
                        if (gSystemTimers.failedAttempts >= 15)
                        {
                            gSystemState.currentState = PERMANENT_LOCKOUT;
                            // Start Buzzer 10s
                            gOutputStatus.buzzer = BUZZER_ON;
                            setTimer(BUZZER_TASK_ID, TIMEOUT_10S_CYCLES);
                        } else { // Penalty Timer
                            uint8_t level = gSystemTimers.failedAttempts / 3;
                            activate_penalty(level);
                            gSystemState.currentState = PENALTY_TIMER;
                            gOutputStatus.buzzer = BUZZER_ON;
                            setTimer(BUZZER_TASK_ID, TIMEOUT_10S_CYCLES);
                        }
                    } else { // Normal Wrong (Not divisible by 3)
                        isShowingError = true;
                        setTimer(WARNING_TASK_ID, TIMEOUT_3S_CYCLES); // Show error 3s
                    }
                }
            }
            // If showing error (Wait for 3s timer)
            else {
                if (timer_flag[WARNING_TASK_ID] == 1)
                {
                    gSystemState.currentState = LOCKED_ENTRY;
                    input_clear();
                    setTimer(ENTRY_TIMEOUT_ID, TIMEOUT_30S_CYCLES);
                }
            }
            break;

        case PENALTY_TIMER:
            // Logic: Wait for penalty end
            // 1. Buzzer timeout (10s)
            if (timer_flag[BUZZER_TASK_ID] == 1)
            {
                gOutputStatus.buzzer = BUZZER_OFF;
            }
            // 2. Check Penalty Time
            if (HAL_GetTick() >= gSystemTimers.penaltyEndTick)
            {
                gSystemState.currentState = LOCKED_ENTRY;
                input_clear();
                setTimer(ENTRY_TIMEOUT_ID, TIMEOUT_30S_CYCLES);
            }
            break;

        case PERMANENT_LOCKOUT:
            // Logic: Infinite loop until Master Key (Handled in Global Overrides)
            // Buzzer timeout
            if (timer_flag[BUZZER_TASK_ID] == 1)
            {
                gOutputStatus.buzzer = BUZZER_OFF;
            }
            break;

        case UNLOCKED_WAITOPEN:
            // Transitions:
            // 1. Door Opens -> DoorOpen
            if (gInputState.doorSensor == 0)
            {
                gSystemState.currentState = UNLOCKED_DOOROPEN;
                setTimer(UNLOCK_WINDOW_ID, TIMEOUT_30S_CYCLES);
            }
            // 2. Timeout 10s (Door never opened) -> Relock
            else if (timer_flag[UNLOCK_WINDOW_ID] == 1)
            {
                gSystemState.currentState = LOCKED_RELOCK;
            }
            // 3. Enter Long Press -> Set Password
            else if (gKeyEvent.isEnterLong)
            {
                gSystemState.currentState = UNLOCKED_SETPASSWORD;
                input_clear();
                setTimer(ENTRY_TIMEOUT_ID, TIMEOUT_30S_CYCLES);
                gKeyEvent.isEnterLong = 0;
            }
            break;

        case UNLOCKED_SETPASSWORD:
            // Logic: Input new password
            // 1. Timeout 30s -> WaitOpen
            if (timer_flag[ENTRY_TIMEOUT_ID] == 1)
            {
                gSystemState.currentState = UNLOCKED_WAITOPEN;
                setTimer(UNLOCK_WINDOW_ID, TIMEOUT_10S_CYCLES);
            }
            // 2. Enter Pressed -> Save
            else if (gKeyEvent.isEnter)
            {
                if (inputLen == PASSWORD_LENGTH)
                {
                    State_SetPassword(inputBuffer); // Update global password
                    gSystemState.currentState = LOCKED_RELOCK;
                } else {
                    gSystemState.currentState = UNLOCKED_WAITOPEN;
                    setTimer(UNLOCK_WINDOW_ID, TIMEOUT_10S_CYCLES);
                }
                gKeyEvent.isEnter = 0;
            }
            // 3. Input
            else if (gKeyEvent.keyChar != 0)
            {
                // Only allow input up to 4 chars
                if (inputLen < PASSWORD_LENGTH)
                {
                    input_append(gKeyEvent.keyChar);
                }
                setTimer(ENTRY_TIMEOUT_ID, TIMEOUT_30S_CYCLES);
                gKeyEvent.keyChar = 0;
            }
            else if (gKeyEvent.isBackspace)
            {
                input_backspace();
                setTimer(ENTRY_TIMEOUT_ID, TIMEOUT_30S_CYCLES);
                gKeyEvent.isBackspace = 0;
            }
            break;

        case UNLOCKED_DOOROPEN:
            // Transitions:
            // 1. Door Closes -> WaitClose
            if (gInputState.doorSensor == 1)
            {
                gSystemState.currentState = UNLOCKED_WAITCLOSE;
                setTimer(UNLOCK_WINDOW_ID, TIMEOUT_10S_CYCLES);
            }
            // 2. Indoor Button Long Press -> Always Open
            else if (gInputState.indoorButtonLong)
            {
                gSystemState.currentState = UNLOCKED_ALWAYSOPEN;
                gInputState.indoorButtonLong = 0;
            }
            // 3. Timeout 30s -> Alarm
            else if (timer_flag[UNLOCK_WINDOW_ID] == 1)
            {
                gSystemState.currentState = ALARM_FORGOTCLOSE;
                gSystemTimers.alarmRepeatTick = HAL_GetTick() + ALARM_REPEAT_MS;
                // Start Buzzer 10s
                gOutputStatus.buzzer = BUZZER_ON;
                setTimer(BUZZER_TASK_ID, TIMEOUT_10S_CYCLES);
            }
            break;

        case ALARM_FORGOTCLOSE:
            // Transitions:
            // 1. Door Closes -> WaitClose
            if (gInputState.doorSensor == 1)
            {
                gSystemState.currentState = UNLOCKED_WAITCLOSE;
                setTimer(UNLOCK_WINDOW_ID, TIMEOUT_10S_CYCLES);
                gOutputStatus.buzzer = BUZZER_OFF; // Stop alarm
            }
            // 2. Buzzer Timeout (10s)
            if (timer_flag[BUZZER_TASK_ID] == 1)
            {
                gOutputStatus.buzzer = BUZZER_OFF;
            }
            // 3. Repeat Alarm (5 min)
            if (HAL_GetTick() >= gSystemTimers.alarmRepeatTick)
            {
                gSystemTimers.alarmRepeatTick = HAL_GetTick() + ALARM_REPEAT_MS;
                gOutputStatus.buzzer = BUZZER_ON;
                setTimer(BUZZER_TASK_ID, TIMEOUT_10S_CYCLES);
            }
            break;

        case UNLOCKED_WAITCLOSE:
            // Transitions:
            // 1. Door Opens again -> DoorOpen
            if (gInputState.doorSensor == 0)
            {
                gSystemState.currentState = UNLOCKED_DOOROPEN;
                setTimer(UNLOCK_WINDOW_ID, TIMEOUT_30S_CYCLES);
            }
            // 2. Timeout 10s -> Relock
            else if (timer_flag[UNLOCK_WINDOW_ID] == 1)
            {
                gSystemState.currentState = LOCKED_RELOCK;
            }
            break;

        case UNLOCKED_ALWAYSOPEN:
            // Transitions:
            // 1. Door Closes -> WaitClose (As per user logic)
            if (gInputState.doorSensor == 1)
            {
                gSystemState.currentState = UNLOCKED_WAITCLOSE;
                setTimer(UNLOCK_WINDOW_ID, TIMEOUT_10S_CYCLES);
            }
            break;

        case LOCKED_RELOCK:
            // Logic: Wait 3s then Sleep
            // Note: Reuse WARNING timer for 3s delay
            if (timer_flag[WARNING_TASK_ID] == 0 && timer_counter[WARNING_TASK_ID] == 0)
            {
                // First entry into state: Set timer
                 setTimer(WARNING_TASK_ID, TIMEOUT_3S_CYCLES);
            }
            else if (timer_flag[WARNING_TASK_ID] == 1)
            {
                gSystemState.currentState = LOCKED_SLEEP;
            }
            break;

        default:
            gSystemState.currentState = LOCKED_SLEEP;
            break;
    }
}

// --- API Implementation ---
bool State_SetPassword(const char *newPass) {
    if (strlen(newPass) != PASSWORD_LENGTH) return false;
    strcpy(gPassword, newPass);
    return true;
}
