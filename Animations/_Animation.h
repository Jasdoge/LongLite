#ifndef _ANIMATION
#define _ANIMATION

class Animation{
	public:
		uint32_t started = 0;

		virtual void onBegin(){}
		virtual void onRender( tinyNeoPixel &leds, const uint32_t delta, const bool isA ){}

		void render( tinyNeoPixel &leds, const uint32_t ms, const bool isA = true ){
			onRender( leds, ms-started, isA );
		}
		void begin(){
			started = millis();
			onBegin();
		}


};


#endif

