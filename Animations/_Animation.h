#ifndef _ANIMATION
#define _ANIMATION

class Animation{
	public:
		uint32_t started = 0;

		virtual void onBegin(){}
		virtual void onRender( tinyNeoPixel &leds ){}

		void render( tinyNeoPixel &leds ){
			onRender( leds );
		}
		void begin(){
			started = millis();
			onBegin();
		}


};


#endif

