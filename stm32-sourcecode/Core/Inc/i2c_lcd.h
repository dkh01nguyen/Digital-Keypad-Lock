/*
 * i2c_lcd.h
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 */

#ifndef INC_I2C_LCD_H_
#define INC_I2C_LCD_H_

#include <stdint.h>

/**
 * @brief Includes the HAL driver present in the project
 */
#if __has_include("stm32f1xx_hal.h")
	#include "stm32f1xx_hal.h"
#elif __has_include("stm32c0xx_hal.h")
	#include "stm32c0xx_hal.h"
#elif __has_include("stm32g4xx_hal.h")
	#include "stm32g4xx_hal.h"
#endif

/**
 * @brief Structure to hold LCD instance information
 */
typedef struct {
	I2C_HandleTypeDef *hi2c;    // I2C handler for communication
	uint8_t address;            // I2C address of the LCD
} I2C_LCD_HandleTypeDef; // <<< ĐÃ THÊM ĐỊNH NGHĨA STRUCT >>>


/**
 * @brief Initializes the LCD.
 * @param lcd: Pointer to the LCD handle
 */
void lcd_init(I2C_LCD_HandleTypeDef *lcd);

/**
 * @brief Sends a command to the LCD.
 * @param lcd: Pointer to the LCD handle
 * @param cmd: Command byte to send
 */
void lcd_send_cmd(I2C_LCD_HandleTypeDef *lcd, char cmd);

// ... (Các khai báo hàm API khác giữ nguyên) ...
void lcd_send_data(I2C_LCD_HandleTypeDef *lcd, char data);
void lcd_putchar(I2C_LCD_HandleTypeDef *lcd, char ch);
void lcd_puts(I2C_LCD_HandleTypeDef *lcd, char *str);
void lcd_gotoxy(I2C_LCD_HandleTypeDef *lcd, int col, int row);
void lcd_clear(I2C_LCD_HandleTypeDef *lcd);

#endif /* INC_I2C_LCD_H_ */
