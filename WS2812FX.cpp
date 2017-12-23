/*
  WS2812FX.cpp - Library for WS2812 LED effects.

  Harm Aldick - 2016
  www.aldick.org


  FEATURES
    * A lot of blinken modes and counting
    * WS2812FX can be used as drop-in replacement for Adafruit Neopixel Library

  NOTES
    * Uses the Adafruit Neopixel library. Get it here:
      https://github.com/adafruit/Adafruit_NeoPixel



  LICENSE

  The MIT License (MIT)

  Copyright (c) 2016  Harm Aldick

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.


  CHANGELOG

  2016-05-28   Initial beta release
  2016-06-03   Code cleanup, minor improvements, new modes
  2016-06-04   2 new fx, fixed setColor (now also resets _mode_color)
  2017-02-02   added external trigger functionality (e.g. for sound-to-light)
  2017-02-02   removed "blackout" on mode, speed or color-change
  2017-09-26   implemented segment and reverse features
  2017-11-16   changed speed calc, reduced memory footprint
*/

#include "WS2812FX.h"

void WS2812FX::init() {
  RESET_RUNTIME;
  Adafruit_NeoPixel::begin();
  setBrightness(_brightness);
  Adafruit_NeoPixel::show();
}

void WS2812FX::service() {
  if(_running || _triggered) {
    unsigned long now = millis(); // Be aware, millis() rolls over every 49 days
    bool doShow = false;
    for(uint8_t i=0; i < _num_segments; i++) {
      _segment_index = i;
      if(now > SEGMENT_RUNTIME.next_time || _triggered) {
        doShow = true;
        uint16_t delay = (this->*_mode[SEGMENT.mode])();
        SEGMENT_RUNTIME.next_time = now + max(delay, SPEED_MIN);
        SEGMENT_RUNTIME.counter_mode_call++;
      }
    }
    if(doShow) {
      delay(1); // for ESP32 (see https://forums.adafruit.com/viewtopic.php?f=47&t=117327)
      Adafruit_NeoPixel::show();
    }
    _triggered = false;
  }
}

void WS2812FX::start() {
  RESET_RUNTIME;
  _running = true;
}

void WS2812FX::stop() {
  _running = false;
  strip_off();
}

void WS2812FX::trigger() {
  _triggered = true;
}

void WS2812FX::setMode(uint8_t m) {
  RESET_RUNTIME;
  _segments[0].mode = constrain(m, 0, MODE_COUNT - 1);
  setBrightness(_brightness);
}

void WS2812FX::setSpeed(uint16_t s) {
  RESET_RUNTIME;
  _segments[0].speed = constrain(s, SPEED_MIN, SPEED_MAX);
}

void WS2812FX::increaseSpeed(uint8_t s) {
  s = constrain(SEGMENT.speed + s, SPEED_MIN, SPEED_MAX);
  setSpeed(s);
}

void WS2812FX::decreaseSpeed(uint8_t s) {
  s = constrain(SEGMENT.speed - s, SPEED_MIN, SPEED_MAX);
  setSpeed(s);
}

void WS2812FX::setColor(uint8_t r, uint8_t g, uint8_t b) {
  setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
}

void WS2812FX::setColor(uint32_t c) {
  RESET_RUNTIME;
  _segments[0].colors[0] = c;
  setBrightness(_brightness);
}

void WS2812FX::setBrightness(uint8_t b) {
  _brightness = constrain(b, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  Adafruit_NeoPixel::setBrightness(_brightness);
  Adafruit_NeoPixel::show();
  delay(1);
}

void WS2812FX::increaseBrightness(uint8_t s) {
  s = constrain(_brightness + s, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  setBrightness(s);
}

void WS2812FX::decreaseBrightness(uint8_t s) {
  s = constrain(_brightness - s, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  setBrightness(s);
}

void WS2812FX::setLength(uint16_t b) {
  RESET_RUNTIME;
  if (b < 1) b = 1;

  // Decrease numLEDs to maximum available memory
  do {
      Adafruit_NeoPixel::updateLength(b);
      b--;
  } while(!Adafruit_NeoPixel::numLEDs && b > 1);

  _segments[0].start = 0;
  _segments[0].stop = Adafruit_NeoPixel::numLEDs - 1;
}

void WS2812FX::increaseLength(uint16_t s) {
  s = _segments[0].stop - _segments[0].start + 1 + s;
  setLength(s);
}

void WS2812FX::decreaseLength(uint16_t s) {
  if (s > _segments[0].stop - _segments[0].start + 1) s = 1;
  s = _segments[0].stop - _segments[0].start + 1 - s;

  for(uint16_t i=_segments[0].start + s; i <= (_segments[0].stop - _segments[0].start + 1); i++) {
    Adafruit_NeoPixel::setPixelColor(i, 0);
  }
  Adafruit_NeoPixel::show();

  setLength(s);
}

boolean WS2812FX::isRunning() {
  return _running;
}

uint8_t WS2812FX::getMode(void) {
  return _segments[0].mode;
}

uint16_t WS2812FX::getSpeed(void) {
  return _segments[0].speed;
}

uint8_t WS2812FX::getBrightness(void) {
  return _brightness;
}

uint16_t WS2812FX::getLength(void) {
  return _segments[0].stop - _segments[0].start + 1;
}

uint8_t WS2812FX::getModeCount(void) {
  return MODE_COUNT;
}

uint8_t WS2812FX::getNumSegments(void) {
  return _num_segments;
}

void WS2812FX::setNumSegments(uint8_t n) {
  _num_segments = n;
}

uint32_t WS2812FX::getColor(void) {
  return _segments[0].colors[0];
}

WS2812FX::segment* WS2812FX::getSegments(void) {
  return _segments;
}

const __FlashStringHelper* WS2812FX::getModeName(uint8_t m) {
  if(m < MODE_COUNT) {
    return _name[m];
  } else {
    return F("");
  }
}

void WS2812FX::setSegment(uint8_t n, uint16_t start, uint16_t stop, uint8_t mode, uint32_t color, uint16_t speed, bool reverse) {
  if(n < MAX_NUM_SEGMENTS) {
    if(n + 1 > _num_segments) _num_segments = n + 1;
    _segments[n].start = start;
    _segments[n].stop = stop;
    _segments[n].mode = mode;
    _segments[n].speed = speed;
    _segments[n].reverse = reverse;
    _segments[n].colors[0] = color;
  }
}

void WS2812FX::setSegment(uint8_t n, uint16_t start, uint16_t stop, uint8_t mode, const uint32_t colors[], uint16_t speed, bool reverse) {
  if(n < MAX_NUM_SEGMENTS) {
    if(n + 1 > _num_segments) _num_segments = n + 1;
    _segments[n].start = start;
    _segments[n].stop = stop;
    _segments[n].mode = mode;
    _segments[n].speed = speed;
    _segments[n].reverse = reverse;

    for(uint8_t i=0; i<NUM_COLORS; i++) {
      _segments[n].colors[i] = colors[i];
    }
  }
}

/* #####################################################
#
#  Color and Blinken Functions
#
##################################################### */

/*
 * Turns everything off. Doh.
 */
void WS2812FX::strip_off() {
  Adafruit_NeoPixel::clear();
  Adafruit_NeoPixel::show();
}


/*
 * Put a value 0 to 255 in to get a color value.
 * The colours are a transition r -> g -> b -> back to r
 * Inspired by the Adafruit examples.
 */
uint32_t WS2812FX::color_wheel(uint8_t pos) {
  pos = 255 - pos;
  if(pos < 85) {
    return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
  } else if(pos < 170) {
    pos -= 85;
    return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
  } else {
    pos -= 170;
    return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
  }
}


/*
 * Returns a new, random wheel index with a minimum distance of 42 from pos.
 */
uint8_t WS2812FX::get_random_wheel_index(uint8_t pos) {
  uint8_t r = 0;
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t d = 0;

  while(d < 42) {
    r = random(256);
    x = abs(pos - r);
    y = 255 - x;
    d = min(x, y);
  }

  return r;
}


/*
 * No blinking. Just plain old static light.
 */
uint16_t WS2812FX::mode_static(void) {
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, SEGMENT.colors[0]);
  }
  return 500;
}


/*
 * Blink/strobe function
 * Alternate between color1 and color2
 * if(strobe == true) then create a strobe effect
 */
uint16_t WS2812FX::blink(uint32_t color1, uint32_t color2, bool strobe) {
  uint32_t color = ((SEGMENT_RUNTIME.counter_mode_call & 1) == 0) ? color1 : color2;
  if(SEGMENT.reverse) color = (color == color1) ? color2 : color1;

  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, color);
  }

  if((SEGMENT_RUNTIME.counter_mode_call & 1) == 0) {
    return strobe ? 20 : (SEGMENT.speed / 2);
  } else {
    return strobe ? SEGMENT.speed - 20 : (SEGMENT.speed / 2);
  }
}


/*
 * Normal blinking. 50% on/off time.
 */
uint16_t WS2812FX::mode_blink(void) {
  return blink(SEGMENT.colors[0], SEGMENT.colors[1], false);
}


/*
 * Classic Blink effect. Cycling through the rainbow.
 */
uint16_t WS2812FX::mode_blink_rainbow(void) {
  return blink(color_wheel(SEGMENT_RUNTIME.counter_mode_call & 0xFF), BLACK, false);
}


/*
 * Classic Strobe effect.
 */
uint16_t WS2812FX::mode_strobe(void) {
  return blink(SEGMENT.colors[0], SEGMENT.colors[1], true);
}


/*
 * Classic Strobe effect. Cycling through the rainbow.
 */
uint16_t WS2812FX::mode_strobe_rainbow(void) {
  return blink(color_wheel(SEGMENT_RUNTIME.counter_mode_call & 0xFF), BLACK, true);
}


/*
 * Color wipe function
 * LEDs are turned on (color1) in sequence, then turned off (color2) in sequence.
 * if (bool rev == true) then LEDs are turned off in reverse order
 */
uint16_t WS2812FX::color_wipe(uint32_t color1, uint32_t color2, bool rev) {
  if(SEGMENT_RUNTIME.counter_mode_step < SEGMENT_LENGTH) {
    uint32_t led_offset = SEGMENT_RUNTIME.counter_mode_step;
    if(SEGMENT.reverse) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - led_offset, color1);
    } else {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + led_offset, color1);
    }
  } else {
    uint32_t led_offset = SEGMENT_RUNTIME.counter_mode_step - SEGMENT_LENGTH;
    if((SEGMENT.reverse && !rev) || (!SEGMENT.reverse && rev)) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - led_offset, color2);
    } else {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + led_offset, color2);
    }
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % (SEGMENT_LENGTH * 2);
  return (SEGMENT.speed / (SEGMENT_LENGTH * 2));
}

/*
 * Lights all LEDs one after another.
 */
uint16_t WS2812FX::mode_color_wipe(void) {
  return color_wipe(SEGMENT.colors[0], SEGMENT.colors[1], false);
}

uint16_t WS2812FX::mode_color_wipe_inv(void) {
  return color_wipe(SEGMENT.colors[1], SEGMENT.colors[0], false);
}

uint16_t WS2812FX::mode_color_wipe_rev(void) {
  return color_wipe(SEGMENT.colors[0], SEGMENT.colors[1], true);
}

uint16_t WS2812FX::mode_color_wipe_rev_inv(void) {
  return color_wipe(SEGMENT.colors[1], SEGMENT.colors[0], true);
}


/*
 * Turns all LEDs after each other to a random color.
 * Then starts over with another color.
 */
uint16_t WS2812FX::mode_color_wipe_random(void) {
  if(SEGMENT_RUNTIME.counter_mode_step % SEGMENT_LENGTH == 0) { // aux_param will store our random color wheel index
    SEGMENT_RUNTIME.aux_param = get_random_wheel_index(SEGMENT_RUNTIME.aux_param);
  }
  uint32_t color = color_wheel(SEGMENT_RUNTIME.aux_param);
  return color_wipe(color, color, false) * 2;
}


/*
 * Random color intruduced alternating from start and end of strip.
 */
uint16_t WS2812FX::mode_color_sweep_random(void) {
  if(SEGMENT_RUNTIME.counter_mode_step % SEGMENT_LENGTH == 0) { // aux_param will store our random color wheel index
    SEGMENT_RUNTIME.aux_param = get_random_wheel_index(SEGMENT_RUNTIME.aux_param);
  }
  uint32_t color = color_wheel(SEGMENT_RUNTIME.aux_param);
  return color_wipe(color, color, true) * 2;
}


/*
 * Lights all LEDs in one random color up. Then switches them
 * to the next random color.
 */
uint16_t WS2812FX::mode_random_color(void) {
  SEGMENT_RUNTIME.aux_param = get_random_wheel_index(SEGMENT_RUNTIME.aux_param); // aux_param will store our random color wheel index
  uint32_t color = color_wheel(SEGMENT_RUNTIME.aux_param);

  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, color);
  }
  return (SEGMENT.speed);
}


/*
 * Lights every LED in a random color. Changes one random LED after the other
 * to another random color.
 */
uint16_t WS2812FX::mode_single_dynamic(void) {
  if(SEGMENT_RUNTIME.counter_mode_call == 0) {
    for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
      Adafruit_NeoPixel::setPixelColor(i, color_wheel(random(256)));
    }
  }

  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + random(SEGMENT_LENGTH), color_wheel(random(256)));
  return (SEGMENT.speed);
}


/*
 * Lights every LED in a random color. Changes all LED at the same time
 * to new random colors.
 */
uint16_t WS2812FX::mode_multi_dynamic(void) {
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, color_wheel(random(256)));
  }
  return (SEGMENT.speed);
}


/*
 * Does the "standby-breathing" of well known i-Devices. Fixed Speed.
 * Use mode "fade" if you like to have something similar with a different speed.
 */
uint16_t WS2812FX::mode_breath(void) {
  //                                      0    1    2   3   4   5   6    7   8   9  10  11   12   13   14   15   16    // step
  uint16_t breath_delay_steps[] =     {   7,   9,  13, 15, 16, 17, 18, 930, 19, 18, 15, 13,   9,   7,   4,   5,  10 }; // magic numbers for breathing LED
  uint8_t breath_brightness_steps[] = { 150, 125, 100, 75, 50, 25, 16,  15, 16, 25, 50, 75, 100, 125, 150, 220, 255 }; // even more magic numbers!

  if(SEGMENT_RUNTIME.counter_mode_call == 0) {
    SEGMENT_RUNTIME.aux_param = breath_brightness_steps[0] + 1; // we use aux_param to store the brightness
  }

  uint8_t breath_brightness = SEGMENT_RUNTIME.aux_param;

  if(SEGMENT_RUNTIME.counter_mode_step < 8) {
    breath_brightness--;
  } else {
    breath_brightness++;
  }

  // update index of current delay when target brightness is reached, start over after the last step
  if(breath_brightness == breath_brightness_steps[SEGMENT_RUNTIME.counter_mode_step]) {
    SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % (sizeof(breath_brightness_steps)/sizeof(uint8_t));
  }

  int b = map(breath_brightness, 0, 255, 0, _brightness);  // keep brightness below brightness set by user
  uint8_t red = (SEGMENT.colors[0] >> 16 & 0xFF) * b / _brightness; // modify RGB colors with brightness info
  uint8_t green = (SEGMENT.colors[0] >> 8 & 0xFF) * b / _brightness;
  uint8_t blue = (SEGMENT.colors[0] & 0xFF) * b / _brightness;
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, red, green, blue);    // set all LEDs to selected color
  }

  SEGMENT_RUNTIME.aux_param = breath_brightness;
  return breath_delay_steps[SEGMENT_RUNTIME.counter_mode_step];
}


/*
 * Fades the LEDs on and (almost) off again.
 */
uint16_t WS2812FX::mode_fade(void) {
  int b = SEGMENT_RUNTIME.counter_mode_step - 31;
  b = 63 - (abs(b) * 2);
  b = map(b, 0, 64, min(25, _brightness), _brightness);

  uint8_t red = (SEGMENT.colors[0] >> 16 & 0xFF) * b / _brightness; // modify RGB colors with brightness info
  uint8_t green = (SEGMENT.colors[0] >> 8 & 0xFF) * b / _brightness;
  uint8_t blue = (SEGMENT.colors[0] & 0xFF) * b / _brightness;
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, red, green, blue);
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % 64;
  return (SEGMENT.speed / 64);
}


/*
 * Runs a single pixel back and forth.
 */
uint16_t WS2812FX::mode_scan(void) {
  if(SEGMENT_RUNTIME.counter_mode_step > (SEGMENT_LENGTH * 2) - 3) {
    SEGMENT_RUNTIME.counter_mode_step = 0;
  }

  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, BLACK);
  }

  int led_offset = SEGMENT_RUNTIME.counter_mode_step - (SEGMENT_LENGTH - 1);
  led_offset = abs(led_offset); 

  if(SEGMENT.reverse) {
    Adafruit_NeoPixel::setPixelColor(SEGMENT.start + led_offset, SEGMENT.colors[0]);
  } else {
    Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - led_offset, SEGMENT.colors[0]);
  }

  SEGMENT_RUNTIME.counter_mode_step++;
  return (SEGMENT.speed / (SEGMENT_LENGTH * 2));
}


/*
 * Runs two pixel back and forth in opposite directions.
 */
uint16_t WS2812FX::mode_dual_scan(void) {
  if(SEGMENT_RUNTIME.counter_mode_step > (SEGMENT_LENGTH * 2) - 3) {
    SEGMENT_RUNTIME.counter_mode_step = 0;
  }

  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, BLACK);
  }

  int led_offset = SEGMENT_RUNTIME.counter_mode_step - (SEGMENT_LENGTH - 1);
  led_offset = abs(led_offset);

  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + led_offset, SEGMENT.colors[0]);
  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + SEGMENT_LENGTH - led_offset - 1, SEGMENT.colors[0]);

  SEGMENT_RUNTIME.counter_mode_step++;
  return (SEGMENT.speed / (SEGMENT_LENGTH * 2));
}


/*
 * Cycles all LEDs at once through a rainbow.
 */
uint16_t WS2812FX::mode_rainbow(void) {
  uint32_t color = color_wheel(SEGMENT_RUNTIME.counter_mode_step);
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, color);
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) & 0xFF;
  return (SEGMENT.speed / 256);
}


/*
 * Cycles a rainbow over the entire string of LEDs.
 */
uint16_t WS2812FX::mode_rainbow_cycle(void) {
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++) {
	  uint32_t color = color_wheel(((i * 256 / SEGMENT_LENGTH) + SEGMENT_RUNTIME.counter_mode_step) & 0xFF);
    Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, color);
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) & 0xFF;
  return (SEGMENT.speed / 256);
}


/*
 * theater chase function
 */
uint16_t WS2812FX::theater_chase(uint32_t color1, uint32_t color2) {
  SEGMENT_RUNTIME.counter_mode_call = SEGMENT_RUNTIME.counter_mode_call % 3;
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++) {
    if((i % 3) == SEGMENT_RUNTIME.counter_mode_call) {
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - i, color1);
      } else {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, color1);
      }
    } else {
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - i, color2);
      } else {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, color2);
      }
    }
  }

  return (SEGMENT.speed / SEGMENT_LENGTH);
}


/*
 * Theatre-style crawling lights.
 * Inspired by the Adafruit examples.
 */
uint16_t WS2812FX::mode_theater_chase(void) {
  return theater_chase(SEGMENT.colors[0], BLACK);
}


/*
 * Theatre-style crawling lights with rainbow effect.
 * Inspired by the Adafruit examples.
 */
uint16_t WS2812FX::mode_theater_chase_rainbow(void) {
  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) & 0xFF;
  return theater_chase(color_wheel(SEGMENT_RUNTIME.counter_mode_step), BLACK);
}


/*
 * Running lights effect with smooth sine transition.
 */
uint16_t WS2812FX::mode_running_lights(void) {
  uint8_t r = ((SEGMENT.colors[0] >> 16) & 0xFF);
  uint8_t g = ((SEGMENT.colors[0] >> 8) & 0xFF);
  uint8_t b = (SEGMENT.colors[0] & 0xFF);

  for(uint16_t i=0; i < SEGMENT_LENGTH; i++) {
    int s = (sin(i + SEGMENT_RUNTIME.counter_mode_call) * 127) + 128;
    if(SEGMENT.reverse) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, (((uint32_t)(r * s)) / 255), (((uint32_t)(g * s)) / 255), (((uint32_t)(b * s)) / 255));
    } else {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - i, (((uint32_t)(r * s)) / 255), (((uint32_t)(g * s)) / 255), (((uint32_t)(b * s)) / 255));
    }
  }
  return (SEGMENT.speed / SEGMENT_LENGTH);
}


/*
 * twinkle function
 */
uint16_t WS2812FX::twinkle(uint32_t color) {
  if(SEGMENT_RUNTIME.counter_mode_step == 0) {
    for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
      Adafruit_NeoPixel::setPixelColor(i, BLACK);
    }
    uint16_t min_leds = max(1, SEGMENT_LENGTH / 5); // make sure, at least one LED is on
    uint16_t max_leds = max(1, SEGMENT_LENGTH / 2); // make sure, at least one LED is on
    SEGMENT_RUNTIME.counter_mode_step = random(min_leds, max_leds);
  }

  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + random(SEGMENT_LENGTH), color);

  SEGMENT_RUNTIME.counter_mode_step--;
  return (SEGMENT.speed);
}

/*
 * Blink several LEDs on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 */
uint16_t WS2812FX::mode_twinkle(void) {
  return twinkle(SEGMENT.colors[0]);
}

/*
 * Blink several LEDs in random colors on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 */
uint16_t WS2812FX::mode_twinkle_random(void) {
  return twinkle(color_wheel(random(256)));
}


/*
 * fade out function
 * fades out the current segment by dividing each pixel's intensity by 2
 */
void WS2812FX::fade_out() {
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    uint32_t px_rgb = Adafruit_NeoPixel::getPixelColor(i);
    px_rgb = (px_rgb >> 1) & 0x007F7F7F;
    Adafruit_NeoPixel::setPixelColor(i, px_rgb);
  }
}

/*
 * twinkle_fade function
 */
uint16_t WS2812FX::twinkle_fade(uint32_t color) {
  fade_out();

  if(random(3) == 0) {
    Adafruit_NeoPixel::setPixelColor(SEGMENT.start + random(SEGMENT_LENGTH), color);
  }
  return (SEGMENT.speed / 8);
}


/*
 * Blink several LEDs on, fading out.
 */
uint16_t WS2812FX::mode_twinkle_fade(void) {
  return twinkle_fade(SEGMENT.colors[0]);
}


/*
 * Blink several LEDs in random colors on, fading out.
 */
uint16_t WS2812FX::mode_twinkle_fade_random(void) {
  return twinkle_fade(color_wheel(random(256)));
}


/*
 * Blinks one LED at a time.
 * Inspired by www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 */
uint16_t WS2812FX::mode_sparkle(void) {
  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + SEGMENT_RUNTIME.aux_param, BLACK);
  SEGMENT_RUNTIME.aux_param = random(SEGMENT_LENGTH); // aux_param stores the random led index
  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + SEGMENT_RUNTIME.aux_param, SEGMENT.colors[0]);
  return (SEGMENT.speed);
}


/*
 * Lights all LEDs in the color. Flashes single white pixels randomly.
 * Inspired by www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 */
uint16_t WS2812FX::mode_flash_sparkle(void) {
  if(SEGMENT_RUNTIME.counter_mode_call == 0) {
    for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
      Adafruit_NeoPixel::setPixelColor(i, SEGMENT.colors[0]);
    }
  }

  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + SEGMENT_RUNTIME.aux_param, SEGMENT.colors[0]);

  if(random(5) == 0) {
    SEGMENT_RUNTIME.aux_param = random(SEGMENT_LENGTH); // aux_param stores the random led index
    Adafruit_NeoPixel::setPixelColor(SEGMENT.start + SEGMENT_RUNTIME.aux_param, WHITE);
    return 20;
  } 
  return SEGMENT.speed;
}


/*
 * Like flash sparkle. With more flash.
 * Inspired by www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 */
uint16_t WS2812FX::mode_hyper_sparkle(void) {
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, SEGMENT.colors[0]);
  }

  if(random(5) < 2) {
    for(uint16_t i=0; i < max(1, SEGMENT_LENGTH/3); i++) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + random(SEGMENT_LENGTH), WHITE);
    }
    return 20;
  }
  return SEGMENT.speed;
}


/*
 * Strobe effect with different strobe count and pause, controlled by speed.
 */
uint16_t WS2812FX::mode_multi_strobe(void) {
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, BLACK);
  }

  uint16_t delay = SEGMENT.speed / (2 * ((SEGMENT.speed / 10) + 1));
  if(SEGMENT_RUNTIME.counter_mode_step < (2 * ((SEGMENT.speed / 10) + 1))) {
    if((SEGMENT_RUNTIME.counter_mode_step & 1) == 0) {
      for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
        Adafruit_NeoPixel::setPixelColor(i, SEGMENT.colors[0]);
      }
      delay = 20;
    } else {
      delay = 50;
    }
  }
  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % ((2 * ((SEGMENT.speed / 10) + 1)) + 1);
  return delay;
}


/*
 * color chase function.
 * color1 = background color
 * color2 and color3 = colors of two adjacent leds
 */

uint16_t WS2812FX::chase(uint32_t color1, uint32_t color2, uint32_t color3) {
  uint16_t a = SEGMENT_RUNTIME.counter_mode_step;
  uint16_t b = (a + 1) % SEGMENT_LENGTH;
  uint16_t c = (b + 1) % SEGMENT_LENGTH;
  if(SEGMENT.reverse) {
    Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - a, color1);
    Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - b, color2);
    Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - c, color3);
  } else {
    Adafruit_NeoPixel::setPixelColor(SEGMENT.start + a, color1);
    Adafruit_NeoPixel::setPixelColor(SEGMENT.start + b, color2);
    Adafruit_NeoPixel::setPixelColor(SEGMENT.start + c, color3);
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % SEGMENT_LENGTH;
  return (SEGMENT.speed / SEGMENT_LENGTH);
}


/*
 * Bicolor chase mode
 */
uint16_t WS2812FX::mode_bicolor_chase(void) {
  return chase(SEGMENT.colors[0], SEGMENT.colors[1], SEGMENT.colors[2]);
}


/*
 * White running on _color.
 */
uint16_t WS2812FX::mode_chase_color(void) {
  return chase(SEGMENT.colors[0], WHITE, WHITE);
}


/*
 * Black running on _color.
 */
uint16_t WS2812FX::mode_chase_blackout(void) {
  return chase(SEGMENT.colors[0], BLACK, BLACK);
}


/*
 * _color running on white.
 */
uint16_t WS2812FX::mode_chase_white(void) {
  return chase(WHITE, SEGMENT.colors[0], SEGMENT.colors[0]);
}


/*
 * White running followed by random color.
 */
uint16_t WS2812FX::mode_chase_random(void) {
  if(SEGMENT_RUNTIME.counter_mode_step == 0) {
    SEGMENT_RUNTIME.aux_param = get_random_wheel_index(SEGMENT_RUNTIME.aux_param);
  }
  return chase(color_wheel(SEGMENT_RUNTIME.aux_param), WHITE, WHITE);
}


/*
 * Rainbow running on white.
 */
uint16_t WS2812FX::mode_chase_rainbow_white(void) {
  uint16_t n = SEGMENT_RUNTIME.counter_mode_step;
  uint16_t m = (SEGMENT_RUNTIME.counter_mode_step + 1) % SEGMENT_LENGTH;
  uint32_t color2 = color_wheel(((n * 256 / SEGMENT_LENGTH) + (SEGMENT_RUNTIME.counter_mode_call & 0xFF)) & 0xFF);
  uint32_t color3 = color_wheel(((m * 256 / SEGMENT_LENGTH) + (SEGMENT_RUNTIME.counter_mode_call & 0xFF)) & 0xFF);

  return chase(WHITE, color2, color3);
}


/*
 * White running on rainbow.
 */
uint16_t WS2812FX::mode_chase_rainbow(void) {
  uint8_t color_sep = 256 / SEGMENT_LENGTH;
  uint8_t color_index = SEGMENT_RUNTIME.counter_mode_call & 0xFF;
  uint32_t color = color_wheel(((SEGMENT_RUNTIME.counter_mode_step * color_sep) + color_index) & 0xFF);

  return chase(color, WHITE, WHITE);
}


/*
 * Black running on rainbow.
 */
uint16_t WS2812FX::mode_chase_blackout_rainbow(void) {
  uint8_t color_sep = 256 / SEGMENT_LENGTH;
  uint8_t color_index = SEGMENT_RUNTIME.counter_mode_call & 0xFF;
  uint32_t color = color_wheel(((SEGMENT_RUNTIME.counter_mode_step * color_sep) + color_index) & 0xFF);

  return chase(color, BLACK, BLACK);
}


/*
 * White flashes running on _color.
 */
uint16_t WS2812FX::mode_chase_flash(void) {
  const static uint8_t flash_count = 4;
  uint8_t flash_step = SEGMENT_RUNTIME.counter_mode_call % ((flash_count * 2) + 1);

  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    Adafruit_NeoPixel::setPixelColor(i, SEGMENT.colors[0]);
  }

  uint16_t delay = (SEGMENT.speed / SEGMENT_LENGTH);
  if(flash_step < (flash_count * 2)) {
    if(flash_step % 2 == 0) {
      uint16_t n = SEGMENT_RUNTIME.counter_mode_step;
      uint16_t m = (SEGMENT_RUNTIME.counter_mode_step + 1) % SEGMENT_LENGTH;
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - n, WHITE);
        Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - m, WHITE);
      } else {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + n, WHITE);
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + m, WHITE);
      }
      delay = 20;
    } else {
      delay = 30;
    }
  } else {
    SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % SEGMENT_LENGTH;
  }
  return delay;
}


/*
 * White flashes running, followed by random color.
 */
uint16_t WS2812FX::mode_chase_flash_random(void) {
  const static uint8_t flash_count = 4;
  uint8_t flash_step = SEGMENT_RUNTIME.counter_mode_call % ((flash_count * 2) + 1);

  for(uint16_t i=0; i < SEGMENT_RUNTIME.counter_mode_step; i++) {
    Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, color_wheel(SEGMENT_RUNTIME.aux_param));
  }

  uint16_t delay = (SEGMENT.speed / SEGMENT_LENGTH);
  if(flash_step < (flash_count * 2)) {
    uint16_t n = SEGMENT_RUNTIME.counter_mode_step;
    uint16_t m = (SEGMENT_RUNTIME.counter_mode_step + 1) % SEGMENT_LENGTH;
    if(flash_step % 2 == 0) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + n, WHITE);
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + m, WHITE);
      delay = 20;
    } else {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + n, color_wheel(SEGMENT_RUNTIME.aux_param));
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + m, BLACK);
      delay = 30;
    }
  } else {
    SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % SEGMENT_LENGTH;

    if(SEGMENT_RUNTIME.counter_mode_step == 0) {
      SEGMENT_RUNTIME.aux_param = get_random_wheel_index(SEGMENT_RUNTIME.aux_param);
    }
  }
  return delay;
}


/*
 * Alternating pixels running function.
 */
uint16_t WS2812FX::running(uint32_t color1, uint32_t color2) {
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++) {
    if((i + SEGMENT_RUNTIME.counter_mode_step) % 4 < 2) {
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, color1);
      } else {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - i, color1);
      }
    } else {
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, color2);
      } else {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - i, color2);
      }
    }
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) & 0x3;
  return (SEGMENT.speed / SEGMENT_LENGTH);
}

/*
 * Alternating color/white pixels running.
 */
uint16_t WS2812FX::mode_running_color(void) {
  return running(SEGMENT.colors[0], WHITE);
}


/*
 * Alternating red/blue pixels running.
 */
uint16_t WS2812FX::mode_running_red_blue(void) {
  return running(RED, BLUE);
}


/*
 * Alternating red/green pixels running.
 */
uint16_t WS2812FX::mode_merry_christmas(void) {
  return running(RED, GREEN);
}

/*
 * Alternating orange/purple pixels running.
 */
uint16_t WS2812FX::mode_halloween(void) {
  return running(PURPLE, ORANGE);
}


/*
 * Random colored pixels running.
 */
uint16_t WS2812FX::mode_running_random(void) {
  for(uint16_t i=SEGMENT_LENGTH-1; i > 0; i--) {
    if(SEGMENT.reverse) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - i, Adafruit_NeoPixel::getPixelColor(SEGMENT.stop - i + 1));
    } else {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, Adafruit_NeoPixel::getPixelColor(SEGMENT.start + i - 1));
    }
  }

  if(SEGMENT_RUNTIME.counter_mode_step == 0) {
    SEGMENT_RUNTIME.aux_param = get_random_wheel_index(SEGMENT_RUNTIME.aux_param);
    if(SEGMENT.reverse) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.stop, color_wheel(SEGMENT_RUNTIME.aux_param));
    } else {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start, color_wheel(SEGMENT_RUNTIME.aux_param));
    }
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step == 0) ? 1 : 0;
  return (SEGMENT.speed / SEGMENT_LENGTH);
}


/*
 * K.I.T.T.
 */
uint16_t WS2812FX::mode_larson_scanner(void) {
  fade_out();

  if(SEGMENT_RUNTIME.counter_mode_step < SEGMENT_LENGTH) {
    if(SEGMENT.reverse) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - SEGMENT_RUNTIME.counter_mode_step, SEGMENT.colors[0]);
    } else {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + SEGMENT_RUNTIME.counter_mode_step, SEGMENT.colors[0]);
    }
  } else {
    if(SEGMENT.reverse) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - ((SEGMENT_LENGTH * 2) - SEGMENT_RUNTIME.counter_mode_step) + 2, SEGMENT.colors[0]);
    } else {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + ((SEGMENT_LENGTH * 2) - SEGMENT_RUNTIME.counter_mode_step) - 2, SEGMENT.colors[0]);
    }
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % ((SEGMENT_LENGTH * 2) - 2);
  return (SEGMENT.speed / (SEGMENT_LENGTH * 2));
}


/*
 * Firing comets from one end.
 */
uint16_t WS2812FX::mode_comet(void) {
  fade_out();

  if(SEGMENT.reverse) {
    Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - SEGMENT_RUNTIME.counter_mode_step, SEGMENT.colors[0]);
  } else {
    Adafruit_NeoPixel::setPixelColor(SEGMENT.start + SEGMENT_RUNTIME.counter_mode_step, SEGMENT.colors[0]);
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % SEGMENT_LENGTH;
  return (SEGMENT.speed / SEGMENT_LENGTH);
}


/*
 * Fireworks function.
 */
uint16_t WS2812FX::fireworks(uint32_t color) {
  fade_out();

  uint32_t px_rgb = 0;
  byte px_r = 0;
  byte px_g = 0;
  byte px_b = 0;

  // first LED has only one neighbour
  px_r = (((Adafruit_NeoPixel::getPixelColor(SEGMENT.start+1) & 0xFF0000) >> 16) >> 1) + ((Adafruit_NeoPixel::getPixelColor(SEGMENT.start) & 0xFF0000) >> 16);
  px_g = (((Adafruit_NeoPixel::getPixelColor(SEGMENT.start+1) & 0x00FF00) >>  8) >> 1) + ((Adafruit_NeoPixel::getPixelColor(SEGMENT.start) & 0x00FF00) >>  8);
  px_b = (((Adafruit_NeoPixel::getPixelColor(SEGMENT.start+1) & 0x0000FF)      ) >> 1) + ((Adafruit_NeoPixel::getPixelColor(SEGMENT.start) & 0x0000FF));
  Adafruit_NeoPixel::setPixelColor(SEGMENT.start, px_r, px_g, px_b);

  // set brightness(i) = ((brightness(i-1)/2 + brightness(i+1)) / 2) + brightness(i)
  for(uint16_t i=SEGMENT.start+1; i <= SEGMENT.stop-1; i++) {
    px_r = ((
            (((Adafruit_NeoPixel::getPixelColor(i-1) & 0xFF0000) >> 16) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(i+1) & 0xFF0000) >> 16)     ) ) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(i  ) & 0xFF0000) >> 16)     );

    px_g = ((
            (((Adafruit_NeoPixel::getPixelColor(i-1) & 0x00FF00) >> 8) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(i+1) & 0x00FF00) >> 8)     ) ) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(i  ) & 0x00FF00) >> 8)     );

    px_b = ((
            (((Adafruit_NeoPixel::getPixelColor(i-1) & 0x0000FF)     ) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(i+1) & 0x0000FF)     )     ) ) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(i  ) & 0x0000FF)     )     );

    Adafruit_NeoPixel::setPixelColor(i, px_r, px_g, px_b);
  }

  // last LED has only one neighbour
  px_r = (((Adafruit_NeoPixel::getPixelColor(SEGMENT.stop-1) & 0xFF0000) >> 16) >> 2) + ((Adafruit_NeoPixel::getPixelColor(SEGMENT.stop) & 0xFF0000) >> 16);
  px_g = (((Adafruit_NeoPixel::getPixelColor(SEGMENT.stop-1) & 0x00FF00) >>  8) >> 2) + ((Adafruit_NeoPixel::getPixelColor(SEGMENT.stop) & 0x00FF00) >>  8);
  px_b = (((Adafruit_NeoPixel::getPixelColor(SEGMENT.stop-1) & 0x0000FF)      ) >> 2) + ((Adafruit_NeoPixel::getPixelColor(SEGMENT.stop) & 0x0000FF));
  Adafruit_NeoPixel::setPixelColor(SEGMENT.stop, px_r, px_g, px_b);

  if(!_triggered) {
    for(uint16_t i=0; i<max(1, SEGMENT_LENGTH/20); i++) {
      if(random(10) == 0) {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + random(SEGMENT_LENGTH), color);
      }
    }
  } else {
    for(uint16_t i=0; i<max(1, SEGMENT_LENGTH/10); i++) {
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + random(SEGMENT_LENGTH), color);
    }
  }
  return (SEGMENT.speed / SEGMENT_LENGTH);
}


/*
 * Firework sparks.
 */
uint16_t WS2812FX::mode_fireworks(void) {
  uint32_t color = SEGMENT.colors[0];
  return fireworks(color);
}


/*
 * Random colored firework sparks.
 */
uint16_t WS2812FX::mode_fireworks_random(void) {
  uint32_t color = color_wheel(random(256));
  return fireworks(color);
}


/*
 * Fire flicker function
 */
uint16_t WS2812FX::fire_flicker(int rev_intensity) {
  byte p_r = (SEGMENT.colors[0] & 0xFF0000) >> 16;
  byte p_g = (SEGMENT.colors[0] & 0x00FF00) >>  8;
  byte p_b = (SEGMENT.colors[0] & 0x0000FF);
  byte flicker_val = max(p_r, max(p_g, p_b)) / rev_intensity;
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    int flicker = random(0, flicker_val);
    int r1 = p_r - flicker;
    int g1 = p_g - flicker;
    int b1 = p_b - flicker;
    if(g1<0) g1 = 0;
    if(r1<0) r1 = 0;
    if(b1<0) b1 = 0;
    Adafruit_NeoPixel::setPixelColor(i, r1, g1, b1);
  }
  return (SEGMENT.speed);
}

/*
 * Random flickering.
 */
uint16_t WS2812FX::mode_fire_flicker(void) {
  return fire_flicker(3);
}

/*
* Random flickering, less intesity.
*/
uint16_t WS2812FX::mode_fire_flicker_soft(void) {
  return fire_flicker(6);
}

/*
* Random flickering, more intesity.
*/
uint16_t WS2812FX::mode_fire_flicker_intense(void) {
  return fire_flicker(1.7);
}


/*
 * Tricolor chase function
 */
uint16_t WS2812FX::tricolor_chase(uint32_t color1, uint32_t color2, uint32_t color3) {
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++) {
    if((i + SEGMENT_RUNTIME.counter_mode_step) % 6 < 2) {
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, color1);
      } else {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - i, color1);
      }
    } else if((i + SEGMENT_RUNTIME.counter_mode_step) % 6 < 4) {
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, color2);
      } else {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - i, color2);
      }
    } else {
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.start + i, color3);
      } else {
        Adafruit_NeoPixel::setPixelColor(SEGMENT.stop - i, color3);
      }
    }
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % 6;
  return (SEGMENT.speed / SEGMENT_LENGTH);
}


/*
 * Tricolor chase mode
 */
uint16_t WS2812FX::mode_tricolor_chase(void) {
  return tricolor_chase(SEGMENT.colors[0], SEGMENT.colors[1], SEGMENT.colors[2]);
}


/*
 * Alternating white/red/black pixels running.
 */
uint16_t WS2812FX::mode_circus_combustus(void) {
  return tricolor_chase(RED, WHITE, BLACK);
}

/*
 * ICU mode
 */
uint16_t WS2812FX::mode_icu() {
  uint16_t dest = SEGMENT_RUNTIME.counter_mode_step & 0xFFFF;
 
  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + dest, SEGMENT.colors[0]);
  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + dest + SEGMENT_LENGTH/2, SEGMENT.colors[0]);

  if(SEGMENT_RUNTIME.aux_param == dest) { // pause between eye movements
    if(random(6) == 0) { // blink once in a while
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + dest, 0);
      Adafruit_NeoPixel::setPixelColor(SEGMENT.start + dest + SEGMENT_LENGTH/2, 0);
      return 200;
    }
    SEGMENT_RUNTIME.aux_param = random(SEGMENT_LENGTH/2);
    return 1000 + random(2000);
  }

  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + dest, 0);
  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + dest + SEGMENT_LENGTH/2, 0);

  if(SEGMENT_RUNTIME.aux_param > SEGMENT_RUNTIME.counter_mode_step) {
    SEGMENT_RUNTIME.counter_mode_step++;
    dest++;
  } else if (SEGMENT_RUNTIME.aux_param < SEGMENT_RUNTIME.counter_mode_step) {
    SEGMENT_RUNTIME.counter_mode_step--;
    dest--;
  }

  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + dest, SEGMENT.colors[0]);
  Adafruit_NeoPixel::setPixelColor(SEGMENT.start + dest + SEGMENT_LENGTH/2, SEGMENT.colors[0]);

  return (SEGMENT.speed / SEGMENT_LENGTH);
}

//kirov

uint16_t WS2812FX::remap_pos(uint16_t pos){
  uint16_t col = pos % 30;
  uint16_t row = pos / 30;
  if (col % 2 == 0) { // четная
  } else { // нечетная
    row = 9 - row;
  }
  return 10 * col + row;
}

uint16_t WS2812FX::map_pos(uint16_t x, uint16_t y){
  uint16_t col = x;
  uint16_t row = y;
  if (col % 2 == 0) { // четная
  } else { // нечетная
    row = 9 - row;
  }
  return 10 * col + row;
}

uint32_t WS2812FX::fade_color(uint32_t color, uint8_t level) {
  int b = level - 31;
  b = 63 - (abs(b) * 2);
  b = map(b, 0, 64, min(25, _brightness), _brightness);

  uint8_t red = (color >> 16 & 0xFF) * b / _brightness; // modify RGB colors with brightness info
  uint8_t green = (color >> 8 & 0xFF) * b / _brightness;
  uint8_t blue = (color & 0xFF) * b / _brightness;
  return red*65536 + green * 256 + blue;
}


uint16_t WS2812FX::mode_ext_circle(void) {
  for(uint16_t i=0; i < SEGMENT_LENGTH; i++) {
    uint32_t color = color_wheel(((i * 256 / SEGMENT_LENGTH) + SEGMENT_RUNTIME.counter_mode_step) & 0xFF);
    uint16_t pos = remap_pos(SEGMENT.start + i);
    Adafruit_NeoPixel::setPixelColor(pos, color);
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) & 0xFF;
  return (SEGMENT.speed / 256);
}

uint16_t WS2812FX::mode_ext_larson(void) {
  uint32_t color = color_wheel(random(256)); //SEGMENT.colors[0]
  fade_out();
  uint16_t len = SEGMENT_LENGTH / 10;
  uint16_t start = 0;
  uint16_t stop = 9;
  for(uint16_t i=0; i < 30; i++) {
    if(SEGMENT_RUNTIME.counter_mode_step < len) {
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(remap_pos(i + 30*(stop - SEGMENT_RUNTIME.counter_mode_step)), color);
      } else {
        Adafruit_NeoPixel::setPixelColor(remap_pos(i + 30*(start + SEGMENT_RUNTIME.counter_mode_step)), color);
      }
    } else {
      if(SEGMENT.reverse) {
        Adafruit_NeoPixel::setPixelColor(remap_pos(i + 30*(stop - ((len * 2) - SEGMENT_RUNTIME.counter_mode_step) + 2)), color);
      } else {
        Adafruit_NeoPixel::setPixelColor(remap_pos(i + 30*(start + ((len * 2) - SEGMENT_RUNTIME.counter_mode_step) - 2)), color);
      }
    }
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % ((len * 2) - 2);
  return (SEGMENT.speed / (len * 2));
}

uint16_t WS2812FX::mode_ext_fireworks(void) {
  uint32_t color = color_wheel(random(256));
  fade_out();

  uint32_t px_rgb = 0;
  byte px_r = 0;
  byte px_g = 0;
  byte px_b = 0;

  // first LED has only one neighbour
  px_r = (((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.start+1)) & 0xFF0000) >> 16) >> 1) + ((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.start)) & 0xFF0000) >> 16);
  px_g = (((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.start+1)) & 0x00FF00) >>  8) >> 1) + ((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.start)) & 0x00FF00) >>  8);
  px_b = (((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.start+1)) & 0x0000FF)      ) >> 1) + ((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.start)) & 0x0000FF));
  Adafruit_NeoPixel::setPixelColor(remap_pos(SEGMENT.start), px_r, px_g, px_b);

  // set brightness(i) = ((brightness(i-1)/2 + brightness(i+1)) / 2) + brightness(i)
  for(uint16_t i=SEGMENT.start+1; i <= SEGMENT.stop-1; i++) {
    px_r = ((
            (((Adafruit_NeoPixel::getPixelColor(remap_pos(i-1)) & 0xFF0000) >> 16) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(remap_pos(i+1)) & 0xFF0000) >> 16)     ) ) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(remap_pos(i  )) & 0xFF0000) >> 16)     );

    px_g = ((
            (((Adafruit_NeoPixel::getPixelColor(remap_pos(i-1)) & 0x00FF00) >> 8) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(remap_pos(i+1)) & 0x00FF00) >> 8)     ) ) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(remap_pos(i  )) & 0x00FF00) >> 8)     );

    px_b = ((
            (((Adafruit_NeoPixel::getPixelColor(remap_pos(i-1)) & 0x0000FF)     ) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(remap_pos(i+1)) & 0x0000FF)     )     ) ) >> 1) +
            (((Adafruit_NeoPixel::getPixelColor(remap_pos(i  )) & 0x0000FF)     )     );

    Adafruit_NeoPixel::setPixelColor(remap_pos(i), px_r, px_g, px_b);
  }

  // last LED has only one neighbour
  px_r = (((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.stop-1)) & 0xFF0000) >> 16) >> 2) + ((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.stop)) & 0xFF0000) >> 16);
  px_g = (((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.stop-1)) & 0x00FF00) >>  8) >> 2) + ((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.stop)) & 0x00FF00) >>  8);
  px_b = (((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.stop-1)) & 0x0000FF)      ) >> 2) + ((Adafruit_NeoPixel::getPixelColor(remap_pos(SEGMENT.stop)) & 0x0000FF));
  Adafruit_NeoPixel::setPixelColor(remap_pos(SEGMENT.stop), px_r, px_g, px_b);

  if(!_triggered) {
    for(uint16_t i=0; i<max(1, SEGMENT_LENGTH/20); i++) {
      if(random(10) == 0) {
        Adafruit_NeoPixel::setPixelColor(remap_pos(SEGMENT.start + random(SEGMENT_LENGTH)), color);
      }
    }
  } else {
    for(uint16_t i=0; i<max(1, SEGMENT_LENGTH/10); i++) {
      Adafruit_NeoPixel::setPixelColor(remap_pos(SEGMENT.start + random(SEGMENT_LENGTH)), color);
    }
  }
  return (SEGMENT.speed / SEGMENT_LENGTH);
}

uint16_t WS2812FX::mode_ext_star(void) {  
  uint16_t rays = 5;
  Adafruit_NeoPixel::clear();
//  fade_out();
//  fade_out();
//  fade_out();
  uint16_t stp = 30 / rays;
  uint32_t color = color_wheel((SEGMENT_RUNTIME.counter_mode_step) & 0xFF); //color_wheel(random(256));// SEGMENT.colors[0];
  for(uint16_t i=0; i < rays; i++) {
    uint16_t x = (stp * i + SEGMENT_RUNTIME.counter_mode_step) % 30; 
    for(uint16_t y=0; y < 10; y++) {
      uint16_t ln = (3*y) / stp;
      uint16_t lev = y % stp;
      uint16_t pos = map_pos(x, y);
      //uint32_t color2 = fade_color(color, 10*lev);
      Adafruit_NeoPixel::setPixelColor(pos, color);
      for(uint16_t j=0; j < ln; j++) {
        pos = map_pos((30 + x - j) % 30, y);
        Adafruit_NeoPixel::setPixelColor(pos, color);
        pos = map_pos((30 + x + j) % 30, y);
        Adafruit_NeoPixel::setPixelColor(pos, color);
      }
    }
  }

  SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) & 0xFF;
  return (SEGMENT.speed / 64);
}
