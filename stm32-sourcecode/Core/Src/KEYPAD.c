/*
 * keypad.c
 *
 * Created on: Nov 20, 2025
 * Author: nguye
 * Description: 3x4 Keypad scanning with debounce and double-click (1-6 to A-F) logic.
 */

#include "keypad.h"
#include "global.h" // For KEYPAD_DOUBLE_TIMEOUT, keypad_flags

// Key mapping constants
static const char keypad_single_char[KEYPAD_KEYS] = {
	'1','2','3',
	'4','5','6',
	'7','8','9',
	'*','0','#'
};

// Double-click conversion (1-6 to A-F)
static const char keypad_double_char[KEYPAD_KEYS] = {
	'A','B','C',
	'D','E','F',
	'7','8','9',
	'*','0','#'
};

// Debounce registers (4 stages)
static uint8_t reg0[KEYPAD_KEYS];
static uint8_t reg1[KEYPAD_KEYS];
static uint8_t reg2[KEYPAD_KEYS];
static uint8_t reg3[KEYPAD_KEYS];

// Debounced key state (1 = Pressed, 0 = Released)
static uint8_t key_state_buffer[KEYPAD_KEYS];

// Double-click logic variables
static int8_t active_key = -1;			// Index of the key currently being pressed
static uint8_t press_count = 0;			// 1 (single press) or 2 (double press)
static uint16_t double_timer = 0;		// Countdown for 350ms double-click window
static char preview_char = 0;           // Temporary character being previewed

//  Helper Functions

// Calculates the linear index from row and column
static inline uint8_t KeyIndex(uint8_t r, uint8_t c)
{
	return r * KEYPAD_COLS + c;
}

/**
 * @brief Initializes the Keypad structure and internal states.
 */
void Keypad_Init(Keypad_HandleTypeDef *k,
				GPIO_TypeDef *C1_Port, uint16_t C1_Pin,
				GPIO_TypeDef *C2_Port, uint16_t C2_Pin,
				GPIO_TypeDef *C3_Port, uint16_t C3_Pin,
				GPIO_TypeDef *R1_Port, uint16_t R1_Pin,
				GPIO_TypeDef *R2_Port, uint16_t R2_Pin,
				GPIO_TypeDef *R3_Port, uint16_t R3_Pin,
				GPIO_TypeDef *R4_Port, uint16_t R4_Pin)
{
    // Map GPIO ports and pins to the structure
	k->COL_Port[0] = C1_Port; k->COL_Pin[0] = C1_Pin;
	k->COL_Port[1] = C2_Port; k->COL_Pin[1] = C2_Pin;
	k->COL_Port[2] = C3_Port; k->COL_Pin[2] = C3_Pin;

	k->ROW_Port[0] = R1_Port; k->ROW_Pin[0] = R1_Pin;
	k->ROW_Port[1] = R2_Port; k->ROW_Pin[1] = R2_Pin;
	k->ROW_Port[2] = R3_Port; k->ROW_Pin[2] = R3_Pin;
	k->ROW_Port[3] = R4_Port; k->ROW_Pin[3] = R4_Pin;

	// Reset internal state variables
	active_key = -1;
	press_count = 0;
	double_timer = 0;
	preview_char = 0;
    // Reset global flags (defined in global.h)
	keypad_char_ready = 0;
}

/**
 * @brief Scans the keypad matrix, performs debouncing, and handles double-click logic.
 * Must be called periodically (10ms).
 */
void Keypad_Scan(Keypad_HandleTypeDef *k)
{
	uint8_t detected = 0xFF; // Index of the newly detected key press

	for (uint8_t row = 0; row < KEYPAD_ROWS; row++)
	{
		// Set current ROW LOW (active)
		for (uint8_t r = 0; r < KEYPAD_ROWS; r++)
			HAL_GPIO_WritePin(k->ROW_Port[r], k->ROW_Pin[r],
				(r == row ? GPIO_PIN_RESET : GPIO_PIN_SET)); // <<< ĐIỀU CHỈNH LOGIC >>>

		for (uint8_t col = 0; col < KEYPAD_COLS; col++)
		{
			uint8_t i = KeyIndex(row, col);

			// Shift debounce registers
			reg0[i] = reg1[i];
			reg1[i] = reg2[i];
			reg2[i] = HAL_GPIO_ReadPin(k->COL_Port[col], k->COL_Pin[col]); // <<< ĐỌC CỘT >>>

			if (reg0[i] == reg1[i] && reg1[i] == reg2[i])
			{
				// Update debounced key state
				key_state_buffer[i] = (reg2[i] == GPIO_PIN_RESET) ? 1 : 0; // LOW = Pressed

				// Edge detection (Key pressed)
				if (reg3[i] != reg2[i])
				{
					reg3[i] = reg2[i];

					if (reg2[i] == GPIO_PIN_RESET)
						detected = i; // Store index of the new press
				}
			}
		}
	}

	// Set all ROWs HIGH (safe state)
	for (uint8_t r = 0; r < KEYPAD_ROWS; r++)
		HAL_GPIO_WritePin(k->ROW_Port[r], k->ROW_Pin[r], GPIO_PIN_SET);

	// Double-Click Timer Logic
	if (double_timer > 0)
	{
		double_timer--;
		if (double_timer == 0 && active_key >= 0)
		{
			// Timer expired: Finalize the character (single or double)
			keypad_last_char = (press_count == 1)
								? keypad_single_char[active_key]
								: keypad_double_char[active_key];

			keypad_char_ready = 1;

            // Reset double-click state
			active_key = -1;
			press_count = 0;
			preview_char = 0;
			keypad_preview_char = 0;
		}
	}

	// Handle Key Press Event
	if (detected != 0xFF)
	{
		if (detected == active_key)
		{
			// Second press on the same key (Double Click)
			press_count = 2;
			double_timer = KEYPAD_DOUBLE_TIMEOUT;
			preview_char = keypad_double_char[active_key];
			keypad_preview_char = preview_char;
		}
		else
		{
			// New key pressed (or different key pressed)
			if (active_key >= 0)
			{
				// Finalize the previous pending character immediately
				keypad_last_char = (press_count == 1)
									? keypad_single_char[active_key]
									: keypad_double_char[active_key];
				keypad_char_ready = 1;
			}

			// Start new press cycle
			active_key = detected;
			press_count = 1;
			preview_char = keypad_single_char[detected];
			keypad_preview_char = preview_char;
			double_timer = KEYPAD_DOUBLE_TIMEOUT;
		}
	}
}


/**
 * @brief Checks the current debounced physical state of a key.
 * @param keyChar The character to check ('0'-'9', '*', '#').
 * @return 1 if the key is currently pressed, 0 otherwise.
 */
uint8_t Keypad_IsPressed(char keyChar)
{
    // Find index of the key character
    for (int i = 0; i < KEYPAD_KEYS; i++) {
        if (keypad_single_char[i] == keyChar) {
            return key_state_buffer[i];
        }
    }
    return 0;
}


// API User Functions

/**
 * @brief Retrieves the final (debounced and resolved double-click) character.
 * @return The finalized character (0 if none available).
 */
char Keypad_GetChar(void)
{
	if (keypad_char_ready)
	{
		keypad_char_ready = 0;
		return keypad_last_char;
	}
	return 0;
}

/**
 * @brief Retrieves the temporary character currently being previewed.
 * @return The preview character.
 */
char Keypad_GetPreview(void)
{
	return keypad_preview_char;
}

/**
 * @brief Forces the current pending character to finalize immediately.
 */
void Keypad_ForceFinalize(void)
{
	if (active_key >= 0)
	{
		keypad_last_char = (press_count == 1)
					? keypad_single_char[active_key]
					: keypad_double_char[active_key];

		keypad_char_ready = 1;

		active_key = -1;
		press_count = 0;
		double_timer = 0;
		preview_char = 0;
		keypad_preview_char = 0;
	}
}
