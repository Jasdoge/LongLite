#include <avr/sleep.h>
#include <tinyNeoPixel_Static.h>

//#define USE_SERIAL
#define NO_OFF // Makes the animation permanently on

ISR(RTC_PIT_vect){
  RTC.PITINTFLAGS = RTC_PI_bm;          /* Clear interrupt flag by writing '1' (required) */
}

const uint8_t PIN_NEO = PIN_PA5;


// The signal seems to get duplicated when connected ad-hoc for some reason... but this is good because we don't need 100 individual controls, and we can use a tiny804
const uint8_t NUM_LEDS = 200;
uint8_t pixels[NUM_LEDS*3];
tinyNeoPixel leds = tinyNeoPixel(NUM_LEDS, PIN_NEO, NEO_GRB, pixels);

uint32_t last_update = 0;     // tracks LED refresh rate




const uint16_t COMET_DUR = 10e3;
const uint32_t COMET_HUE_DUR = 60e3;
const float COMET_BLOCKPERC = 1.0/NUM_LEDS;
const float COMET_TAILSIZE = COMET_BLOCKPERC*8;   // 3 LED tail 

const uint8_t MAX_BRIGHTNESS = 100;

// Time sparkle started
const uint8_t MAX_SPARKLES = 10;
const uint16_t SPARKLE_DUR = 1e3;
uint32_t sparkles[MAX_SPARKLES] = {0};
uint16_t sparkles_idx[MAX_SPARKLES] = {0};
uint32_t last_sparkle = 0;

#ifndef NO_OFF
  const uint8_t PIN_READING_EN = PIN_PA2;
  const uint8_t PIN_THR_IN = PIN_PA3;
  const uint8_t PIN_SENS_IN = PIN_PA1;
  const uint8_t PIN_BTN = PIN_PB3;
  const uint8_t PIN_BIGPP_EN = PIN_PA6; // Todo: need to solder on and handle

  uint32_t last_wake = 0;       // 0 if turned off
  uint8_t wake_succ = 0;        // Nr successful hits below the turnon threshold
  uint8_t sleep_step = 0;       // nr seconds we've been sleeping
  uint8_t button_pressed = 0;  // Helps turn off button on release.
    // 1 = below threshold - Turn off on release
    // 2 = above threshold

  const uint8_t NUM_WAKE_SUCC = 10;  // Nr WAKE_READ_DUR cycles to turn on
  const uint32_t WAKE_READ_DUR = 60; // Time in whole seconds between readings while trying to wake up
  const uint32_t ON_DUR = 25200e3;  // 7 hours
#endif

void addSparkle( const uint32_t ms, const uint16_t idx ){
  for( uint8_t i = 0; i < 10; i++ ){
    
    if( !sparkles[i] || ms-sparkles[i] > SPARKLE_DUR ){
      sparkles_idx[i] = idx;
      sparkles[i] = ms+50+random(50);
      return;
    }

  }
}

uint32_t getSparkleStartTime( const uint32_t ms, const uint8_t idx ){
  for( uint8_t i = 0; i < 10; i++ ){
    if( sparkles[i] && sparkles_idx[i] == idx && ms-sparkles[i] < SPARKLE_DUR )
      return sparkles[i];
  }
  return 0;
}
  
void handleComet( uint8_t i, const uint32_t ms, float perc, float huePerc ){

  float out = 0.0;
  float myPerc = (float)i/NUM_LEDS;
  float dist = perc-myPerc;
  // We're ahead
  if( dist < 0 ){
    // Antialias in
    if( -dist <= COMET_BLOCKPERC )
      out = 1.0+dist/COMET_BLOCKPERC;
    // Wrap around
    else if( 1.0+dist < COMET_TAILSIZE )
      out = 1.0-(1.0+dist)/COMET_TAILSIZE; // // measure our distance

  }
  // We're behind, we can just calculate inverse distance 
  else if( dist < COMET_TAILSIZE )
    out = 1.0-dist/COMET_TAILSIZE;

  if( ms-last_sparkle > 200 && fabs(dist) <= COMET_BLOCKPERC && random()%10 == 0 ){
    addSparkle(ms, i);
  }

  // r -> y
  const uint8_t hueSteps = 6;
  const float hueStep = 1.0/hueSteps;
  float r = 1.0, g = huePerc*hueSteps, b = 0.0;
  // y -> g
  if( huePerc > hueStep ){
    g = 1.0;
    r = 1.0-(huePerc-hueStep)*hueSteps;
  }

  // g -> cyan
  if( huePerc > hueStep*2 ){
    r = 0;
    g = 1.0;
    b = (huePerc-hueStep*2)*hueSteps;
  }

  // cyan -> b
  if( huePerc > hueStep*3 ){
    r = 0;
    g = 1.0-(huePerc-hueStep*3)*hueSteps;
    b = 1.0;
  }

  // b -> purple
  if( huePerc > hueStep*4 ){
    r = (huePerc-hueStep*4)*hueSteps;
    g = 0;
    b = 1.0;
  }

  // purple -> r
  if( huePerc > hueStep*5 ){
    g = 1.0-(huePerc-hueStep*5)*hueSteps;
    b = 0;
  }

  const uint32_t sparkleStart = getSparkleStartTime(ms, i);
  if( sparkleStart ){
    
    r = g = b = 1.0;
    float delta = 1.0-(float)(ms-sparkleStart)/SPARKLE_DUR;
    out = delta * ((sin(delta*PI*8)*0.5+0.5)*0.1+0.5);

  }

  out = out*out*out;

  leds.setPixelColor(i, MAX_BRIGHTNESS*out*r,MAX_BRIGHTNESS*out*g,MAX_BRIGHTNESS*out*b);


}


#ifndef NO_OFF
  void toggleWake( bool on ){

    if( on ){
      
      last_wake = millis();

    }
    else{
      last_wake = 0;
      wake_succ = 0;
      digitalWrite(PIN_NEO,LOW);
      digitalWrite(PIN_READING_EN, LOW);
    }
    digitalWrite(PIN_BIGPP_EN, !on); // P-FET

  }


  void toggleInputs( bool on ){

    if( on ){
      pinMode(PIN_THR_IN, INPUT);
      pinMode(PIN_SENS_IN, INPUT);
      pinMode(PIN_BTN, INPUT);
    }
    else{
      pinMode(PIN_THR_IN, OUTPUT);
      pinMode(PIN_SENS_IN, OUTPUT);
      pinMode(PIN_BTN, OUTPUT);
      digitalWrite(PIN_THR_IN, LOW);
      digitalWrite(PIN_SENS_IN, LOW);
      digitalWrite(PIN_BTN, HIGH);
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
    digitalWrite(PIN_READING_EN, on);
    if( on )
      ADC0.CTRLA |= ADC_ENABLE_bm; // Enable ADC
    else
      ADC0.CTRLA &= ~ADC_ENABLE_bm; // Disable ADC
  }

  uint16_t getCal(){
    return analogRead(PIN_THR_IN);
  }

  uint16_t getSens(){
    uint16_t val = analogRead(PIN_SENS_IN);
    if( val > 800 )
      val = 800;
    return map(val,0,800,0,1023);
  }
#endif

void setup(){

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

    pinMode(PIN_READING_EN, OUTPUT);
    pinMode(PIN_BIGPP_EN, OUTPUT);
    toggleInputs(true);

    // Tie up unused pins
    pinMode(PIN_PB2, OUTPUT); // TX pin
    pinMode(PIN_PB1, OUTPUT);
    pinMode(PIN_PA7, OUTPUT);
    
  #endif

	randomSeed(analogRead(PIN_PB1));      /* Basic random seed */

	pinMode(PIN_NEO, OUTPUT);
	
  leds.clear();
  for( uint8_t i = 0; i < NUM_LEDS; ++i )
    leds.setPixelColor(i, 0, 1, 0);
  leds.show();
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
}

void loop(){
	
  const uint32_t ms = millis();

	#ifndef NO_OFF
		// Threshold calibration button pressed takes priority
		bool buttonPressed = !digitalRead(PIN_BTN);
		if( buttonPressed ){

			if( ms-last_update < 16 )
				return;
			last_update = ms;

			if( !last_wake )
				toggleWake(true);

			button_pressed = 1;
			toggleReadings(true);
			digitalWrite(PIN_READING_EN, HIGH);
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
			for( uint8_t i = 0; i < NUM_LEDS; ++i )
				leds.setPixelColor(i,r,g,0);
			leds.show();
			

		}
		// Handle button release
		else if( button_pressed ){
			
			toggleReadings(false);
			leds.clear();
			leds.show();
			
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

			const float perc = (float)(ms%COMET_DUR)/COMET_DUR;
			const float huePerc = (float)(ms%COMET_HUE_DUR)/COMET_HUE_DUR;
			uint8_t i;
			// do LEDs A
			for( i = 0; i < NUM_LEDS; ++i )
				handleComet(i, ms, perc, huePerc);
			leds.show();


	}
    

	#ifndef NO_OFF
			// Check turnoff
			if( ms-last_wake > ON_DUR )
				toggleWake(false);

		}

		// We're turned off and trying to turn on
		else{

			// Spaces out the time between readings
			if( sleep_step < WAKE_READ_DUR ){
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
					if( wake_succ >= NUM_WAKE_SUCC ){
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

