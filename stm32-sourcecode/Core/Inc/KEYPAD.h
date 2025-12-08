#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include <stdint.h>
#include "main.h"
#include "global.h"

// Keypad initialization API
void Keypad_Init(Keypad_HandleTypeDef *keypad,
				GPIO_TypeDef *C1_Port, uint16_t C1_Pin,
				GPIO_TypeDef *C2_Port, uint16_t C2_Pin,
				GPIO_TypeDef *C3_Port, uint16_t C3_Pin,
				GPIO_TypeDef *R1_Port, uint16_t R1_Pin,
				GPIO_TypeDef *R2_Port, uint16_t R2_Pin,
				GPIO_TypeDef *R3_Port, uint16_t R3_Pin,
				GPIO_TypeDef *R4_Port, uint16_t R4_Pin);

/**
 * @brief Periodic function to scan the keypad matrix.
 * Must be called every KEYPAD_SCAN_PERIOD_MS (10ms).
 */
void Keypad_Scan(Keypad_HandleTypeDef *keypad);

/**
 * @brief Retrieves the final (debounced and resolved double-click) character.
 * @return The finalized character (0 if none available).
 */
char Keypad_GetChar(void);

/**
 * @brief Retrieves the temporary character currently being pressed or previewed.
 * @return The preview character.
 */
char Keypad_GetPreview(void);

/**
 * @brief Forces the current pending character to finalize immediately.
 */
void Keypad_ForceFinalize(void);

/**
 * @brief Checks the current debounced physical state of a key.
 * @param keyChar The character to check ('0'-'9', '*', '#').
 * @return 1 if the key is currently pressed (hardware level), 0 otherwise.
 */
uint8_t Keypad_IsPressed(char keyChar);

#endif /* __KEYPAD_H__ */
