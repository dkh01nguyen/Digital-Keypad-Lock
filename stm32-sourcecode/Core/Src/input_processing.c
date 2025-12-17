/*
 * input_processing.c
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 * Description: Reads all hardware inputs, performs event detection (Long Press, Edges),
 * and updates global state (gInputState, gKeyEvent) for the FSM.
 */
#include "input_processing.h"
#include "input_reading.h"
#include "keypad.h"
#include "global.h"

// --- Static variables for edge detection ---
//static char last_keypad_char;
//static uint8_t last_enter_state;
//static uint8_t last_backspace_state;
//static uint8_t last_door_btn_state;

void Input_Init(void) {
    last_keypad_char = 0;
    last_enter_state = 0;
    last_backspace_state = 0;
    last_door_btn_state = 0;
}

void Input_Process(void) {

    // --- Handle Keypad 4x4 (Char Input) ---
    char current_key = Keypad_Readkey(&hKeypad);

    /* Edge Logic for Keypad:
     * 1. current_key must not be 0 (a key is pressed).
     * 2. current_key must differ from last_keypad_char.
     * This handles:
     * - Pressing a key (0 -> 'A'): Trigger 'A'.
     * - Holding 'A' ('A' -> 'A'): Ignored.
     * - Sliding to 'B' without release ('A' -> 'B'): Trigger 'B'.
     */
    if (current_key != 0 && current_key != last_keypad_char)
    {
        gKeyEvent.keyChar = current_key; // Post event
    } else {
        gKeyEvent.keyChar = 0; // Clear event slot
    }
    last_keypad_char = current_key; // Update history

    // --- Handle Enter Button (Edge + Long Press) ---
    uint8_t enter_curr = is_button_pressed(ENTER_BUTTON_INDEX);
    uint8_t enter_long = is_button_pressed_1s(ENTER_BUTTON_INDEX);

    // Reset flags
    gKeyEvent.isEnter = 0;
    gKeyEvent.isEnterLong = 0;

    // Single press detection (Rising Edge: 0 -> 1)
    if (enter_curr == 1 && last_enter_state == 0) {
        gKeyEvent.isEnter = 1;
    }

    // Long press detection (Handled by input_reading timer)
    if (enter_long == 1)
    {
        gKeyEvent.isEnterLong = 1;
    }
    last_enter_state = enter_curr;

    // --- Handle Backspace Button (Edge only) ---
    uint8_t back_curr = is_button_pressed(BACKSPACE_BUTTON_INDEX);
    gKeyEvent.isBackspace = 0;

    // Single press detection (Rising Edge: 0 -> 1)
    if (back_curr == 1 && last_backspace_state == 0)
    {
        gKeyEvent.isBackspace = 1;
    }
    last_backspace_state = back_curr;

    // --- Update System Input State ---
    uint8_t current_door_btn = is_button_pressed(DOOR_SENSOR_INDEX);
	// Edge detective -> Toggle door's state
	if (current_door_btn == 1 && last_door_btn_state == 0) {
		if (gInputState.doorSensor == 0) {
			gInputState.doorSensor = 1;
		} else {
			gInputState.doorSensor = 0;
		}
		// 100 ticks to dislay notify change state
		setTimer(DOOR_NOTIFY_TIMER_ID, 1000);
	}
	last_door_btn_state = current_door_btn;

    // Mechanical Key Sensor
    gInputState.keySensor = is_button_pressed(KEY_SENSOR_INDEX);

    // Indoor Unlock Button
    gInputState.indoorButton = is_button_pressed(INDOOR_BUTTON_INDEX);
    gInputState.indoorButtonLong = is_button_pressed_1s(INDOOR_BUTTON_INDEX);
}
