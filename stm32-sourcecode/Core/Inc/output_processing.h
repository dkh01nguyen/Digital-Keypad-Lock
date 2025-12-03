/*
 * output_processing.h
 *
 *  Created on: Nov 20, 2025
 *      Author: nguye
 */

#ifndef INC_OUTPUT_PROCESSING_H_
#define INC_OUTPUT_PROCESSING_H_

//#include <stdint.h>
//
///**
// * @brief Initialize output processing module.
// *
// * This sets up internal timers for blinking LEDs, buzzer patterns, etc.
// * Call once in main after HAL_Init().
// */
//void Output_Init(void);
//
///**
// * @brief Periodic output update routine.
// *
// * Typically called by scheduler every 10ms.
// * - Reads gOutputStatus
// * - Updates hardware (GPIO, I2C LCD, buzzer)
// * - Handles LED blink, buzzer pattern, LCD throttling
// */
//void Output_Process(void);
//
///**
// * @brief Apply current LED modes (on/off/blink) to GPIO
// */
//void Output_UpdateLEDs(void);
//
///**
// * @brief Apply solenoid state to lock relay
// */
//void Output_UpdateSolenoid(void);
//
///**
// * @brief Apply buzzer mode to GPIO (short/long beeps, patterns)
// */
//void Output_UpdateBuzzer(void);
//
///**
// * @brief Update LCD lines via I2C
// */
//void Output_UpdateLCD(void);

#include <stdint.h>

/**
 * @brief Initialize output processing.
 * - Must be called once after HAL init, and after i2c (if used).
 */
void Output_Init(void);

/**
 * @brief Periodic output processing routine (recommended every 10-50 ms).
 * - Applies gOutputStatus to hardware: LEDs, relay/solenoid, buzzer, LCD.
 */
void Output_Process(void);


#endif /* INC_OUTPUT_PROCESSING_H_ */
