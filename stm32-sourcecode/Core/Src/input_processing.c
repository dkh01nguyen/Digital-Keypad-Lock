/*
 * input_processing.c
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 * Description: Reads all hardware inputs, performs event detection (Long Press, Edges),
 * and updates global state (gInputState, gKeyEvent) for the FSM.
 */

#include "input_processing.h"

// Internal state for Keypad Long Press counting
static uint16_t enter_press_count = 0;
#define LONG_PRESS_CYCLES (1000 / 10) // 100 cycles for 1s (since scheduler runs at 10ms)

// External HAL handles needed for ADC
extern ADC_HandleTypeDef hadc1;

/**
 * @brief Performs ADC conversion and updates gInputState.
 */
static void read_battery_adc(void) {
    if (HAL_ADC_Start(&hadc1) == HAL_OK) {
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            uint16_t adc_value = HAL_ADC_GetValue(&hadc1);

            // LOGIC KIỂM TRA NGƯỠNG ADC ĐƯỢC THỰC HIỆN TẠI ĐÂY (INPUT LAYER)
            if (adc_value < 1000) {
                gInputState.batteryLow = 1;
            } else {
                gInputState.batteryLow = 0;
            }
        }
    }
    HAL_ADC_Stop(&hadc1);
}

/**
 * @brief Initializes the input processing module.
 */
void Input_Init(void)
{
    // Reset Input states
    gInputState.doorSensor = 0;
    gInputState.keySensor = 0;
    gInputState.indoorButton = 0;
    gInputState.batteryLow = 0;

    // Reset Key Event Mailbox
    gKeyEvent.keyChar = 0;
    gKeyEvent.isLong = 0;

    enter_press_count = 0;
}

/**
 * @brief Periodic input processing routine.
 */
void Input_Process(void)
{
    uint8_t current_state = gSystemState.currentState;

    // --- 1. Read ADC and Buttons ---

    read_battery_adc();

    // Read debounced states from input_reading.c
    uint8_t door_curr = is_button_pressed(DOOR_SENSOR_INDEX);
    uint8_t key_curr = is_button_pressed(KEY_SENSOR_INDEX);
    uint8_t indoor_long = is_button_pressed_1s(INDOOR_BUTTON_INDEX);

    // --- 2. Keypad Long Press Counter for Enter (#) ---

    if (Keypad_IsPressed('#')) {
        if (enter_press_count < LONG_PRESS_CYCLES) {
            enter_press_count++;
        }
    } else {
        enter_press_count = 0;
    }

    // --- 3. Consume keypad finalized char & Create Event ---

    char k = Keypad_GetChar();

    if (k != 0) {
        // Disable Keypad input if penalized or warning
        if (current_state == BATTERY_WARNING ||
            current_state == PENALTY_TIMER ||
            current_state == PERMANENT_LOCKOUT)
        {
            // Input is ignored (event is not passed to FSM)
        } else {
            // Check for Long Press Enter (#) (Applies only in UNLOCKED_WAITOPEN)
            if (k == '#' && current_state == UNLOCKED_WAITOPEN && enter_press_count >= LONG_PRESS_CYCLES) {
                 State_Event_KeypadChar_Long(k);
            } else {
                 State_Event_KeypadChar(k);
            }
        }
    }

    // --- 4. Discrete Sensor Edge/Long Press Events ---

    // a) Door Sensor (Edge Detection)
    // Edge UP (Released -> Pressed): Door Opens -> Closes
    if (door_curr == 1 && gInputState.doorSensor == 0) {
        State_Event_DoorSensor_Close();
    }
    // Edge DOWN (Pressed -> Released): Door Closes -> Opens
    else if (door_curr == 0 && gInputState.doorSensor == 1) {
        State_Event_DoorSensor_Open();
    }

    // b) Key Sensor (Edge Detection - Press)
    if (key_curr == 1 && gInputState.keySensor == 0) {
        State_Event_KeySensor(); // FSM handles immediate override
    }

    // c) Indoor Button Long Press (Event)
    if (indoor_long == 1 && gInputState.indoorButton == 0) {
        State_Event_IndoorButton_Long(); // FSM handles toggle/transition
    }

    // --- 5. Update gInputState (Polling Snapshot) ---
    // Update snapshot for FSM to use in transitions logic
    gInputState.doorSensor = door_curr;
    gInputState.keySensor = key_curr;
    gInputState.indoorButton = indoor_long; // Store the Long Press flag

    // --- 6. Switch-Case 14 trạng thái (Tách biệt) ---

    switch (current_state) {
        case LOCKED_SLEEP:
        case LOCKED_WAKEUP:
        case BATTERY_WARNING:
        case LOCKED_ENTRY:
        case LOCKED_VERIFY:
        case PENALTY_TIMER:
        case PERMANENT_LOCKOUT:
        case UNLOCKED_WAITOPEN:
        case UNLOCKED_SETPASSWORD:
        case UNLOCKED_DOOROPEN:
        case ALARM_FORGOTCLOSE:
        case UNLOCKED_WAITCLOSE:
        case UNLOCKED_ALWAYSOPEN:
        case LOCKED_RELOCK:
            // All specific input handling (disabling, event creation) is done above.
            // No further action is required here, ensuring module separation.
            break;
        default:
            break;
    }
}
