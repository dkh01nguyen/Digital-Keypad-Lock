#include "keypad.h"

// ======== MAP PHÍM ========
// Vị trí 0–11 ứng với:
// row 0: 1 2 3
// row 1: 4 5 6
// row 2: 7 8 9
// row 3: * 0 #
static const char keypad_single_char[KEYPAD_KEYS] = {
    '1','2','3',
    '4','5','6',
    '7','8','9',
    '*','0','#'
};

// Nút 1–6 có double-click A–F
static const char keypad_double_char[KEYPAD_KEYS] = {
    'A','B','C',
    'D','E','F',
    '7','8','9',
    '*','0','#'
};

// ======== Debounce registers ========
static uint8_t reg0[KEYPAD_KEYS];
static uint8_t reg1[KEYPAD_KEYS];
static uint8_t reg2[KEYPAD_KEYS];
static uint8_t reg3[KEYPAD_KEYS];

// ======== Double-click logic ========
static int8_t active_key = -1;         // phím đang nhập
static uint8_t press_count = 0;        // 1 hoặc 2
static uint16_t double_timer = 0;      // timeout đếm lùi
static char preview_char = 0;          // ký tự preview


// ======== Helper ========
static inline uint8_t KeyIndex(uint8_t r, uint8_t c)
{
    return r * KEYPAD_COLS + c;
}

// ===========================================================
//                        INIT
// ===========================================================
void Keypad_Init(Keypad_HandleTypeDef *k,
                 GPIO_TypeDef *C1_Port, uint16_t C1_Pin,
                 GPIO_TypeDef *C2_Port, uint16_t C2_Pin,
                 GPIO_TypeDef *C3_Port, uint16_t C3_Pin,
                 GPIO_TypeDef *R1_Port, uint16_t R1_Pin,
                 GPIO_TypeDef *R2_Port, uint16_t R2_Pin,
                 GPIO_TypeDef *R3_Port, uint16_t R3_Pin,
                 GPIO_TypeDef *R4_Port, uint16_t R4_Pin)
{
    k->COL_Port[0] = C1_Port;  k->COL_Pin[0] = C1_Pin;
    k->COL_Port[1] = C2_Port;  k->COL_Pin[1] = C2_Pin;
    k->COL_Port[2] = C3_Port;  k->COL_Pin[2] = C3_Pin;

    k->ROW_Port[0] = R1_Port;  k->ROW_Pin[0] = R1_Pin;
    k->ROW_Port[1] = R2_Port;  k->ROW_Pin[1] = R2_Pin;
    k->ROW_Port[2] = R3_Port;  k->ROW_Pin[2] = R3_Pin;
    k->ROW_Port[3] = R4_Port;  k->ROW_Pin[3] = R4_Pin;

    active_key = -1;
    press_count = 0;
    double_timer = 0;
    preview_char = 0;
    keypad_char_ready = 0;
}

// ===========================================================
//                        SCAN 10ms
// ===========================================================
void Keypad_Scan(Keypad_HandleTypeDef *k)
{
    uint8_t detected = 0xFF;

    for (uint8_t col = 0; col < KEYPAD_COLS; col++)
    {
        // set 1 cột LOW – 2 cột HIGH
        for (uint8_t c = 0; c < KEYPAD_COLS; c++)
            HAL_GPIO_WritePin(k->COL_Port[c], k->COL_Pin[c],
                (c == col ? GPIO_PIN_RESET : GPIO_PIN_SET));

        // nhỏ delay
        for(volatile uint32_t d = 0; d < 60; d++);

        for (uint8_t row = 0; row < KEYPAD_ROWS; row++)
        {
            uint8_t i = KeyIndex(row, col);

            reg0[i] = reg1[i];
            reg1[i] = reg2[i];
            reg2[i] = HAL_GPIO_ReadPin(k->ROW_Port[row], k->ROW_Pin[row]);

            if (reg0[i] == reg1[i] && reg1[i] == reg2[i])
            {
                if (reg3[i] != reg2[i])
                {
                    reg3[i] = reg2[i];

                    if (reg2[i] == GPIO_PIN_RESET)   // nhấn
                        detected = i;
                }
            }
        }
    }

    // trả cột về HIGH
    for (uint8_t c = 0; c < KEYPAD_COLS; c++)
        HAL_GPIO_WritePin(k->COL_Port[c], k->COL_Pin[c], GPIO_PIN_SET);

    // ================= Double timer =================
    if (double_timer > 0)
    {
        double_timer--;
        if (double_timer == 0 && active_key >= 0)
        {
            keypad_last_char = (press_count == 1)
                                ? keypad_single_char[active_key]
                                : keypad_double_char[active_key];

            keypad_char_ready = 1;

            active_key = -1;
            press_count = 0;
            preview_char = 0;
            keypad_preview_char = 0;
        }
    }

    // ================= Xử lý nhấn phím =================
    if (detected != 0xFF)
    {
        if (detected == active_key)
        {
            press_count = 2;
            double_timer = KEYPAD_DOUBLE_TIMEOUT;
            preview_char = keypad_double_char[active_key];
            keypad_preview_char = preview_char;
        }
        else
        {
            if (active_key >= 0)
            {
                keypad_last_char = (press_count == 1)
                                    ? keypad_single_char[active_key]
                                    : keypad_double_char[active_key];
                keypad_char_ready = 1;
            }

            active_key = detected;
            press_count = 1;
            preview_char = keypad_single_char[detected];
            keypad_preview_char = preview_char;
            double_timer = KEYPAD_DOUBLE_TIMEOUT;
        }
    }
}

// ===========================================================
//                   API User gọi từ ngoài
// ===========================================================
char Keypad_GetChar(void)
{
    if (keypad_char_ready)
    {
        keypad_char_ready = 0;
        return keypad_last_char;
    }
    return 0;
}

char Keypad_GetPreview(void)
{
    return keypad_preview_char;
}

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
