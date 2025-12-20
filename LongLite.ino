#include <avr/sleep.h>
#include <tinyNeoPixel_Static.h>
#include "Animations/Comet.h"
#include "Animations/_Animation.h"
#include "Configuration.h"

//#define USE_SERIAL
//#define IGNORE_LEDS_B
#define NO_OFF // Makes the animation permanently on

//std::unique_ptr<Animation> currentAnimation;
uint8_t pixels[Configuration::NUM_LEDS*3];
tinyNeoPixel ledsA = tinyNeoPixel(Configuration::NUM_LEDS, Configuration::PIN_NEO_A, NEO_GRB, pixels);
tinyNeoPixel ledsB = tinyNeoPixel(Configuration::NUM_LEDS, Configuration::PIN_NEO_B, NEO_GRB, pixels);

uint32_t last_update = 0;     // tracks LED refresh rate


// Max sparkles
Comet<10> animationA;
Comet<10> animationB;


void selectAnimator(){
	animationA.begin();
	animationB.begin();
}

void render(){
	
	const uint32_t ms = millis();
	animationA.render(ledsA, ms, true);
	ledsA.show();

	animationB.render(ledsB, ms, false);
	ledsB.show();

}



ISR(RTC_PIT_vect){
  RTC.PITINTFLAGS = RTC_PI_bm;          /* Clear interrupt flag by writing '1' (required) */
}






#ifndef NO_OFF

	uint32_t last_wake = 0;       // 0 if turned off
	uint8_t wake_succ = 0;        // Nr successful hits below the turnon threshold
	uint8_t sleep_step = 0;       // nr seconds we've been sleeping
	uint8_t button_pressed = 0;  // Helps turn off button on release.
		// 1 = below threshold - Turn off on release
		// 2 = above threshold

	

	void toggleWake( bool on ){

		if( on ){
		
			last_wake = millis();

		}
		else{
			last_wake = 0;
			wake_succ = 0;
			digitalWrite(Configuration::PIN_NEO_A,LOW);
			digitalWrite(Configuration::PIN_READING_EN, LOW);
		}
		digitalWrite(Configuration::PIN_BIGPP_EN, !on); // P-FET

	}


	void toggleInputs( bool on ){

		if( on ){
			pinMode(Configuration::PIN_THR_IN, INPUT);
			pinMode(Configuration::PIN_SENS_IN, INPUT);
			pinMode(Configuration::PIN_BTN, INPUT);
		}
		else{
			pinMode(Configuration::PIN_THR_IN, OUTPUT);
			pinMode(Configuration::PIN_SENS_IN, OUTPUT);
			pinMode(Configuration::PIN_BTN, OUTPUT);
			digitalWrite(Configuration::PIN_THR_IN, LOW);
			digitalWrite(Configuration::PIN_SENS_IN, LOW);
			digitalWrite(Configuration::PIN_BTN, HIGH);
		}

	}

	void sleep(){

		#ifdef USE_SERIAL
		//Serial.println("Enter sleep mode");
		//delay(500);
		#endif
		
		toggleInputs(false);
		sleep_cpu();
		toggleInputs(true);

		#ifdef USE_SERIAL
		//delay(500);
		//Serial.println("Woke up!");
		#endif

	}

	void toggleReadings( bool on ){
		digitalWrite(Configuration::PIN_READING_EN, on);
		if( on )
			ADC0.CTRLA |= ADC_ENABLE_bm; // Enable ADC
		else
			ADC0.CTRLA &= ~ADC_ENABLE_bm; // Disable ADC
	}

	uint16_t getCal(){
		return analogRead(Configuration::PIN_THR_IN);
	}

	uint16_t getSens(){
		uint16_t val = analogRead(Configuration::PIN_SENS_IN);
		if( val > 800 )
			val = 800;
		return map(val,0,800,0,1023);
	}
#endif

void setup(){

	pinMode(Configuration::PIN_DEBUG_LED, OUTPUT);
	digitalWrite(Configuration::PIN_DEBUG_LED, HIGH);

	#ifndef NO_OFF
		while( RTC.STATUS > 0 ){
		;                                   /* Wait for all register to be synchronized */
		}
		RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;    /* 32.768kHz Internal Ultra-Low-Power Oscillator (OSCULP32K) */
		RTC.PITINTCTRL = RTC_PI_bm;           /* PIT Interrupt: enabled */
		RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc /* 1hz */
		| RTC_PITEN_bm;                       /* Enable PIT counter: enabled */

		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();

		pinMode(Configuration::PIN_READING_EN, OUTPUT);
		pinMode(Configuration::PIN_BIGPP_EN, OUTPUT);
		toggleInputs(true);

		// Tie up unused pins
		pinMode(PIN_PB2, OUTPUT); // TX pin
		pinMode(PIN_PB1, OUTPUT);
		pinMode(PIN_PA7, OUTPUT);
		
	#endif

	randomSeed(analogRead(PIN_PB1));      /* Basic random seed */

	pinMode(Configuration::PIN_NEO_A, OUTPUT);
	pinMode(Configuration::PIN_NEO_B, OUTPUT);
		
	ledsA.clear();
	ledsB.clear();
	for( uint8_t i = 0; i < Configuration::NUM_LEDS; ++i )
		ledsA.setPixelColor(i, 0, 1, 0);
	ledsA.show();

	for( uint8_t i = 0; i < Configuration::NUM_LEDS; ++i )
		ledsB.setPixelColor(i, 0, 1, 0);
	ledsB.show();

	delay(1000);
	
	#ifdef USE_SERIAL
	Serial.begin(9600);
	delay(1000);
	Serial.println("IT BEGINS!");
	#endif
	delay(10); // Needed for PIN_BTN to get a correct value
	#ifndef NO_OFF
		toggleWake(true);
	#endif
	selectAnimator();
	digitalWrite(Configuration::PIN_DEBUG_LED, LOW);

}

void loop(){
	
  const uint32_t ms = millis();

	#ifndef NO_OFF
		// Threshold calibration button pressed takes priority
		bool buttonPressed = !digitalRead(Configuration::PIN_BTN);
		if( buttonPressed ){

			if( ms-last_update < 16 )
				return;
			last_update = ms;

			if( !last_wake )
				toggleWake(true);

			button_pressed = 1;
			toggleReadings(true);
			digitalWrite(Configuration::PIN_READING_EN, HIGH);
			delay(100);
			
			#ifdef USE_SERIAL
			/*
			Serial.print("Cal:");
			Serial.print(getCal());
			Serial.print(",Sens:");
			Serial.println(analogRead(PIN_SENS_IN));
			*/
			#endif

			uint8_t r = 1, g = 0;
			if( getSens() > getCal() ){
				button_pressed = 2;
				r = 0; g = 1;
			}
			for( uint8_t i = 0; i < Configuration::NUM_LEDS; ++i )
				ledsA.setPixelColor(i,r,g,0);
			ledsA.show(); ledsB.show();	// They use the same pixel buffer, so we can show them both at once
			

		}
		// Handle button release
		else if( button_pressed ){
			
			toggleReadings(false);
			leds.clear();
			ledsA.show();
			ledsB.show();
			
			if( button_pressed == 1 )
				toggleWake(false);
			button_pressed = 0;
			
		}
		// We're awake
		else if( last_wake ){
	#endif

	// Always Update LEDs
	if( ms-last_update > 16 ){

		last_update = ms;
		//handleComet();
		render();

	}
	

	#ifndef NO_OFF
			// Check turnoff
			if( ms-last_wake > Configuration::ON_DUR )
				toggleWake(false);

		}

		// We're turned off and trying to turn on
		else{

			// Spaces out the time between readings
			if( sleep_step < Configuration::WAKE_READ_DUR ){
				++sleep_step;
			}
			// Need to take a quick reading
			else{

				toggleReadings(true);
				delay(5);
				sleep_step = 0; 
				uint16_t sens = getSens(), cal = getCal();
				toggleReadings(false);
				#ifdef USE_SERIAL
				/*
				Serial.print("Sens:");
				Serial.print(sens);
				Serial.print(",Cal:");
				Serial.println(cal);
				*/
				#endif
				if( sens > cal  ){
					
					++wake_succ;
					if( wake_succ >= Configuration::NUM_WAKE_SUCC ){
						//Serial.println("Wake up permanent!");
						toggleWake(true);
						return; // Skip sleep
					}

				}
				else
					wake_succ = 0;

			}

			sleep();

		}
	#endif
  
  

}

