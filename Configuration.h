#ifndef CONFIGURATION_H
#define CONFIGURATION_H

namespace Configuration{

	const uint8_t NUM_LEDS = 100;


	const uint8_t PIN_NEO_A = PIN_PA5;
	const uint8_t PIN_NEO_B = PIN_PA4;
	const uint8_t PIN_READING_EN = PIN_PA2;
	const uint8_t PIN_THR_IN = PIN_PA3;
	const uint8_t PIN_SENS_IN = PIN_PA1;
	const uint8_t PIN_BTN = PIN_PB3;
	const uint8_t PIN_BIGPP_EN = PIN_PA6; // Todo: need to solder on and handle
	const uint8_t PIN_DEBUG_LED = PIN_PB1;

	const uint8_t NUM_WAKE_SUCC = 10;  // Nr WAKE_READ_DUR cycles to turn on
	const uint32_t WAKE_READ_DUR = 60; // Time in whole seconds between readings while trying to wake up
	const uint32_t ON_DUR = 25200e3;  // 7 hours

}


#endif
