/*
 * input_reading.c
 *
 *  Created on: Oct 16, 2025
 *      Author: nguye
 */
#include "input_reading.h"



// Debouncing
static GPIO_PinState buttonBuffer[N0_OF_BUTTONS];
//define two buffers for debouncing
static GPIO_PinState debounceButtonBuffer1[N0_OF_BUTTONS];
static GPIO_PinState debounceButtonBuffer2[N0_OF_BUTTONS];
//define a flag for a button pressed more than 1 second.
static uint8_t flagForButtonPress1s[N0_OF_BUTTONS];
//define counter for automatically increasing the value
//after the button is pressed more than 1 second.
static uint16_t counterForButtonPress1s[N0_OF_BUTTONS];


GPIO_PinState read_button(int index)
{
    switch (index)
    {
        case DOOR_SENSOR_INDEX:
        	return HAL_GPIO_ReadPin(DOOR_SENSOR_GPIO_Port, DOOR_SENSOR_Pin);
        case KEY_SENSOR_INDEX:
        	return HAL_GPIO_ReadPin(KEY_SENSOR_GPIO_Port, KEY_SENSOR_Pin);
        case INDOOR_BUTTON_INDEX:
        	return HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin);
        case ENTER_BUTTON_INDEX:
        	return HAL_GPIO_ReadPin(ENTER_GPIO_Port, ENTER_Pin);
        case BACKSPACE_BUTTON_INDEX:
        	return HAL_GPIO_ReadPin(BACKSPACE_GPIO_Port, BACKSPACE_Pin);
        default:
        	return GPIO_PIN_SET;
    }
}


void button_reading(void) {
	for (uint8_t i = 0; i < N0_OF_BUTTONS; i++)
	{
		debounceButtonBuffer2[i] = debounceButtonBuffer1[i];
		debounceButtonBuffer1[i] = read_button(i);
		if (debounceButtonBuffer1[i] == debounceButtonBuffer2[i])
		{
			buttonBuffer[i] = debounceButtonBuffer1[i];
			if (buttonBuffer[i] == BUTTON_IS_PRESSED)
			{
				 //if a button is pressed, we start counting
				 if(counterForButtonPress1s[i] < DURATION_FOR_AUTO_INCREASING){
					 counterForButtonPress1s[i]++;
				 } else {
					 //the flag is turned on when 1 second has passed
					 //since the button is pressed.
					 flagForButtonPress1s[i] = 1;
					 //todo
				 }
			} else {
				 counterForButtonPress1s[i] = 0;
				 flagForButtonPress1s[i] = 0;
			}
		}
	}
}

unsigned int is_button_pressed(unsigned int index) {
	if(index >= N0_OF_BUTTONS) return 0;
	return (buttonBuffer[index] == BUTTON_IS_PRESSED);
}


unsigned int is_button_pressed_1s(unsigned int index){
	if(index >= N0_OF_BUTTONS) return 0;
	return (flagForButtonPress1s[index] == 1);
}
