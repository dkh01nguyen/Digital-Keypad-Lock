/*
 * state_processing.c
 *
 *  Created on: Nov 20, 2025
 *      Author: nguye
 */
#include "state_processing.h"
#include "global.h"
#include "kmp.h"
#include "i2c_lcd.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>


/* penalty durations in minutes (sequence 1, 5, 25, 125) */
static const uint32_t penalty_minutes[] = { 1, 5, 25, 125 };
static const uint8_t penalty_levels = sizeof(penalty_minutes) / sizeof(penalty_minutes[0]);

/* entry buffer settings */
#define ENTRY_BUFFER_MAX  64  /* allow 4..20 per requirements; but keep room */

/* internal buffers */
static char entryBuffer[ENTRY_BUFFER_MAX + 1];
static uint16_t entryLen;

/* helper to set LCD lines conveniently */
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

/* helper to start penalty for current penaltyLevel */
static void start_penalty(uint8_t level)
{
    if (level == 0) return;
    uint32_t minutes = penalty_minutes[(level - 1) < penalty_levels ? (level - 1) : (penalty_levels - 1)];
    uint32_t ms = minutes * 60U * 1000U;
    gSystemTimers.penaltyLevel = level;
    gSystemTimers.penaltyEndTick = HAL_GetTick() + ms;
    /* buzzer initial 10s beep */
    gOutputStatus.buzzer = BUZZER_ON;
    /* TODO: better buzzer pattern handling */
}

/* helper to clear entry */
static void entry_clear(void)
{
    entryLen = 0;
    entryBuffer[0] = '\0';
}

/* helper to append char to entry buffer (if space) */
static void entry_append(char c)
{
    if (entryLen < ENTRY_BUFFER_MAX) {
        entryBuffer[entryLen++] = c;
        entryBuffer[entryLen] = '\0';
    }
}

/* helper to replace last char (used by double-press replace) */
void State_Event_ReplaceLastChar(char newChar)
{
    if (entryLen == 0) {
        entry_append(newChar);
    } else {
        entryBuffer[entryLen - 1] = newChar;
    }
    /* update LCD preview */
    char disp1[17]; memset(disp1, ' ', 16); disp1[16] = '\0';
    for (int i = 0; i < (int)entryLen && i < 16; ++i) disp1[i] = entryBuffer[i];
    setLCD(disp1, "");
}

/* ---------- API implementations ---------- */

void State_Init(void)
{
    gSystemState.currentState = LOCKED_SLEEP;
    gSystemTimers.failedAttempts = 0;
    gSystemTimers.penaltyLevel = 0;
    gSystemTimers.penaltyEndTick = 0;
    gSystemTimers.unlockExpireTick = 0;
    gSystemTimers.alarmRepeatTick = 0;

    /* reset outputs */
    gOutputStatus.solenoid = SOLENOID_LOCKED;
    gOutputStatus.ledRed = LED_ON;
    gOutputStatus.ledGreen = LED_OFF;
    gOutputStatus.buzzer = BUZZER_OFF;
    setLCD("System Ready","");

    entry_clear();
    memset(gPassword, 0, MAX_PASSWORD_LENGTH + 1);
    /* default password can be set elsewhere */
}

/* helper: check battery (simple threshold) */
static void check_battery(void)
{
    /* TODO: implement ADC reading mapping; for now use gInputState.batteryADC */
    if (gInputState.batteryADC < 1000) {
        gInputState.batteryLow = 1;
    } else {
        gInputState.batteryLow = 0;
    }
}

/* Verify entry buffer against password using KMP: password must appear continuous */
static bool verify_password()
{
    size_t len = entryLen;
    if (len < 4 || len > 20) return false; /* callers should check format first */
    return KMP_FindPassword((const uint8_t*)entryBuffer, (uint16_t)len, (const uint8_t*)gPassword);
}

/* Called every 10ms */
void State_Process(void)
{
    uint32_t now = HAL_GetTick();

    /* Handle penalty expiration */
    if (gSystemTimers.penaltyEndTick != 0 && gSystemTimers.penaltyEndTick != UINT32_MAX) {
        if ((int32_t)(gSystemTimers.penaltyEndTick - now) <= 0) {
            /* penalty ended */
            gSystemTimers.penaltyLevel = 0;
            gSystemTimers.penaltyEndTick = 0;
            gOutputStatus.buzzer = BUZZER_OFF;
        }
    }

    switch (gSystemState.currentState) {
        case LOCKED_SLEEP:
            /* Do: screen off, solenoid locked. Wait for input. */
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_ON;
            gOutputStatus.ledGreen = LED_OFF;
            /* If keypad input or indoor key sensor (override), wake up */
            if (gKeyEvent.keyChar != 0 || gInputState.indoorButton || gInputState.keySensor) {
                gSystemState.currentState = LOCKED_WAKEUP;
            }
            break;

        case LOCKED_WAKEUP:
            /* Do: show screen, red LED, check battery */
            setLCD("Enter password","");
            check_battery();
            if (gInputState.batteryLow) {
                gSystemState.currentState = BATTERY_WARNING;
                /* record start time */
                gSystemTimers.unlockExpireTick = now + 3000; /* show for 3s */
            } else {
                gSystemState.currentState = LOCKED_ENTRY;
                /* prepare entry */
                entry_clear();
                setLCD("Enter password","");
            }
            break;

        case BATTERY_WARNING:
            /* show warning for 3s */
            setLCD("Battery Low!","Please replace");
            if ((int32_t)(gSystemTimers.unlockExpireTick - now) <= 0) {
                gSystemState.currentState = LOCKED_ENTRY;
                entry_clear();
                setLCD("Enter password","");
            }
            break;

        case LOCKED_ENTRY:
            /* Do: show entry screen and capture keys */
            /* Append characters until Enter '#' */
            if (gKeyEvent.keyChar != 0) {
                char k = gKeyEvent.keyChar;
                gKeyEvent.keyChar = 0; /* consume */
                if (k == '*') {
                    /* backspace */
                    if (entryLen > 0) { entryLen--; entryBuffer[entryLen] = '\0'; }
                } else if (k == '#') {
                    /* go to verify */
                    gSystemState.currentState = LOCKED_VERIFY;
                } else {
                    /* append (only allow 0-9, A-F) */
                    if (entryLen < ENTRY_BUFFER_MAX) {
                        entry_append(k);
                    }
                }
                /* update display preview (first line) */
                char disp[17]; memset(disp, ' ', 16); disp[16] = '\0';
                for (int i = 0; i < (int)entryLen && i < 16; ++i) disp[i] = entryBuffer[i];
                setLCD(disp, "");
                /* reset entry timeout? optional */
            }

            /* If no input for 30s -> go back to sleep */
            /* use unlockExpireTick as inactivity timer */
            if (gSystemTimers.unlockExpireTick == 0) {
                gSystemTimers.unlockExpireTick = now + 30000;
            } else if ((int32_t)(gSystemTimers.unlockExpireTick - now) <= 0) {
                gSystemTimers.unlockExpireTick = 0;
                gSystemState.currentState = LOCKED_SLEEP;
                setLCD("",""); /* turn off screen */
            }
            break;

        case LOCKED_VERIFY:
            /* Do: check format length and verify */
            if (entryLen < 4 || entryLen > 20) {
                setLCD("Invalid input","4..20 chars");
                /* do not increment failedAttempts; return to entry */
                /* show for 2s */
                gSystemTimers.unlockExpireTick = now + 2000;
                gSystemState.currentState = LOCKED_ENTRY; /* go back to entry after message */
            } else {
                /* verify using KMP: password must appear continuous in entry */
                if (verify_password()) {
                    /* success */
                    gSystemState.currentState = UNLOCKED_WAITOPEN;
                    gOutputStatus.solenoid = SOLENOID_UNLOCKED;
                    gOutputStatus.ledGreen = LED_ON;
                    gOutputStatus.ledRed = LED_OFF;
                    setLCD("Authentication","Success");
                    /* start 10s open window */
                    gSystemTimers.unlockExpireTick = now + 10000;
                    /* reset failure count */
                    gSystemTimers.failedAttempts = 0;
                } else {
                    /* failure */
                    gSystemTimers.failedAttempts++;
                    /* check if multiple of 3 */
                    if ((gSystemTimers.failedAttempts % 3) == 0) {
                        if (gSystemTimers.failedAttempts >= 15) {
                            /* permanent lockout */
                            gSystemState.currentState = PERMANENT_LOCKOUT;
                            /* buzzer initial 10s */
                            gOutputStatus.buzzer = BUZZER_ON;
                            /* lock keyboard until key used */
                            gSystemTimers.penaltyEndTick = UINT32_MAX; /* represent permanent */
                        } else {
                            /* escalate penalty: compute level = floor(failed/3) */
                            uint8_t level = (uint8_t)(gSystemTimers.failedAttempts / 3);
                            if (level == 0) level = 1;
                            if (level > penalty_levels) level = penalty_levels;
                            start_penalty(level);
                            gSystemState.currentState = PENALTY_TIMER;
                        }
                    } else {
                        /* normal failure: notify and return to entry */
                        char msg1[17];
                        snprintf(msg1, sizeof(msg1), "Wrong! Att:%u", (unsigned)gSystemTimers.failedAttempts);
                        setLCD("Authentication", msg1);
                        /* short delay then back to entry */
                        gSystemTimers.unlockExpireTick = now + 2000;
                        gSystemState.currentState = LOCKED_ENTRY;
                    }
                }
            }
            /* clear entry buffer */
            entry_clear();
            break;

        case PENALTY_TIMER:
            /* Do: disable keyboard, buzzer for 10s initial, show message blocked */
            setLCD("Keypad disabled","Penalty active");
            /* during penalty, keypad input should be ignored (input_processing should not post keys),
               here we monitor if mechanical key used to bypass */
            if (gSystemTimers.penaltyEndTick != 0 && gSystemTimers.penaltyEndTick != UINT32_MAX) {
                if ((int32_t)(gSystemTimers.penaltyEndTick - now) <= 0) {
                    /* penalty expired */
                    gSystemTimers.penaltyLevel = 0;
                    gSystemTimers.penaltyEndTick = 0;
                    gOutputStatus.buzzer = BUZZER_OFF;
                    gSystemState.currentState = LOCKED_ENTRY;
                    setLCD("Enter password","");
                }
            }
            /* allow mechanical override */
            if (gInputState.keySensor) {
                gSystemState.currentState = UNLOCKED_WAITOPEN;
                gOutputStatus.solenoid = SOLENOID_UNLOCKED;
                setLCD("Unlocked","By Key");
            }
            break;

        case PERMANENT_LOCKOUT:
            setLCD("Permanently Locked","Use Mechanical Key");
            gOutputStatus.buzzer = BUZZER_ON; /* initial beep performed earlier */
            /* only exit when mechanical key used */
            if (gInputState.keySensor) {
                gSystemState.currentState = UNLOCKED_WAITOPEN;
                gOutputStatus.solenoid = SOLENOID_UNLOCKED;
                gOutputStatus.buzzer = BUZZER_OFF;
                setLCD("Unlocked","By Key");
            }
            break;

        case UNLOCKED_WAITOPEN:
            /* Solenoid is open, start 10s timer (set earlier) */
            if ((int32_t)(gSystemTimers.unlockExpireTick - now) <= 0) {
                /* auto re-lock */
                gSystemState.currentState = LOCKED_RELOCK;
            }
            /* If door opened (microswitch released) */
            if (gInputState.doorSensor == 0) { /* assuming 0 = open per your wiring */
                gSystemState.currentState = UNLOCKED_DOOROPEN;
                /* start 30s close timer */
                gSystemTimers.unlockExpireTick = now + 30000;
            }
            /* long enter (maybe set password) handled by input events: if '*' or specific long enter use case,
               leave implementation for Unlocked_SetPassword when Enter pressed >1s */
            break;

        case UNLOCKED_SETPASSWORD:
            /* allow user to enter new password (must be exactly 4 characters) */
            /* We'll accept 4-char entry then update gPassword */
            /* For simplicity, reuse entryBuffer: */
            if (gKeyEvent.keyChar != 0) {
                char k = gKeyEvent.keyChar;
                gKeyEvent.keyChar = 0;
                if (k == '#') {
                    /* finalize setting */
                    if (entryLen == 4) {
                        memcpy(gPassword, entryBuffer, 4);
                        gPassword[4] = '\0';
                        setLCD("Password set","Saved");
                        gSystemState.currentState = LOCKED_RELOCK;
                        gOutputStatus.solenoid = SOLENOID_LOCKED;
                    } else {
                        setLCD("Password error","Must be 4 chars");
                    }
                    entry_clear();
                } else if (k == '*') {
                    if (entryLen > 0) { entryLen--; entryBuffer[entryLen] = '\0'; }
                } else {
                    if (entryLen < 20) entry_append(k);
                }
            }
            /* show preview */
            {
                char disp[17]; memset(disp, ' ', 16);
                for (int i = 0; i < (int)entryLen && i < 16; ++i) disp[i] = entryBuffer[i];
                setLCD("Set New Password", disp);
            }
            break;

        case UNLOCKED_DOOROPEN:
            /* door is open, wait for close or timeout */
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;
            if (gInputState.doorSensor == 1) { /* closed */
                /* door closed */
                /* if indoor button pressed > 1s, possibly go to always open (handled elsewhere) */
                gSystemState.currentState = UNLOCKED_WAITCLOSE;
                gSystemTimers.unlockExpireTick = now + 10000; /* 10s to re-lock */
            } else {
                /* still open, check 30s max */
                if ((int32_t)(gSystemTimers.unlockExpireTick - now) <= 0) {
                    /* alarm forgot close */
                    gSystemState.currentState = ALARM_FORGOTCLOSE;
                    gSystemTimers.alarmRepeatTick = now + (5U * 60U * 1000U); /* repeat every 5 minutes */
                }
            }
            break;

        case ALARM_FORGOTCLOSE:
            /* buzzer 10s then repeat every 5 minutes */
            gOutputStatus.buzzer = BUZZER_ON;
            setLCD("Close door!","");
            /* simple: after 10s turn buzzer off; repeat managed by alarmRepeatTick */
            /* Use unlockExpireTick as alarm end tick */
            if (gSystemTimers.alarmRepeatTick != 0 && (int32_t)(gSystemTimers.alarmRepeatTick - now) <= 0) {
                /* beep again and schedule next */
                /* for simplicity, just update next repeat */
                gSystemTimers.alarmRepeatTick = now + (5U * 60U * 1000U);
            }
            if (gInputState.doorSensor == 1) {
                gSystemState.currentState = UNLOCKED_WAITCLOSE;
                gOutputStatus.buzzer = BUZZER_OFF;
            }
            break;

        case UNLOCKED_WAITCLOSE:
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            /* start 10s relock timer */
            if ((int32_t)(gSystemTimers.unlockExpireTick - now) <= 0) {
                gSystemState.currentState = LOCKED_RELOCK;
            }
            if (gInputState.doorSensor == 0) {
                gSystemState.currentState = UNLOCKED_DOOROPEN; /* reopened */
            }
            break;

        case UNLOCKED_ALWAYSOPEN:
            /* keep solenoid unlocked until indoor button pressed >1s again toggles */
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            /* logic for toggling handled via events */
            break;

        case LOCKED_RELOCK:
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_ON;
            gOutputStatus.ledGreen = LED_OFF;
            setLCD("",""); /* turn off after relock */
            gSystemState.currentState = LOCKED_SLEEP;
            break;

        default:
            gSystemState.currentState = LOCKED_SLEEP;
            break;
    } /* end switch */
}

/* Event handlers invoked by input_processing or external interrupts */
void State_Event_KeypadChar(char c)
{
    /* Post char to gKeyEvent for polling by State_Process */
    gKeyEvent.keyChar = c;
    gKeyEvent.isLong = 0;
}

void State_Event_IndoorButton(void)
{
    gInputState.indoorButton = 1;
    /* immediate behaviour: if locked_sleep, wake up */
    if (gSystemState.currentState == LOCKED_SLEEP) {
        gSystemState.currentState = LOCKED_WAKEUP;
    }
}

void State_Event_KeySensor(void)
{
    gInputState.keySensor = 1;
    /* immediate unlock override */
    gSystemState.currentState = UNLOCKED_WAITOPEN;
    gOutputStatus.solenoid = SOLENOID_UNLOCKED;
    setLCD("Unlocked","By Key");
}

void State_Event_DoorSensor_Open(void)
{
    gInputState.doorSensor = 0; /* 0 = open per earlier assumption */
    if (gSystemState.currentState == UNLOCKED_WAITOPEN) {
        gSystemState.currentState = UNLOCKED_DOOROPEN;
        gSystemTimers.unlockExpireTick = HAL_GetTick() + 30000;
    }
}

void State_Event_DoorSensor_Close(void)
{
    gInputState.doorSensor = 1;
    if (gSystemState.currentState == UNLOCKED_DOOROPEN) {
        gSystemState.currentState = UNLOCKED_WAITCLOSE;
        gSystemTimers.unlockExpireTick = HAL_GetTick() + 10000;
    }
}

/* Password API */
bool State_SetPassword(const char *newPass)
{
    if (!newPass) return false;
    size_t len = strlen(newPass);
    if (len < 4 || len > MAX_PASSWORD_LENGTH) return false;
    strncpy(gPassword, newPass, MAX_PASSWORD_LENGTH);
    gPassword[MAX_PASSWORD_LENGTH] = '\0';
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
    setLCD("Force Unlock","Debug");
}





