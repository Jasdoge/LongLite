#ifndef CANDYCANE
#define CANDYCANE
#include "_Animation.h"
#include "../Configuration.h"

template <size_t MAX_SPARKLES>
class Candycane: public Animation{

	const uint8_t TAIL_PIXEL_LENGTH = 32;
	uint8_t MAX_BRIGHTNESS = 255; 
	uint16_t COMET_DUR = 5e3;

	float COMET_BLOCKPERC;
	float COMET_TAILSIZE;   // 3 LED tail 

	// Time sparkle started
	//const uint8_t MAX_SPARKLES = 10;

	uint16_t SPARKLE_DUR = 3e3;
	uint32_t sparkles[MAX_SPARKLES] = {0};
	uint16_t sparkles_idx[MAX_SPARKLES] = {0};
	uint32_t last_sparkle = 0;
	

	
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
	
	void animatePixel( tinyNeoPixel &leds, const bool isA, const uint8_t i, const uint32_t delta, const float perc ){

		float out = 0.0;
		float myPerc = 1.0-(float)i/Configuration::NUM_LEDS;
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

		if( delta-last_sparkle > 200 && fabs(dist) <= COMET_BLOCKPERC && random()%10 == 0 ){
			addSparkle(delta, i);
		}
		
		float r = 1.0;
		float g = 1.0;
		float b = 1.0;
		if( isA )
			g = b = 0.0;


		const uint32_t sparkleStart = getSparkleStartTime(delta, i);
		if( sparkleStart ){
			
			r = g = b = 1.0;
			float d = 1.0-(float)(delta-sparkleStart)/SPARKLE_DUR;
			out = d * ((sin(delta*PI*8)*0.5+0.5)*0.1+0.5);

		}

		out = out*out*out;
		if( out < 1.0/MAX_BRIGHTNESS )
			out = 1.0/MAX_BRIGHTNESS;

		leds.setPixelColor(i, MAX_BRIGHTNESS*out*r,MAX_BRIGHTNESS*out*g,MAX_BRIGHTNESS*out*b);


	}

	void onBegin() override{
		COMET_BLOCKPERC = 1.0/Configuration::NUM_LEDS;
		COMET_TAILSIZE = COMET_BLOCKPERC*TAIL_PIXEL_LENGTH;
	}

	void onRender( tinyNeoPixel &leds, const uint32_t delta, const bool isA ) override{

		uint32_t d = delta;
		if( isA )
			d += COMET_DUR/2;
		float perc = (float)(d%COMET_DUR)/COMET_DUR;
		uint8_t i;
		for( i = 0; i < Configuration::NUM_LEDS; ++i )
			animatePixel(leds, isA, i, delta, perc);
			
	}
	
};



#endif
