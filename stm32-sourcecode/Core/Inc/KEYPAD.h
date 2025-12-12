#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include <stdint.h>
#include "main.h"
#include "global.h"


void Keypad_Init(Keypad_HandleTypeDef* KEYPAD, char KEYMAP[NUMROWS][NUMCOLS],
										GPIO_TypeDef* COL1_PORT, uint32_t COL1_PIN,
										GPIO_TypeDef* COL2_PORT, uint32_t COL2_PIN,
										GPIO_TypeDef* COL3_PORT, uint32_t COL3_PIN,
										GPIO_TypeDef* COL4_PORT, uint32_t COL4_PIN,
										GPIO_TypeDef* ROW1_PORT, uint32_t ROW1_PIN,
										GPIO_TypeDef* ROW2_PORT, uint32_t ROW2_PIN,
										GPIO_TypeDef* ROW3_PORT, uint32_t ROW3_PIN,
										GPIO_TypeDef* ROW4_PORT, uint32_t ROW4_PIN);

char Keypad_Readkey(Keypad_HandleTypeDef* KEYPAD);

#endif /* __KEYPAD_H__ */
