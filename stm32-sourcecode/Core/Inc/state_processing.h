/*
 * state_processing.h
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 */

#ifndef INC_STATE_PROCESSING_H_
#define INC_STATE_PROCESSING_H_

/**
 * @file state_processing.h
 * @brief State machine module (FSM) core logic.
 *
 * Notes:
 * - FSM uses global structures (gInputState, gKeyEvent) for input events.
 * - FSM updates global output status (gOutputStatus) and timers (gSystemTimers, timer_flag).
 */

#include <stdint.h>
#include <stdbool.h>
#include "global.h" // Primary dependency for all shared structs/defines

/* Initialize state machine. Sets initial state to LOCKED_SLEEP. */
void State_Init(void);

/**
 * @brief Periodic handler, the core FSM loop.
 * Must be called by the scheduler every 10ms.
 */
void State_Process(void);

/* --- Event handlers for discrete triggers (called by input_processing) --- */

/**
 * @brief Posts a regular keypad character event to the FSM mailbox.
 */
void State_Event_KeypadChar(char c);

/**
 * @brief Posts a long-press keypad character event (used for Enter '#').
 */
void State_Event_KeypadChar_Long(char c);

/**
 * @brief Posts a long-press event from the Indoor Unlock Button.
 */
void State_Event_IndoorButton_Long(void);

/**
 * @brief Posts an edge event indicating the Mechanical Key was used.
 */
void State_Event_KeySensor(void);

/**
 * @brief Posts an edge event indicating the Door Sensor was released (Door is Open).
 */
void State_Event_DoorSensor_Open(void);

/**
 * @brief Posts an edge event indicating the Door Sensor was pressed (Door is Closed).
 */
void State_Event_DoorSensor_Close(void);

/* --- Password API --- */

/**
 * @brief Sets and saves the new system password (must be PASSWORD_LENGTH).
 */
bool State_SetPassword(const char *newPass);

/**
 * @brief Retrieves the current password.
 */
const char* State_GetPassword(void);

/**
 * @brief Returns the current FSM state.
 */
uint8_t State_GetState(void);

/**
 * @brief Forces the FSM to UNLOCKED_WAITOPEN state (for debugging/testing).
 */
void State_ForceUnlock(void);

#endif /* INC_STATE_PROCESSING_H_ */
