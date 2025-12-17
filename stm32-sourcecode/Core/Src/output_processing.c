/*
 * output_processing.c
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 * Description: Applies the state machine's output requests (gOutputStatus)
 * to physical actuators (LEDs, Solenoid, Buzzer) and the LCD.
 */
#include "output_processing.h"
#include "i2c_lcd.h"
#include "main.h"
#include "timer.h"
#include <stdio.h>
#include <string.h>

// --- Private Variables ---
static char prevLcdLine1[17] = "";
static char prevLcdLine2[17] = "";

// Variables for Password Masking Logic
static int lastInputLen = 0; // To detect new key presses
#define MASK_TIMEOUT_MS 1000 // 1 second visibility

// --- Helper Functions ---

/**
 * @brief Centers text on a 16-char line.
 */
static void center_text(char *dest, const char *src)
{
    size_t len = strlen(src);
    if (len > 16) len = 16;
    size_t pad_left = (16 - len) / 2;
    memset(dest, ' ', 16);
    memcpy(dest + pad_left, src, len);
    dest[16] = '\0';
}

/**
 * @brief Logic display enter string: Sliding Window (4 chars) + Masking (1s)
 * Output writes to gOutputStatus.lcdLine2 at index 6-9.
 */
static void format_password_display(void)
{
	char displayBuffer[17];

    memset(displayBuffer, ' ', 16); // Fill with spaces
    displayBuffer[16] = '\0';

    int currentLen = strlen(inputBuffer);

    // 1. Detect new character input to restart visibility timer
    if (currentLen > lastInputLen)
    {
        setTimer(MASK_TIMER_ID, MASK_TIMEOUT_MS); // Start 1s timer
    }
    lastInputLen = currentLen;

    // 2. Sliding Window Logic: Determine which 4 characters to show
    // If len <= 4: Show [0]..[len-1]
    // If len > 4:  Show [len-4]..[len-1]
    int startIdx = (currentLen > 4) ? (currentLen - 4) : 0;
    int endIdx = currentLen;
    int windowLen = endIdx - startIdx;

    // 3. Render characters into a temp 4-char buffer
    // Target position on LCD: Index 6, 7, 8, 9 (Center of 16 chars)
    // 012345 6789 012345
    // "      XXXX      "

    int lcdIdx = 6; // Start writing at index 6

    for (int i = 0; i < windowLen; i++) {
        int originalIdx = startIdx + i;
        char charToShow = inputBuffer[originalIdx];

        // Masking Logic:
        // - Older chars are always '*'
        // - The very last char is visible ONLY if MASK_TIMER is running
        if (originalIdx == (currentLen - 1)) {
            // Check if timer is still running (flag == 0 means running)
            if (timer_counter[MASK_TIMER_ID] > 0) {
                // Keep char visible
            } else {
                charToShow = '*';
            }
        } else {
            charToShow = '*';
        }

        displayBuffer[lcdIdx++] = charToShow;
    }

    // Copy to global output
    strcpy(gOutputStatus.lcdLine2, displayBuffer);
}

/**
 * @brief Maps current FSM State to Visuals (LCD Text & LED Colors)
 */
static void MapStateToVisuals(void)
{
    // Default Actuator States (Safety First)
    char tempStr[17];
	gOutputStatus.inLength = strlen(inputBuffer);
    switch (gSystemState.currentState) {
        case LOCKED_SLEEP:
            center_text(gOutputStatus.lcdLine1, " ");
            center_text(gOutputStatus.lcdLine2, " ");
            break;
        case LOCKED_WAKEUP:
            center_text(gOutputStatus.lcdLine1, "LOCK WAKE UP");
            center_text(gOutputStatus.lcdLine2, "Checking...");
            break;
        case BATTERY_WARNING:
            center_text(gOutputStatus.lcdLine1, "WARNING");
            center_text(gOutputStatus.lcdLine2, "LOW BATTERY!");
            break;
        case LOCKED_ENTRY:
            center_text(gOutputStatus.lcdLine1, "ENTER PASSWORD:");
            format_password_display(); // Handle complex masking
            break;
        case LOCKED_VERIFY:
            // Display static error messages based on input buffer analysis
            // The state stays here for 3s (controlled by FSM timer)
            if (strlen(inputBuffer) < (PASSWORD_LENGTH) || strlen(inputBuffer) > (MAX_INPUT_LENGTH)) {
                center_text(gOutputStatus.lcdLine1, "Input string");
                center_text(gOutputStatus.lcdLine2, "format error");
            } else {
                // Wrong password logic
                center_text(gOutputStatus.lcdLine1, "Wrong password");

                // Calculate tries left: n = 3 - (failedAttempts % 3)
                int triesLeft = 3 - (gSystemTimers.failedAttempts % 3);
                sprintf(tempStr, "%d tries left", triesLeft);
                center_text(gOutputStatus.lcdLine2, tempStr);
            }
            break;
        case PENALTY_TIMER:
            center_text(gOutputStatus.lcdLine1, "Lockout warning");
            // Calculate remaining minutes
            if (gSystemTimers.penaltyEndTick > HAL_GetTick()) {
                uint32_t diff = gSystemTimers.penaltyEndTick - HAL_GetTick();
                uint32_t min = (diff / 60000) + 1; // Round up
                sprintf(tempStr, "%lu minutes", min);
                center_text(gOutputStatus.lcdLine2, tempStr);
            } else {
                center_text(gOutputStatus.lcdLine2, "Wait...");
            }
            break;
        case PERMANENT_LOCKOUT:
            center_text(gOutputStatus.lcdLine1, "Use the key");
            center_text(gOutputStatus.lcdLine2, "to unlock!");
            break;
        case UNLOCKED_WAITOPEN:
            center_text(gOutputStatus.lcdLine1, "Successful");
            center_text(gOutputStatus.lcdLine2, "authentication");
            break;
        case UNLOCKED_SETPASSWORD:
            center_text(gOutputStatus.lcdLine1, "New password:");
            format_password_display(); // Use same logic as entry
            break;
        case UNLOCKED_DOOROPEN:
            center_text(gOutputStatus.lcdLine1, "DOOR OPEN");
            center_text(gOutputStatus.lcdLine2, " ");
            break;
        case ALARM_FORGOTCLOSE:
            center_text(gOutputStatus.lcdLine1, "Close door!");
            center_text(gOutputStatus.lcdLine2, "!!!");
            break;
        case UNLOCKED_WAITCLOSE:
            center_text(gOutputStatus.lcdLine1, "Waiting for");
            center_text(gOutputStatus.lcdLine2, "Locking...");
            break;
        case UNLOCKED_ALWAYSOPEN:
            center_text(gOutputStatus.lcdLine1, "The door");
            center_text(gOutputStatus.lcdLine2, "always open!");
            break;
        case LOCKED_RELOCK:
            center_text(gOutputStatus.lcdLine1, "Locking...");
            center_text(gOutputStatus.lcdLine2, " ");
            break;
        default:
            break;
    }
}

// --- Public API ---

void Output_Init(void)
{
    // Hardware Init States
    HAL_GPIO_WritePin(GPIOB, LED_GREEN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_RED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, RELAY_Pin, GPIO_PIN_RESET);

    // LCD Init
    lcd_init(&lcd1);
    lcd_clear(&lcd1);

    // Clear Buffers
    memset(prevLcdLine1, 0, 17);
    memset(prevLcdLine2, 0, 17);
    lastInputLen = 0;
}

void Output_Process(void)
{
	// 1. Determine what to show based on State
	if (timer_counter[DOOR_NOTIFY_TIMER_ID] > 0)
	{
		// Overwrite lcd by notify change state of the door
		char tempStr[17];
		strcpy(gOutputStatus.lcdLine1, "DOOR STATUS:    ");
		if (gInputState.doorSensor == 1)
		{
			strcpy(tempStr, "    CLOSED      ");
		} else {
			strcpy(tempStr, "    OPENED      ");
		}
		strcpy(gOutputStatus.lcdLine2, tempStr);
	} else {
		MapStateToVisuals();
	}

    // 2. Hardware Actuation (LEDs, Buzzer, Solenoid)
    // Controlled directly by gOutputStatus set by FSM (state_processing)
    HAL_GPIO_WritePin(GPIOB, LED_GREEN_Pin, (gOutputStatus.ledGreen == LED_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_RED_Pin, (gOutputStatus.ledRed == LED_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, (gOutputStatus.buzzer == BUZZER_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, RELAY_Pin, (gOutputStatus.solenoid == SOLENOID_UNLOCKED) ? GPIO_PIN_SET : GPIO_PIN_RESET); // Active High or Low depends on Relay module, assuming Active High here

    // 3. LCD Update (Only if changed)
    if (strcmp(gOutputStatus.lcdLine1, prevLcdLine1) != 0) {
        lcd_gotoxy(&lcd1, 0, 0);
        lcd_puts(&lcd1, gOutputStatus.lcdLine1);
        strcpy(prevLcdLine1, gOutputStatus.lcdLine1);
    }

    if (strcmp(gOutputStatus.lcdLine2, prevLcdLine2) != 0) {
        lcd_gotoxy(&lcd1, 0, 1);
        lcd_puts(&lcd1, gOutputStatus.lcdLine2);
        strcpy(prevLcdLine2, gOutputStatus.lcdLine2);
    }
}
