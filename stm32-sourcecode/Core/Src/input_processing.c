/*
 * input_processing.c
 *
 *  Created on: Nov 20, 2025
 *      Author: nguye
 */

#include "input_processing.h"
#include "global.h"
#include "keypad.h"    /* your keypad API */
#include "main.h"
#include "stm32f1xx_hal.h"

/* simple edge detection for discrete inputs if input_reading not available */
static uint8_t prevDoor = 0xFF;
static uint8_t prevKeySensor = 0xFF;
static uint8_t prevIndoor = 0xFF;

/* Init */
void Input_Init(void)
{
    /* clear state */
    gInputState.latestKey = 0;
    gInputState.doorSensor = 0;
    gInputState.keySensor = 0;
    gInputState.indoorButton = 0;
    gInputState.batteryADC = 0;
    gInputState.batteryLow = 0;

    gKeyEvent.keyChar = 0;
    gKeyEvent.isLong = 0;

    /* init prev values to current hardware values (debounced reading assumed in input_reading) */
    prevDoor = (HAL_GPIO_ReadPin(DOOR_SENSOR_GPIO_Port, DOOR_SENSOR_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    prevKeySensor = (HAL_GPIO_ReadPin(KEY_SENSOR_GPIO_Port, KEY_SENSOR_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    prevIndoor = (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) ? 1 : 0;

    /* keypad globals (if used) are set inside keypad.Init */
}

/* Called every 10ms by scheduler */
void Input_Process(void)
{
    /* 1) consume keypad finalized char (preferred) */
    char k = 0;
    /* If your keypad implementation uses Keypad_GetChar(), call it */
    /* Otherwise, if you expose keypad_last_char global, read and clear it */

    /* prefer function if available */
    k = Keypad_GetChar(); /* returns 0 if none */

    if (k != 0) {
        /* place into gInputState and post event to state machine */
        gInputState.latestKey = (uint8_t)k;
        gKeyEvent.keyChar = k;
        gKeyEvent.isLong = 0; /* long-press not implemented for keypad here */
    } else {
        /* clear latestKey if none new - leave last value as historical if desired */
        gInputState.latestKey = 0;
    }

    /* 2) Read discrete sensors and post changes to gInputState (edge detection) */
    uint8_t door = (HAL_GPIO_ReadPin(DOOR_SENSOR_GPIO_Port, DOOR_SENSOR_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    uint8_t keySensor = (HAL_GPIO_ReadPin(KEY_SENSOR_GPIO_Port, KEY_SENSOR_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    uint8_t indoor = (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) ? 1 : 0;

    /* update input state snapshot */
    gInputState.doorSensor = door;
    gInputState.keySensor = keySensor;
    gInputState.indoorButton = indoor;

    /* detect edges if needed by state machine: set gKeyEvent with special codes or let state poll gInputState */
    /* Here we simply let state_processing poll gInputState in State_Process() */
    (void)prevDoor; (void)prevKeySensor; (void)prevIndoor;
}



