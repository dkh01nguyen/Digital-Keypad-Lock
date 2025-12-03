/*
 * output_processing.c
 *
 *  Created on: Nov 20, 2025
 *      Author: nguye
 */
#include "output_processing.h"
#include "global.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "i2c_lcd.h"   /* your lcd functions */
#include <string.h>

/* NOTE: set this I2C handle to your actual I2C instance if you want LCD working.
   Example: in main.c after MX_I2C_Init() call, set lcd.hi2c = &hi2c1;
*/
static I2C_LCD_HandleTypeDef lcd_handle;
static I2C_LCD_HandleTypeDef *plcd = NULL;
extern I2C_HandleTypeDef hi2c1;

/* internal shadow to detect changes */
static char last_line1[17];
static char last_line2[17];

void Output_Init(void)
{
    /* initialize shadow buffers */
    memset(last_line1, 0, sizeof(last_line1));
    memset(last_line2, 0, sizeof(last_line2));

    /* default outputs */
    gOutputStatus.ledGreen = LED_OFF;
    gOutputStatus.ledRed = LED_OFF;
    gOutputStatus.buzzer = BUZZER_OFF;
    gOutputStatus.solenoid = SOLENOID_LOCKED;
    memset(gOutputStatus.lcdLine1, 0, sizeof(gOutputStatus.lcdLine1));
    memset(gOutputStatus.lcdLine2, 0, sizeof(gOutputStatus.lcdLine2));

    plcd = &lcd_handle;
    plcd->hi2c = &hi2c1;
    plcd->address = (0x27 << 1);
    lcd_init(plcd);
}

static void apply_leds(void)
{
    /* your board has LEED_GREEN_Pin (note spelling) and LED_RED_Pin */
    HAL_GPIO_WritePin(LEED_GREEN_GPIO_Port, LEED_GREEN_Pin, (gOutputStatus.ledGreen == LED_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, (gOutputStatus.ledRed == LED_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void apply_solenoid(void)
{
    /* Relay pin used to drive solenoid (active high to unlock?) */
    if (gOutputStatus.solenoid == SOLENOID_UNLOCKED) {
        HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_RESET);
    }
}

static void apply_buzzer(void)
{
    if (gOutputStatus.buzzer == BUZZER_ON) {
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
    }
}

static void apply_lcd(void)
{
    /* If no LCD configured (plcd == NULL) skip */
    if (plcd == NULL) return;

    /* update only when content changed */
    if (strncmp(last_line1, (char*)gOutputStatus.lcdLine1, 16) != 0) {
        lcd_gotoxy(plcd, 0, 0);
        lcd_puts(plcd, (char*)gOutputStatus.lcdLine1);
        strncpy(last_line1, (char*)gOutputStatus.lcdLine1, 16);
        last_line1[16] = '\0';
    }
    if (strncmp(last_line2, (char*)gOutputStatus.lcdLine2, 16) != 0) {
        lcd_gotoxy(plcd, 0, 1);
        lcd_puts(plcd, (char*)gOutputStatus.lcdLine2);
        strncpy(last_line2, (char*)gOutputStatus.lcdLine2, 16);
        last_line2[16] = '\0';
    }
}

void Output_Process(void)
{
    apply_leds();
    apply_solenoid();
    apply_buzzer();
    apply_lcd();
}


