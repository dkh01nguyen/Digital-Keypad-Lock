#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include "main.h"
#include "stdint.h"
#include "global.h"

// ==== API ====

// Khởi tạo keypad
void Keypad_Init(Keypad_HandleTypeDef *keypad,
                 GPIO_TypeDef *C1_Port, uint16_t C1_Pin,
                 GPIO_TypeDef *C2_Port, uint16_t C2_Pin,
                 GPIO_TypeDef *C3_Port, uint16_t C3_Pin,
                 GPIO_TypeDef *R1_Port, uint16_t R1_Pin,
                 GPIO_TypeDef *R2_Port, uint16_t R2_Pin,
                 GPIO_TypeDef *R3_Port, uint16_t R3_Pin,
                 GPIO_TypeDef *R4_Port, uint16_t R4_Pin);

// Gọi mỗi 10ms
void Keypad_Scan(Keypad_HandleTypeDef *keypad);

// Lấy ký tự hoàn chỉnh (trả về 0 nếu chưa có)
char Keypad_GetChar(void);

// Lấy ký tự preview (chưa finalize)
char Keypad_GetPreview(void);

// Bắt buộc finalize (nhấn Enter '#')
void Keypad_ForceFinalize(void);

#endif
