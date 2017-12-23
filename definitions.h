#include "WS2812FX.h"

// Neopixel
#define PIN D4        // PIN where neopixel / WS2811 strip is attached
#define NUMLEDS 300    // Number of leds in the strip


#define HOSTNAME "MC_ARTNET"   // Friedly hostname


// ***************************************************************************
// Global variables / definitions
// ***************************************************************************
#define DBG_OUTPUT_PORT Serial  // Set debug output port

// List of all color modes
enum MODE { SET_MODE, HOLD, OFF, ALL, WIPE, RAINBOW, RAINBOWCYCLE, THEATERCHASE, THEATERCHASERAINBOW, TV, ARTNET, REALTIME, RANDOM };

MODE mode = RANDOM;   // Standard mode that is active when software starts

uint16_t ws2812fx_speed = 5000;   // Global variable for storing the delay between color changes --> smaller == faster
int brightness = 255;       // Global variable for storing the brightness (255 == 100%)

int ws2812fx_mode = 0;      // Helper variable to set WS2812FX modes

bool exit_func = false;     // Global helper variable to get out of the color modes when mode changes

struct ledstate             // Data structure to store a state of a single led
{
   uint8_t red;
   uint8_t green;
   uint8_t blue;
};

typedef struct ledstate LEDState;   // Define the datatype LEDState
LEDState ledstates[NUMLEDS];        // Get an array of led states to store the state of the whole strip
LEDState main_color;                // Store the "main color" of the strip used in single color modes 

uint8_t randomModes[] = {FX_MODE_MULTI_DYNAMIC, FX_MODE_RAINBOW, FX_MODE_RAINBOW_CYCLE, FX_MODE_THEATER_CHASE_RAINBOW, 
FX_MODE_RUNNING_LIGHTS, FX_MODE_CHASE_RANDOM, FX_MODE_FIREWORKS_RANDOM, FX_MODE_EXT_CIRCLE, FX_MODE_EXT_LARSON_SCANNER, FX_MODE_EXT_FIREWORKS_RANDOM, FX_MODE_EXT_STAR, FX_MODE_EXT_RUN_CHRISTMASS};
//uint8_t randomModes[] = {FX_MODE_EXT_RUN_CHRISTMASS};
