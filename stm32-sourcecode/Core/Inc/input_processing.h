/*
 * input_processing.h
 *
 *  Created on: Nov 20, 2025
 *      Author: nguye
 */

#ifndef INC_INPUT_PROCESSING_H_
#define INC_INPUT_PROCESSING_H_

//#include <stdint.h>
//#include <stdbool.h>
//
///**
// * @brief Initialize input processing module. Call once in main.
// *
// * This sets up internal state, counters, debouncing, and prepares
// * key scanning FSM.
// */
//void Input_Init(void);
//
///**
// * @brief Periodic input processing routine.
// *
// * Typically called by scheduler every 10ms.
// * - Scans keypad and buttons
// * - Updates gKeyEvent (or posts events to state module)
// * - Handles long-press, debounce, and keypad lockout
// */
//void Input_Process(void);
//
///**
// * @brief Process a confirmed keypad character.
// * @param c Character pressed ('0'..'9', 'A'..'F', '*', '#')
// * @param isLong 1 if long press, 0 if short press
// *
// * Notes:
// *  - This may update gKeyEvent or directly call State_Event_KeypadChar()
// */
//void Input_KeyEvent(char c, uint8_t isLong);
//
///**
// * @brief Process indoor unlock button pressed
// *
// * Notes:
// *  - May trigger immediate unlock if allowed
// *  - Updates gInputState.indoorButton
// */
//void Input_IndoorButton(void);
//
///**
// * @brief Process mechanical key sensor
// *
// * Notes:
// *  - Updates gInputState.keySensor
// *  - Can trigger override unlock
// */
//void Input_KeySensor(void);
//
///**
// * @brief Process door sensor changes (open/close)
// * @param isOpen 1 = door opened, 0 = door closed
// * Notes:
// *  - Updates gInputState.doorSensor
// *  - Can trigger alarms / state transitions
// */
//void Input_DoorSensor(uint8_t isOpen);


#include <stdint.h>

/**
 * @brief Initialize input processing module.
 * Call once after HAL init and keypad init.
 */
void Input_Init(void);

/**
 * @brief Process inputs, called periodically (recommended every 10 ms).
 *
 * Responsibilities:
 *  - Read finalized keypad characters (Keypad_GetChar or global flags)
 *  - Read discrete sensors (door, key, indoor button) via hardware or gInputState
 *  - Populate gInputState.latestKey and gKeyEvent when a new key arrives
 */
void Input_Process(void);

#endif /* INC_INPUT_PROCESSING_H_ */
