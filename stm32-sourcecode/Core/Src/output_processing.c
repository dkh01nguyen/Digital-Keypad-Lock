/*
 * output_processing.c
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 * Description: Applies the state machine's output requests (gOutputStatus)
 * to physical actuators (LEDs, Solenoid, Buzzer) and the LCD.
 */
#include "output_processing.h"

// Buffer for LCD comparison (prevents redundant I2C writes)
static char prevLcdLine1[17] = "";
static char prevLcdLine2[17] = "";

/**
 * @brief Helper function to center text on a 16-character LCD line.
 */
static void center_text(char *dest, const char *src)
{
    size_t len = strlen(src);
    size_t pad_left = (16 > len) ? (16 - len) / 2 : 0;
    memset(dest, ' ', 16);
    strncpy(dest + pad_left, src, len);
    dest[16] = '\0';
}

/**
 * @brief Updates the Red and Green status LEDs.
 */
void Output_UpdateLEDs(void){
	// LED_GREEN_Pin (PB3), LED_RED_Pin (PB4)
	HAL_GPIO_WritePin(GPIOB, LED_GREEN_Pin, gOutputStatus.ledGreen ? GPIO_PIN_SET:GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, LED_RED_Pin, gOutputStatus.ledRed ? GPIO_PIN_SET:GPIO_PIN_RESET);
}

/**
 * @brief Updates the Solenoid state (Locked/Unlocked).
 */
void Output_UpdateSolenoid(void){
	// RELAY_Pin (PB1)
	HAL_GPIO_WritePin(GPIOB, RELAY_Pin, gOutputStatus.solenoid? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Updates the Buzzer state (ON/OFF).
 * FSM manages the duration (10s) by setting/resetting gOutputStatus.buzzer.
 */
void Output_UpdateBuzzer(void){
    // BUZZER_Pin (PB0)
    HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, gOutputStatus.buzzer? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Updates the LCD display via I2C only if content has changed.
 */
void Output_UpdateLCD(void){
    // lcd1 is extern from global.h
	if (strcmp(gOutputStatus.lcdLine1, prevLcdLine1) != 0){
		lcd_gotoxy(&lcd1, 0, 0);
		lcd_puts(&lcd1, gOutputStatus.lcdLine1);
		strcpy(prevLcdLine1, gOutputStatus.lcdLine1);
	}
	if (strcmp(gOutputStatus.lcdLine2, prevLcdLine2) != 0){
		lcd_gotoxy(&lcd1, 0, 1);
		lcd_puts(&lcd1, gOutputStatus.lcdLine2);
		strcpy(prevLcdLine2,gOutputStatus.lcdLine2);
	}
}

/**
 * @brief Maps the current FSM state (gSystemState) to required visuals and non-dynamic actuators.
 * This ensures all 14 states have proper display formatting.
 */
static void MapStateToVisuals(void) {
    uint32_t now = HAL_GetTick();
    char temp_line2[17];

    // Default DO: FSM should set these, but this ensures a fallback/reset state
    gOutputStatus.buzzer = BUZZER_OFF;
    gOutputStatus.ledGreen = LED_OFF;
    gOutputStatus.ledRed = LED_OFF;
    gOutputStatus.solenoid = SOLENOID_LOCKED;

    switch (gSystemState.currentState) {

        case LOCKED_SLEEP:
            // DO: Screen OFF, Solenoid Lock, LED OFF
            center_text(gOutputStatus.lcdLine1, " ");
            center_text(gOutputStatus.lcdLine2, " ");
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            break;

        case LOCKED_WAKEUP:
            // DO: LED Red ON, Solenoid Lock. (Brief display)
            gOutputStatus.ledRed = LED_ON;
            center_text(gOutputStatus.lcdLine1, "LOCK WAKE UP");
            center_text(gOutputStatus.lcdLine2, "Checking Batt...");
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            break;

        case BATTERY_WARNING:
            // DO: LED Red ON (3s warning)
            gOutputStatus.ledRed = LED_ON;
            center_text(gOutputStatus.lcdLine1, "PIN YEU");
            center_text(gOutputStatus.lcdLine2, "CANH BAO (3S)");
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            break;

        case LOCKED_ENTRY:
            // DO: LED Red ON, Display input mask
            gOutputStatus.ledRed = LED_ON;
            strcpy(gOutputStatus.lcdLine1, "ENTER PASSWORD: ");

            // Display * mask starting at column 9 (index 8)
            int len = strlen(inputBuffer);
            memset(temp_line2, ' ', 16);
            memset(temp_line2 + 8, '*', len);
            temp_line2[16] = '\0';
            strcpy(gOutputStatus.lcdLine2, temp_line2);
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            break;

        case LOCKED_VERIFY:
            // DO: LED Red ON (Transient state display)
            gOutputStatus.ledRed = LED_ON;
            center_text(gOutputStatus.lcdLine1, "XAC THUC...");
            center_text(gOutputStatus.lcdLine2, "");
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            break;

        case PENALTY_TIMER:
            // DO: LED Red ON, Buzzer ON (managed by FSM timer), Solenoid Lock
            gOutputStatus.ledRed = LED_ON;
            gOutputStatus.buzzer = BUZZER_ON;
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            center_text(gOutputStatus.lcdLine1, "CAM NHAP LIEU!");

            // Display dynamic countdown (long penalty)
            if (gSystemTimers.penaltyEndTick != 0 && gSystemTimers.penaltyEndTick != UINT32_MAX) {
                int32_t remaining_ms = (int32_t)(gSystemTimers.penaltyEndTick - now);
                if (remaining_ms > 0) {
                    uint32_t remaining_s = remaining_ms / 1000;
                    uint32_t minutes = remaining_s / 60;
                    uint32_t seconds = remaining_s % 60;
                    snprintf(temp_line2, 17, "CHO %02lu:%02lu PHUT", minutes, seconds);
                    center_text(gOutputStatus.lcdLine2, temp_line2);
                } else {
                    center_text(gOutputStatus.lcdLine2, "THU LAI...");
                }
            }
            break;

        case PERMANENT_LOCKOUT:
            // DO: LED Red ON, Buzzer ON, Solenoid Lock
            gOutputStatus.ledRed = LED_ON;
            gOutputStatus.buzzer = BUZZER_ON;
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            center_text(gOutputStatus.lcdLine1, "KHOA VINH VIEN");
            center_text(gOutputStatus.lcdLine2, "DUNG CHIA CO!");
            break;

        case UNLOCKED_WAITOPEN:
            // DO: Solenoid Unlock, LED Green ON (10s window)
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;
            center_text(gOutputStatus.lcdLine1, "XAC THUC THANH CONG");
            center_text(gOutputStatus.lcdLine2, "CHO MO CUA (10S)");
            break;

        case UNLOCKED_SETPASSWORD:
            // DO: Solenoid Unlock, LED Green ON, Display new input
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;
            strcpy(gOutputStatus.lcdLine1, "MAT KHAU MOI:");

            center_text(temp_line2, inputBuffer);
            strcpy(gOutputStatus.lcdLine2, temp_line2);
            break;

        case UNLOCKED_DOOROPEN:
            // DO: Solenoid Unlock, LED Green ON (30s window)
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;
            center_text(gOutputStatus.lcdLine1, "CUA DANG MO");
            center_text(gOutputStatus.lcdLine2, "30S CHO DONG");
            break;

        case ALARM_FORGOTCLOSE:
            // DO: Solenoid Unlock, LED Green ON, Buzzer ON
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;
            gOutputStatus.buzzer = BUZZER_ON;
            center_text(gOutputStatus.lcdLine1, "QUEN DONG CUA!");
            center_text(gOutputStatus.lcdLine2, "CANH BAO (5 PHUT)");
            break;

        case UNLOCKED_WAITCLOSE:
            // DO: Solenoid Unlock, LED Green ON (10s relock timer)
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;
            center_text(gOutputStatus.lcdLine1, "CUA DA DONG");

            // Hiển thị thời gian khóa lại (sử dụng UNLOCK_WINDOW_ID timer counter)
            if (timer_counter[UNLOCK_WINDOW_ID] > 0) {
                uint32_t seconds = timer_counter[UNLOCK_WINDOW_ID] * TIMER_CYCLE / 1000;
                snprintf(temp_line2, 17, "KHOA LAI SAU %2lu S", seconds);
                center_text(gOutputStatus.lcdLine2, temp_line2);
            } else {
                 center_text(gOutputStatus.lcdLine2, "DANG KHOA LAI...");
            }
            break;

        case UNLOCKED_ALWAYSOPEN:
            // DO: Solenoid Unlock, LED Green ON
            gOutputStatus.solenoid = SOLENOID_UNLOCKED;
            gOutputStatus.ledGreen = LED_ON;
            center_text(gOutputStatus.lcdLine1, "CUA LUON MO");
            center_text(gOutputStatus.lcdLine2, "NHAN GIU NUT DE KHOA");
            break;

        case LOCKED_RELOCK:
            // DO: Solenoid Lock, LED Red ON (3s duration)
            gOutputStatus.solenoid = SOLENOID_LOCKED;
            gOutputStatus.ledRed = LED_ON;
            center_text(gOutputStatus.lcdLine1, "DANG KHOA LAI...");
            center_text(gOutputStatus.lcdLine2, "");
            break;

        default:
            break;
    }
}

/**
 * @brief Initializes output processing. Sets initial state for all actuators.
 */
void Output_Init(void){
    // Initialize GPIOs to their default OFF/RESET state
	HAL_GPIO_WritePin(GPIOB, LED_GREEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, LED_RED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, RELAY_Pin, GPIO_PIN_RESET);

	// Set initial display state (used before FSM starts)
	strcpy(gOutputStatus.lcdLine1, "System Init...");
	strcpy(gOutputStatus.lcdLine2, " ");
	Output_UpdateLCD();
}

/**
 * @brief Periodic output processing routine (e.g., every 20 ms).
 * Applies gOutputStatus to hardware: LEDs, solenoid, buzzer, LCD.
 */
void Output_Process(){
	MapStateToVisuals();
	Output_UpdateLEDs();
	Output_UpdateBuzzer();
	Output_UpdateLCD();
	Output_UpdateSolenoid();
}
