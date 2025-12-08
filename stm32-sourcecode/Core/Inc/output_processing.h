/*
 * output_processing.h
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 */

#ifndef INC_OUTPUT_PROCESSING_H_
#define INC_OUTPUT_PROCESSING_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "global.h"
#include "i2c_lcd.h"
#include "main.h" // Needed for GPIO defines
#include "timer.h" // Needed for timer access

/**
 * @brief Initializes output processing.
 * Sets initial state for all actuators (OFF/RESET).
 */
void Output_Init(void);

/**
 * @brief Periodic output processing routine (e.g., every 20 ms).
 * Applies the desired state from gOutputStatus to hardware components.
 */
void Output_Process(void);

#endif /* INC_OUTPUT_PROCESSING_H_ */
