/*
 * state_processing.h
 *
 *  Created on: Nov 20, 2025
 *      Author: nguye
 */

#ifndef INC_STATE_PROCESSING_H_
#define INC_STATE_PROCESSING_H_

/**
 * @file state_processing.h
 * @brief State machine module
 *
 * Notes:
 *  - All global variables (e.g., gSystemState, gSystemTimers) are declared in global.h
 *  - Input events come from input_processing (via gKeyEvent or State_Event_* calls)
 *  - Output updates via gOutputStatus
 */

#include <stdint.h>
#include <stdbool.h>

/* Initialize state machine. Call after Input_Init() and Output_Init(). */
void State_Init(void);

/* Periodic handler — call by scheduler every 10ms */
void State_Process(void);

/* Event helpers for external triggers (optional calls from input_processing) */
void State_Event_KeypadChar(char c);      /* immediate event for key char */
void State_Event_IndoorButton(void);
void State_Event_KeySensor(void);
void State_Event_DoorSensor_Open(void);
void State_Event_DoorSensor_Close(void);

/* Password api */
bool State_SetPassword(const char *newPass);
const char* State_GetPassword(void);

/* Query state */
uint8_t State_GetState(void);

/* Debug/force */
void State_ForceUnlock(void);

#endif /* INC_STATE_PROCESSING_H_ */

