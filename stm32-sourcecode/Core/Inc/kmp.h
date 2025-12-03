/*
 * kmp.h
 *
 *  Created on: Dec 3, 2025
 *      Author: nguye
 */

#ifndef INC_KMP_H_
#define INC_KMP_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "global.h"
#include "stm32f1xx_hal.h"

// Configuration
#define MAX_INPUT_LENGTH 20
#define PASSWORD_LENGTH 4

// Password database (can be stored in flash)
extern const char VALID_PASSWORD[PASSWORD_LENGTH];

void KMP_BuildLPS(const uint8_t *pattern, uint16_t *lps);
bool KMP_FindPassword(const uint8_t *input, uint16_t length, const uint8_t *password);

#endif /* INC_KMP_H_ */
