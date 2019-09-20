/**
 *	Our Portal Box project has a number of LED arrays; NeoPixels in revisions
 *	up to and including 2.06 and DotStars thereafter. As the arrays are
 *	connected with a single data line for control, timing of controll signals
 *	is delicate and the Raspberry Pis running newer Linux Kernels do not work
 *	well. Therefore we have added an Arduino to control the LED arrays. This
 *	firmware is designed to be installed on the Arduino by the Pi as needed.
 *
 *	@target Arduino Pro Mini (pro8MHzatmega328)
 *
 *	We are using the Arduino setup thus the functions setup and loop are special
 *	If this were a vanilla C/C++ project, one could imagine main being written:
 *
 *	int main() {
 *		Arduino.init();
 *		setup();
 *		while(true) {
 *			loop();
 *		}
 *	}
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/**
 * Define a maximum command buffer length that is actually one shorter than
 * what we intend to be the maximum. This way when we init the buffer with
 * `calloc` and clear it with `memset` there will always be a "null" byte at
 * the end of the buffer making it safe to pass the buffer to the UNSAFE avr
 * string functions like `strtok`
 */
#define MAX_INPUT_BUFFER_LEN 127

/**
 * Define the Digital IO pin that is connected to the data line for controlling
 * the strip of LED Arrays 
 */
#define LED_PIN 5

/**
 * Define how many LED arrays are in the strip
 */
#define LED_COUNT 15

/**
 * Set a default brightness to about 1/5 (max = 255)
 */
#define DEFAULT_BRIGHTNESS 128

#define MAX_PULSE_BRIGHTNESS 120
#define MIN_PULSE_BRIGHTNESS 20
#define PULSE_BRIGHTNESS_STEP 5

/*
 * Declare a buffer where we will accumulate characters coming in over the
 * Serial connection
 */
char *input_buffer;

/**
 * The Arduino `readBytesUntil` function is non blocking. Therefore we must
 * implement our own in the loop function by writing character from the serial
 * buffer into the end of our buffer. Thus we need to track the end of the data
 * in the buffer as an offset from the start of the buffer.
 */
int len_input_buffer_data;

/**
 *	Declare the interface to the strip of LED arrays
 */
Adafruit_NeoPixel strip;

/**
 * flag to indicate we should be pulsing.
 */
bool is_pulsing = false;
bool pulse_rising = false;

/**
 * Parse a command and carry it out.
 */
void process_command(char * command) {
	int errno = 0;
	digitalWrite(LED_BUILTIN, LOW);

	// get command part of buffer and determine if it is recognized
	char * fragment = strtok(command, " ");
	if(0 == strcmp("blink", fragment)) {
		// the blink command requires a color as three components, a duration,
		// and a repeat as inputs
		fragment = strtok(NULL, " ");
		int red = atoi(fragment);
		if(0 > red || 255 < red) {
			Serial.println(1); // respond that there was an error
			return;
		}

		fragment = strtok(NULL, " ");
		int green = atoi(fragment);
		if(0 > green || 255 < green) {
			Serial.println(1); // respond that there was an error
			return;
		}

		fragment = strtok(NULL, " ");
		int blue = atoi(fragment);
		if(0 > blue || 255 < blue) {
			Serial.println(1); // respond that there was an error
			return;
		}

		fragment = strtok(NULL, " ");
		int duration = atoi(fragment);
		if(0 > duration) {
			Serial.println(1); // respond that there was an error
			return;
		}

		fragment = strtok(NULL, " ");
		int repeats = atoi(fragment);
		if(0 > repeats) {
			Serial.println(1); // respond that there was an error
			return;
		}

		int wait = duration / (2 * repeats);
		uint32_t color = strip.Color(red, green, blue);
		uint32_t black = strip.Color(0,0,0);

		is_pulsing = false;
		strip.setBrightness(DEFAULT_BRIGHTNESS);
		for(int i = 0; i < repeats; i++) {
			for(int j = 0; j < LED_COUNT; j++) {
				strip.setPixelColor(j, black);
			}
			strip.show();
			delay(wait);
			for(int j = 0; j < LED_COUNT; j++) {
				strip.setPixelColor(j, color);
			}
			strip.show();
			delay(wait);
		}
		for(int j = 0; j < LED_COUNT; j++) {
			strip.setPixelColor(j, black);
		}
		strip.show();
	} else if(0 == strcmp("wipe", fragment)) {
		// the wipe command requires four values: red, green, blue, duration
		// red, green and blue are unsigned chars. duration is an unsigned int
		// of milliseconds that the entire effect should take to complete.
		fragment = strtok(NULL, " ");
		int red = atoi(fragment);
		if(0 > red || 255 < red) {
			Serial.println(1); // respond that there was an error
			return;
		}

		fragment = strtok(NULL, " ");
		int green = atoi(fragment);
		if(0 > green || 255 < green) {
			Serial.println(1); // respond that there was an error
			return;
		}

		fragment = strtok(NULL, " ");
		int blue = atoi(fragment);
		if(0 > blue || 255 < blue) {
			Serial.println(1); // respond that there was an error
			return;
		}

		fragment = strtok(NULL, " ");
		int duration = atoi(fragment);
		if(0 > duration) {
			Serial.println(1); // respond that there was an error
			return;
		}

		uint32_t color = strip.Color(red, green, blue);
		int wait = duration / LED_COUNT;

		is_pulsing = false;
		strip.setBrightness(DEFAULT_BRIGHTNESS);
		for(int i=0; i<LED_COUNT; i++) {
			strip.setPixelColor(i, color);
			strip.show();
			delay(wait);
		}
	} else if(0 == strcmp("color", fragment)) {
		// the color command requires three values: red, green, and blue. Red,
		// green and blue are unsigned chars.
		fragment = strtok(NULL, " ");
		int red = atoi(fragment);
		if(0 > red || 255 < red) {
			Serial.println(1); // respond that there was an error
			return;
		}

		fragment = strtok(NULL, " ");
		int green = atoi(fragment);
		if(0 > green || 255 < green) {
			Serial.println(1); // respond that there was an error
			return;
		}

		fragment = strtok(NULL, " ");
		int blue = atoi(fragment);
		if(0 > blue || 255 < blue) {
			Serial.println(1); // respond that there was an error
			return;
		}

		uint32_t color = strip.Color(red, green, blue);

		is_pulsing = false;
		strip.setBrightness(DEFAULT_BRIGHTNESS);
		for(int i=0; i<LED_COUNT; i++) {
			strip.setPixelColor(i, color);
		}
		strip.show();
	} else if(0 == strcmp("pulse", fragment)) {
		// pulsing is indefinate... set a flag and do in loop 
		is_pulsing = true;
	} else {
		errno = 1;
	}

	digitalWrite(LED_BUILTIN, HIGH);
	Serial.println(errno);
}

/**
 * Clears the characters in the input buffer and set the length of the data
 * to zero (0)
 */
void flush_input_buffer() {
	memset(input_buffer, 0, len_input_buffer_data);
	len_input_buffer_data = 0;
}

/**
 *	setup is a special function defined by the Arduino platform
 *	that is called once after the core firmware initialization
 *	happens but before loop is called for the first time
 */
void setup(void) {
	// Initialize the input buffer;
	//   one byte longer than max so always ends in \0 for strtok
	// Note: can not flush buffer as it does not yet exist
	input_buffer = (char *)calloc(MAX_INPUT_BUFFER_LEN + 1, 1);

	strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
	strip.begin();
	strip.setBrightness(DEFAULT_BRIGHTNESS);
	strip.setBrightness(DEFAULT_BRIGHTNESS);
	for(int i = 0; i < LED_COUNT; i++) {
		strip.setPixelColor(i, 0);
	}
	strip.show();

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	// Initialize serial connection
	while(!Serial) {
		delay(100);
	}
	Serial.begin(9600);
}

/**
 *	loop is a special function defined by the Arduino platform that is
 *	called continuously in an infinite loop once platform specific
 *	initialization and setup() have run.
 */
void loop(void) {
	int input;

	// wait for input on Serial.
	if(Serial.available()) {
		while(-1 != (input = Serial.read())) {
			switch(input) {
				case 0: // invalid character; do not buffer
					break;
				case 13:
				case 10: // new line process buffer
						// but only if the buffer contains data...
						// so CR+LF does not result in response of:
						// "invalid command"
					if(0 < len_input_buffer_data) {
						process_command(input_buffer);
						flush_input_buffer();
					}
					break;
				default: // append to buffer
					// warning data loss by truncation for non UTF-8 input
					char input_byte = (char)input;
					input_buffer[len_input_buffer_data] = input_byte;
					len_input_buffer_data++;
					if(MAX_INPUT_BUFFER_LEN <= len_input_buffer_data) {
						Serial.println("Input too long");
						flush_input_buffer();
					}
			}
		}
	}

	if(is_pulsing) {
		int brightness = strip.getBrightness();
		if(pulse_rising) {
			brightness += PULSE_BRIGHTNESS_STEP;
			if(MAX_PULSE_BRIGHTNESS < brightness) {
				pulse_rising = false;
				brightness = MAX_PULSE_BRIGHTNESS;
			}
		} else {
			brightness -= PULSE_BRIGHTNESS_STEP;
			if(MIN_PULSE_BRIGHTNESS > brightness) {
				pulse_rising = true;
				brightness = MIN_PULSE_BRIGHTNESS;
			}
		}
		strip.setBrightness(brightness);
		strip.show();
		delay(100);
	}
}
