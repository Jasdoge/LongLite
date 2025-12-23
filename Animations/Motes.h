#ifndef MOTES
#define MOTES
#include "_Animation.h"
#include "../Configuration.h"

template <size_t MAX_MOTES>
class Motes: public Animation{

	uint8_t MAX_BRIGHTNESS = 255; 

	uint16_t ANIM_DUR = 30e3;
	uint32_t sparkles[MAX_MOTES] = {0};
	uint16_t sparkles_idx[MAX_MOTES] = {0};
	uint8_t sparkles_color[MAX_MOTES] = {0};	// 0b111 = rgb
	uint16_t sparkles_dur[MAX_MOTES] = {0};

	uint32_t last_sparkle = 0;
	
	void addSparkle( const uint32_t ms, const uint16_t idx ){

		for( uint8_t i = 0; i < MAX_MOTES; ++i ){
			
			if( !sparkles[i] || ms-sparkles[i] > sparkles_dur[i] ){

				const size_t NUM_COLORS = 6;
				const uint8_t colors[NUM_COLORS] = {
					0b100, 0b010, 0b001,
					0b110, 0b101, 0b011
				};
				sparkles_idx[i] = idx;
				sparkles[i] = ms;
				sparkles_dur[i] = ANIM_DUR + random(ANIM_DUR);
				sparkles_color[i] = colors[random(NUM_COLORS)];
				return;

			}

		}
	}

	uint32_t getSparkleStartTime( const uint32_t ms, const uint8_t idx ){
		for( uint8_t i = 0; i < MAX_MOTES; ++i ){
			if( sparkles[i] && sparkles_idx[i] == idx && ms-sparkles[i] < sparkles_dur[i] )
				return sparkles[i];
		}
		return 0;
	}

	uint16_t getSparkleDur( const uint8_t idx ){
		for( uint8_t i = 0; i < MAX_MOTES; ++i ){
			if( sparkles_idx[i] == idx )
				return sparkles_dur[i];
		}
		return 0;
	}

	uint8_t getSparkleColor( const uint8_t idx ){
		for( uint8_t i = 0; i < MAX_MOTES; ++i ){
			if( sparkles_idx[i] == idx )
				return sparkles_color[i];
		}
		return 0;
	}
	
	void animatePixel( tinyNeoPixel &leds, const bool isA, const uint8_t i, const uint32_t delta ){

		

		const uint32_t startTime = getSparkleStartTime(delta, i);
		
		float r = 0.0;
		float g = 0.0;
		float b = 0.0;
		if( startTime ){

			const uint32_t localDelta = delta-startTime;
			const uint8_t color = getSparkleColor(i);
			r = (color & 0b100) > 0;
			g = (color & 0b010) > 0;
			b = (color & 0b001) > 0;

			float intensity = (float)localDelta/(float)getSparkleDur(i);
			if( intensity > 0.5 )
				intensity = 1.0-(intensity-0.5)*2.0;
			else
				intensity = intensity*2.0;

			r *= intensity;
			g *= intensity;
			b *= intensity;

		}
		else if( delta-last_sparkle > 500 && random()%((uint16_t)Configuration::NUM_LEDS*100) == 0 ){
			addSparkle(delta, i);
			last_sparkle = delta;
		}

		leds.setPixelColor(i, MAX_BRIGHTNESS*r,MAX_BRIGHTNESS*g,MAX_BRIGHTNESS*b);


	}

	void onBegin() override{
		
	}

	void onRender( tinyNeoPixel &leds, const uint32_t delta, const bool isA ) override{

		uint8_t i;
		for( i = 0; i < Configuration::NUM_LEDS; ++i )
			animatePixel(leds, isA, i, delta);
			
	}
	
};



#endif
