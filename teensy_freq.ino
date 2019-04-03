/*  Glenn's Music visualizer.
 *   1024 point FFT for better resolution.
 *   Chaining the two 64x32s sorta works, but if you set the width to 128, the FFT never complete
 *   (guessing interrupt contention).  96 works, but that then has a weird overlap.
 *   So, I'm sticking with just one 64 x 32.
 *   Doing bars of widtch 3, which means 21 bins (64/3 = 21.3)
 */

// all these libraries are required for the Teensy Audio Library
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include <SmartLEDShieldV4.h>  // comment out this line for if you're not using SmartLED Shield V4 hardware (this line needs to be before #include <SmartMatrix3.h>)
#include <SmartMatrix3.h>
#include <FastLED.h>

#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
const uint8_t kMatrixWidth = 64;        // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = 32;       // known working: 16, 32, 48, 64
const uint8_t kRefreshDepth = 36;       // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN; // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels, or use SMARTMATRIX_HUB75_64ROW_MOD32SCAN for common 64x64 panels
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
//SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);

#define ADC_INPUT_PIN   A2

AudioInputAnalog         input(ADC_INPUT_PIN);
AudioAnalyzeFFT1024      fft;
AudioConnection          audioConnection(input, 0, fft, 0);

CRGB palette[21];

#define FREQ_BINS 21

// An array to hold the frequency bins
float level[FREQ_BINS];

// This array holds the on-screen levels.  When the signal drops quickly,
// these are used to lower the on-screen level 1 bar per update, which
// looks more pleasing to corresponds to human sound perception.
int freq_hist[FREQ_BINS];


void init_palette( void )
{
  fill_gradient_RGB(palette, 7, CRGB::Purple, CRGB::Blue);
  fill_gradient_RGB(&(palette[7]), 7, CRGB::Green, CRGB::Yellow);
  fill_gradient_RGB(&(palette[14]),7, CRGB::Yellow, CRGB::Red);
}

void setup()
{
    Serial.begin(9600);
    Wire.begin();

    init_palette();

    // Initialize Matrix
    matrix.addLayer(&backgroundLayer); 
    //matrix.addLayer(&scrollingLayer);
    matrix.begin();

    matrix.setBrightness(255);

    // Audio requires memory to work.
    AudioMemory(12);

}

void print_fft( void )
{
  int i;
  int temp_level;

  for (i=0; i<64; i++)
  {

    
    Serial.print("Bin ");
    Serial.print(i);
    Serial.print("= ");
    Serial.println(fft.read(i));
  }

  Serial.println("============");
}


void print_levels( void )
{
  int i;
  
  for (i=0; i<FREQ_BINS; i++)
  {  
    Serial.print("Bin ");
    Serial.print(i);
    Serial.print("= ");
    Serial.println(level[i]);
  }

  Serial.println("============");
}

void display_freq_raw( void )
{
  int i;
  int mag;
  int bin;
  int x;
  rgb24 black = {0,0,0};

  backgroundLayer.fillScreen(black);

  for (i = 0; i < FREQ_BINS; i++)
  {
    // level[i] is an adjusted float...looks like a gain of about 100 will be right, but I want to play with that.
    mag = level[i];
    if (mag > 31) mag = 31;
    if (mag < 0) mag = 0;

    x = i*3;

    backgroundLayer.drawRectangle(x, 32, x+2, 31-mag, palette[i%21]);
  }
  
  backgroundLayer.swapBuffers();
  
}


void display_freq_decay( void )
{
  int i;
  int mag;
  int bin;
  int x;
  rgb24 color = {0, 0, 50};
  rgb24 black = {0,0,0};

  backgroundLayer.fillScreen(black);

  for (i = 0; i < FREQ_BINS; i++)
  {
    // level[i] is an adjusted float...looks like a gain of about 100 will be right, but I want to play with that.
    mag = level[i];
    if (mag > 31) mag = 31;
    if (mag < 0) mag = 0;

    // check if current magnitude is smaller than our recent history.
    if (mag < freq_hist[i])
    {
      mag = freq_hist[i] - 1;
    }

    // store new value...either new max or "decayed" value
    freq_hist[i] = mag;

    x = i*3;

    backgroundLayer.drawRectangle(x, 32, x+2, 31-mag, palette[i%21]);
  }
  
  backgroundLayer.swapBuffers();
  
}
void update_levels( void )
{
  int i;
  
  // I'm adding together 3 fft bins to increase the range.
  for (i = 0; i < FREQ_BINS; i++)
  {
    level[i] = 100.0 * fft.read(i*3, i*3+2);
  }
}


void loop()
{
    int bin;
    int x;
    int y;
    
    if (fft.available()) 
    {
        //update_levels();
        //print_levels();
        //while (!Serial.available());
        //while (Serial.available()) char c=Serial.read();
       
        update_levels();
        display_freq_decay();
    }

}
