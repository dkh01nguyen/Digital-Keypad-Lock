/*
 * input_processing.h
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 */

#ifndef INC_INPUT_PROCESSING_H_
#define INC_INPUT_PROCESSING_H_

/**
 * @brief Initialize input processing module.
 * Call once after HAL init and keypad init.
 */
void Input_Init(void);

/**
 * @brief Process inputs, called periodically (recommended every 10 ms).
 *
 * Responsibilities:
 * - Read ADC and update gInputState.batteryLow.
 * - Detect edges/long presses on discrete buttons.
 * - Create Key Events (gKeyEvent) for the FSM.
 */
void Input_Process(void);

#endif /* INC_INPUT_PROCESSING_H_ */
