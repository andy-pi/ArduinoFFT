/*
Title:    AndyPi Spectrum Analyser for Arduino Nano
Code by:  Andrew Wilsion http://andypi.co.uk
Date:     21/09/2014
Verision: 1.0

Code based on:
OpenMusicLabs.com FHT example; Adafruit WS2801 Library & example
NOTE: FastLED library does NOT work due to memory conflicts

Hardware:
Arduino Nano
WS2801 pixel strip: data pin 13; clock pin 11
Adafruit electret mic: A0 pin
*/

#include "Adafruit_WS2801.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
Adafruit_WS2801 strip = Adafruit_WS2801(6); // setup WS2801 strip with 6 pixels

#define OCTAVE 1 //   // Group buckets into octaves  (use the log output function LOG_OUT 1)
#define OCT_NORM 0 // Don't normalise octave intensities by number of bins
#define FHT_N 256 // set to 256 point fht
#include <FHT.h> // include the library
int noise[]={204,188,68,73,150,120,88,68}; // noise level determined by playing pink noise and seeing levels [trial and error]{204,188,68,73,150,98,88,68}

void setup() {
  Serial.begin(115200); // use the serial port
  strip.begin();  // initialise WS2801 strip
  strip.show(); // Update LED contents, to start they are all 'off'
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0
}

void loop() {

  // Start of Fourier Transform code; takes input on ADC from mic
  
  while(1) { // reduces jitter
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < FHT_N ; i++) { // save 256 samples
      while(!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fht_input[i] = k; // put real data into bins
    }
    fht_window(); // window the data for better frequency response
    fht_reorder(); // reorder the data before doing the fht
    fht_run(); // process the data in the fht
    fht_mag_octave(); // take the output of the fht  fht_mag_log()
    sei();

    // End of Fourier Transform code - output is stored in fht_oct_out[i]. 

    // i=0-7 frequency (octave) bins (don't use 0 or 1), fht_oct_out[1]= amplitude of frequency for bin 1
    // for loop a) removes background noise average and takes absolute value b) low / high pass filter as still very noisy
    // c) maps amplitude of octave to a colour between blue and red d) sets pixel colour to amplitude of each frequency (octave)
    
     int fht_noise_adjusted[8];
     int ColourSpectrum[8];
     for (int i = 2; i < 8; i++) {  // For each of the 6 useful octave bins
       fht_noise_adjusted[i] = abs(fht_oct_out[i]-noise[i]);  // take the pink noise average level out, take the asbolute value to avoid negative numbers
       fht_noise_adjusted[i] = constrain(fht_noise_adjusted[i], 37, 125); // 37 lowpass for noise; 125 high pass doesn't go much higher than this [found by trial and error]
       ColourSpectrum[i] = map(fht_noise_adjusted[i], 37, 125, 160, 0); // map to values 0 - 160, i.e. blue to red on colour spectrum - larger range gives more colour variability [found by trial and error]
       strip.setPixelColor((i-2), Wheel(ColourSpectrum[i]));  // set each pixels colour to the amplitude of that particular octave
     }  
     
  strip.show(); // update the LEDs

  }
}




/* Helper functions to create colours */

// Create a 24 bit color value from R,G,B
uint32_t Color(byte g, byte r, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

