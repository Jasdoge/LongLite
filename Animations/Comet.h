#ifndef COMET
#define COMET
#include "_Animation.h"
#include "../Configuration.h"

template <size_t MAX_SPARKLES>
class Comet: public Animation{

	uint8_t MAX_BRIGHTNESS = 100; 
	uint16_t COMET_DUR = 10e3;
	uint16_t COMET_HUE_DUR = 60e3;

	float COMET_BLOCKPERC;
	float COMET_TAILSIZE;   // 3 LED tail 

	// Time sparkle started
	//const uint8_t MAX_SPARKLES = 10;

	uint16_t SPARKLE_DUR = 1e3;
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
	
	void handleComet( tinyNeoPixel leds, const uint8_t i, const uint32_t delta, const float perc, const float huePerc ){

		float out = 0.0;
		float myPerc = (float)i/Configuration::NUM_LEDS;
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

		const uint32_t sparkleStart = getSparkleStartTime(delta, i);
		if( sparkleStart ){
			
			r = g = b = 1.0;
			float d = 1.0-(float)(delta-sparkleStart)/SPARKLE_DUR;
			out = d * ((sin(delta*PI*8)*0.5+0.5)*0.1+0.5);

		}

		out = out*out*out;

		leds.setPixelColor(i, MAX_BRIGHTNESS*out*r,MAX_BRIGHTNESS*out*g,MAX_BRIGHTNESS*out*b);


	}

	void onBegin() override{
		COMET_BLOCKPERC = 1.0/Configuration::NUM_LEDS;
		COMET_TAILSIZE = COMET_BLOCKPERC*8;
	}

	void onRender( tinyNeoPixel &leds, const uint32_t delta, const bool isA ) override{

		const float perc = (float)(delta%COMET_DUR)/COMET_DUR;
		const float huePerc = (float)(delta%COMET_HUE_DUR)/COMET_HUE_DUR;
		uint8_t i;
		for( i = 0; i < Configuration::NUM_LEDS; ++i )
			handleComet(leds, i, delta, perc, huePerc);
			
	}
	
};



#endif
