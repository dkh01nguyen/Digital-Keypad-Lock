/*
 * kmp.h
 *
 *  Created on: Dec 3, 2025
 *      Author: nguye
 */

#ifndef INC_KMP_H_
#define INC_KMP_H_

#include <stdint.h>
#include <stdbool.h>

void KMP_BuildLPS(const uint8_t *pattern, uint16_t *lps);
bool KMP_FindPassword(const uint8_t *input, uint16_t length);

#endif /* INC_KMP_H_ */
