uint8_t brightness = 2; //this is not right at all.
uint8_t demo = 0;
uint8_t compassdebug = 0;
uint8_t framerate= 120; // SIESURE WARNING?
uint8_t colorschemeselector = 2;
void (*renderEffect[])(byte) = {
  //PeteOV,
  // sineCompass, //need to get it built before we can learn the compass
  //sparkle, //need to make this look better, probably looks sweet when moving fas
  //##########in development###########
  // twoatonce,
  Dice,
  //###########good codes, dont change these#####
  hsvtest,
  //  wavyFlag,// stock
  colorDrift, // why wasn't this smooth? gamma.
  strobe, //need to have a better system for duty cycle modulation
  // SnakeChase, //serial monitor does not work with this one, too intesne
  MonsterHunter, //woah dont fuck with this guy
  pacman, //bounces back from end to end and builds every time
  rainbowChase, //stock rainbow chase doesnt work at 240 hz
  sineChase, //stock sine chase
  POV, //if using uno comment this out. 2k of ram is not enough! or is it?
  //needs to store index and message string in progmem
  fans, // varied duty cycle per led per section strobe
  skipAflash,
  RandomColorsEverywhere,
}
,
(*renderAlpha[])(void) = {
  renderAlpha00,
  renderAlpha01,
  renderAlpha02 };

//########################################################################################################################
/*
mmmaxwwwell
 7/16/12
 added singularity node's modified code for getting serial input
 https://github.com/analognode/LPD8806/blob/master/LDP8806old/examples/interactive_strand/interactive_strand.pde
 6/30/2012
 added:
 -a button on external interrupt 0 with software debounce
 -demo mode to go with above
 -some fun patterns
 -a running average that could be an array(hint hint), eventually for compass
 -a hmc5883l magnometer and heading math? in the main loop. not the best way to do it but it works. should be on an interrupt
 so nothing is in loop
 ladyada awesome job with the entire thing and thanks for the light rope!
 */
/*
Smoothing
 Reads repeatedly from an analog input, calculating a running average
 and printing it to the computer. Keeps ten readings in an array and
 continually averages them.
 The circuit:
 * Analog sensor (potentiometer will do) attached to analog input 0
 Created 22 April 2007
 By David A. Mellis <dam@mellis.org>
 modified 9 Apr 2012
 by Tom Igoe
 http://www.arduino.cc/en/Tutorial/Smoothing
 This example code is in the public domain.
 */
/*

 HMC5883L_Example.pde - Examhsvple sketch for integration with an HMC5883L triple axis magnetomerwe.
 Copyright (C) 2011 Love Electronics (loveelectronics.co.uk)
 This program is free software: you can redistribute it and/or modify
 it under the terms of the version 3 GNU General Public License as
 published by the Free Software Foundation.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************/
// Example to control LPD8806-based RGB LED Modules in a strip
// Original code by Adafruit
// SPI additions by cjbaar
/*****************************************************************************/


// Example to control LPD8806-based RGB LED Modules in a strip; originally
// intended for the Adafruit Digital Programmable LED Belt Kit.
// REQUIRES TIMER1 LIBRARY: http://www.arduino.cc/playground/Code/Timer1
// ALSO REQUIRES LPD8806 LIBRARY, which should be included with this code.

// I'm generally not fond of canned animation patterns. Wanting something
// more nuanced than the usual 8-bit beep-beep-boop-boop pixelly animation,
// this program smoothly cycles through a set of procedural animated effects
// and transitions -- it's like a Video Toaster for your waist! Some of the
// coding techniques may be a bit obtuse (e.g. function arrays), so novice
// programmers may have an easier time starting out with the 'strandtest'
// program also included with the LPD8806 library.

#include <avr/pgmspace.h>
#include "SPI.h"
#include "LPD8806.h"
#include "TimerOne.h"
#include <Wire.h>
// Declare the number of pixels in strand; 32 = 32 pixels in a row. The
// LED strips have 32 LEDs per meter, but you can extend or cut the strip.
#define numPixels 32
// 'const' makes subsequent array declarations possible, otherwise there
// would be a pile of malloc() calls later.

// Instantiate LED strip; arguments are the total number of pixels in strip,
// the data pin number and clock pin number:
LPD8806 strip = LPD8806(numPixels);
int tCounter = 0;

// You can also use hardware SPI for ultra-fast writes by omitting the data
// and clock pin arguments. This is faster, but the data and clock are then
// fixed to very specific pin numbers: on Arduino 168/328, data = pin 11,
// clock = pin 13. On Mega, data = pin 51, clock = pin 52.
//LPD8806 strip = LPD8806(numPixels);

int crazycounter;
//#####################menu stuffs
uint8_t menuphase = 0,menuphase0 = 0,menuphase1 = 0,menuphase2 = 0,menuphase3 = 0,menuphase4 = 0,menuphase5 = 0,menuphase6 = 0,menuphase7 = 0;
//##############compass maths
int plane;
boolean compassreadphase = 0;
uint16_t yzheadingdegrees,xzheadingdegrees,xyheadingdegrees,xyheadingdegreescalibrated,xyheadingdegreesmin,xyheadingdegreesmax,
xzheadingdegreescalibrated,xzheadingdegreesmin,xzheadingdegreesmax,
yzheadingdegreescalibrated,yzheadingdegreesmin,yzheadingdegreesmax,xyheadingdegreeslast,xzheadingdegreeslast,yzheadingdegreeslast;
float xyheading, xzheading ,yzheading,xyheadinglast, xzheadinglast ,yzheadinglast, xytravel,xztravel,yztravel;

//############### stuff for the averages for the compass
//const int numReadings = 100;
//int readings[numReadings]; // the readings from the analog input
//int index = 0; // the index of the current reading
//int total = 0; // the running total
//int average = 0;
//#############compass stuff
#include <HMC5883L.h>
HMC5883L compass;
uint8_t error = 0;
#define compassscale 8.1
//acceptable values are 0.88, 1.3, 1.9, 2.5, 4.0, 4.7, 5.6, 8.1

//#############software debounce for the button and button
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
uint16_t debounceDelay = 200;
uint8_t button = 0;
char serInStr[30]; // array that will hold the serial input string


#define maroon 0x800000
#define dark red 0x8B0000
#define brown 0xA52A2A
#define firebrick 0xB22222
#define crimson 0xDC143C
#define red 0xFF0000
#define tomato 0xFF6347
#define coral 0xFF7F50
#define indian red 0xCD5C5C
#define lightcoral 0xF08080
#define darksalmon 0xE9967A
#define salmon 0xFA8072
#define lightsalmon 0xFFA07A
#define orangered 0xFF4500
#define darkorange 0xFF8C00
#define orange 0xFFA500
#define gold 0xFFD700
#define dark golden rod 0xB8860B
#define goldenrod 0xDAA520
#define palegolden rod 0xEEE8AA
#define darkkhaki 0xBDB76B
#define khaki 0xF0E68C
#define olive 0x808000
#define yellow 0xFFFF00
#define yellowgreen 0x9ACD32
#define darkolive green 0x556B2F
#define olivedrab 0x6B8E23
#define lawngreen 0x7CFC00
#define chartreuse 0x7FFF00
#define greenyellow 0xADFF2F
#define darkgreen 0x006400
#define green 0x00FF00
#define forestgreen 0x228B22
#define lime 0x00FF00
#define limegreen 0x32CD32
#define lightgreen 0x90EE90
#define palegreen 0x98FB98
#define darkseagreen 0x8FBC8F
#define mediumspringgreen 0x00FA9A
#define springgreen 0x00FF7F
#define seagreen 0x2E8B57
#define mediumaquamarine 0x66CDAA
#define mediumseagreen 0x3CB371
#define lightseagreen 0x20B2AA
#define darkslategray 0x2F4F4F
#define teal 0x008080
#define darkcyan 0x008B8B
#define aqua 0x00FFFF
#define cyan 0x00FFFF
#define lightcyan 0xE0FFFF
#define darkturquoise 0x00CED1
#define turquoise 0x40E0D0
#define mediumturquoise 0x48D1CC
#define paleturquoise 0xAFEEEE
#define aquamarine 0x7FFFD4
#define powderblue 0xB0E0E6
#define cadetblue 0x5F9EA0
#define steelblue 0x4682B4
#define cornflowerblue 0x6495ED
#define deepskyblue 0x00BFFF
#define dodgerblue 0x1E90FF
#define lightblue 0xADD8E6
#define skyblue 0x87CEEB
#define lightskyblue 0x87CEFA
#define midnightblue 0x191970
#define navy 0x000080
#define darkblue 0x00008B
#define mediumblue 0x0000CD
#define blue 0x0000FF
#define royalblue 0x4169E1
#define blueviolet 0x8A2BE2
#define indigo 0x4B0082
#define darkslateblue 0x483D8B
#define slateblue 0x6A5ACD
#define mediumslateblue 0x7B68EE
#define mediumpurple 0x9370DB
#define darkmagenta 0x8B008B
#define darkviolet 0x9400D3
#define darkorchid 0x9932CC
#define mediumorchid 0xBA55D3
#define purple 0x800080
#define thistle 0xD8BFD8
#define plum 0xDDA0DD
#define violet 0xEE82EE
#define magenta 0xFF00FF
#define orchid 0xDA70D6
#define mediumvioletred 0xC71585
#define palevioletred 0xDB7093
#define deeppink 0xFF1493
#define hotpink 0xFF69B4
#define lightpink 0xFFB6C1
#define pink 0xFFC0CB
#define antiquewhite 0xFAEBD7
#define beige 0xF5F5DC
#define bisque 0xFFE4C4
#define blanchedalmond 0xFFEBCD
#define wheat 0xF5DEB3
#define cornsilk 0xFFF8DC
#define lemonchiffon 0xFFFACD
#define lightgolden rod yellow 0xFAFAD2
#define lightyellow 0xFFFFE0
#define saddlebrown 0x8B4513
#define sienna 0xA0522D
#define chocolate 0xD2691E
#define peru 0xCD853F
#define sandybrown 0xF4A460
#define burlywood 0xDEB887
#define tanish 0xD2B48C
#define rosybrown 0xBC8F8F
#define moccasin 0xFFE4B5
#define navajowhite 0xFFDEAD
#define peachpuff 0xFFDAB9
#define mistyrose 0xFFE4E1
#define lavenderblush 0xFFF0F5
#define linen 0xFAF0E6
#define oldlace 0xFDF5E6
#define papayawhip 0xFFEFD5
#define seashell 0xFFF5EE
#define mintcream 0xF5FFFA
#define slategray 0x708090
#define lightslategray 0x778899
#define lightsteelblue 0xB0C4DE
#define lavender 0xE6E6FA
#define floralwhite 0xFFFAF0
#define aliceblue 0xF0F8FF
#define ghostwhite 0xF8F8FF
#define honeydew 0xF0FFF0
#define ivory 0xFFFFF0
#define azure 0xF0FFFF
#define snow 0xFFFAFA
#define black 0x000000
#define dimgrey 0x696969
#define grey 0x808080
#define darkgrey 0xA9A9A9
#define silver 0xC0C0C0
#define lightgrey 0xD3D3D3
#define gainsboro 0xDCDCDC
#define whitesmoke 0xF5F5F5
#define white 0xFFFFFF

long eightcolorschema[][8] PROGMEM={
  azure,snow,lavender,aliceblue,honeydew,seashell,lightslategray,lavenderblush, //white ish
  red,green,blue,magenta,teal,yellow,white,black,
  grey,orange,seashell,peru,red,azure,black,silver,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
};

//##############bitmap storage
/* string index of character table
 !"#$%&'()*+,-./ //start 0 end 15
 0123456789:;>=<? //start 16 end 31
 @ABCDEFGHIJKLMNO //start 32 end 47
 PQRSTUVWXYZ[ ]^_ //START 48 END 63
 `abcdefghijklmno //start 64 end 79
 pqrstuvwxyz{|}~~ //start 80 end 95
 */
//uint8_t message[2] ={2,2};
// led character definitions modified from http://www.edaboard.com/thread45151.html
// 5 data columns + 1 space
// for each character



const char led_chars[97][6] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00, // space 0
  0x00,0x00,0xfa,0x00,0x00,0x00, // ! 1
  0x00,0xe0,0x00,0xe0,0x00,0x00,  // "2
  0x28,0xfe,0x28,0xfe,0x28,0x00,  // #3
  0x24,0x54,0xfe,0x54,0x48,0x00, // $4
  0xc4,0xc8,0x10,0x26,0x46,0x00, // %5
  0x6c,0x92,0xaa,0x44,0x0a,0x00, // &6
  0x00,0xa0,0xc0,0x00,0x00,0x00, // '7
  0x00,0x38,0x44,0x82,0x00,0x00,	// (8
  0x00,0x82,0x44,0x38,0x00,0x00, // )10
  0x28,0x10,0x7c,0x10,0x28,0x00, // *11
  0x10,0x10,0x7c,0x10,0x10,0x00, // +12
  0x00,0x0a,0x0c,0x00,0x00,0x00, // ,13
  0x10,0x10,0x10,0x10,0x10,0x00, // -14
  0x00,0x06,0x06,0x00,0x00,0x00, // .15
  0x04,0x08,0x10,0x20,0x40,0x00, // / 16
  0x7c,0x8a,0x92,0xa2,0x7c,0x00, // 0 17
  0x00,0x42,0xfe,0x02,0x00,0x00, // 1 18
  0x42,0x86,0x8a,0x92,0x62,0x00, // 2 9
  0x84,0x82,0xa2,0xd2,0x8c,0x00,	// 3 0
  0x18,0x28,0x48,0xfe,0x08,0x00,	// 4 1
  0xe5,0xa2,0xa2,0xa2,0x9c,0x00,	// 5 2
  0x3c,0x52,0x92,0x92,0x0c,0x00,	// 6 3
  0x80,0x8e,0x90,0xa0,0xc0,0x00,	// 7 4
  0x6c,0x92,0x92,0x92,0x6c,0x00,	// 8 5
  0x60,0x92,0x92,0x94,0x78,0x00,	// 9 6
  0x00,0x6c,0x6c,0x00,0x00,0x00,	// : 7
  0x00,0x6a,0x6c,0x00,0x00,0x00,	// ;8
  0x10,0x28,0x44,0x82,0x00,0x00,	// <9
  0x28,0x28,0x28,0x28,0x28,0x00,	// =0
  0x00,0x82,0x44,0x28,0x10,0x00,	// >1
  0x40,0x80,0x8a,0x90,0x60,0x00,	// ?2
  0x4c,0x92,0x9e,0x82,0x7c,0x00,	// @3
  0x7e,0x88,0x88,0x88,0x7e,0x00,	// A4
  0xfe,0x92,0x92,0x92,0x6c,0x00,	// B5
  0x7c,0x82,0x82,0x82,0x44,0x00,	// C6
  0xfe,0x82,0x82,0x44,0x38,0x00,	// D7
  0xfe,0x92,0x92,0x92,0x82,0x00,	// E8
  0xfe,0x90,0x90,0x90,0x80,0x00,	// F9
  0x7c,0x82,0x92,0x92,0x5e,0x00,	// G0
  0xfe,0x10,0x10,0x10,0xfe,0x00,	// H1
  0x00,0x82,0xfe,0x82,0x00,0x00,	// I2
  0x04,0x02,0x82,0xfc,0x80,0x00,	// J3
  0xfe,0x10,0x28,0x44,0x82,0x00,	// K4
  0xfe,0x02,0x02,0x02,0x02,0x00,	// L5
  0xfe,0x40,0x30,0x40,0xfe,0x00,	// M6
  0xfe,0x20,0x10,0x08,0xfe,0x00,	// N7
  0x7c,0x82,0x82,0x82,0x7c,0x00,	// O8
  0xfe,0x90,0x90,0x90,0x60,0x00,	// P9
  0x7c,0x82,0x8a,0x84,0x7a,0x00,	// Q0
  0xfe,0x90,0x98,0x94,0x62,0x00,	// R1
  0x62,0x92,0x92,0x92,0x8c,0x00,	// S2
  0x80,0x80,0xfe,0x80,0x80,0x00,	// T3
  0xfc,0x02,0x02,0x02,0xfc,0x00,	// U4
  0xf8,0x04,0x02,0x04,0xf8,0x00,	// V5
  0xfc,0x02,0x1c,0x02,0xfc,0x00,	// W6
  0xc6,0x28,0x10,0x28,0xc6,0x00,	// X7
  0xe0,0x10,0x0e,0x10,0xe0,0x00,	// Y8
  0x86,0x8b,0x92,0xa2,0xc2,0x00,	// Z9
  0x00,0xfe,0x82,0x82,0x00,0x00,	// [0
  0x00,0x00,0x00,0x00,0x00,0x00, //1 *** do not remove this empty char ***
  0x00,0x82,0x82,0xfe,0x00,0x00,	// ]2
  0x20,0x40,0x80,0x40,0x20,0x00,	// ^3
  0x02,0x02,0x02,0x02,0x02,0x00,	// _4
  0x00,0x80,0x40,0x20,0x00,0x00,	// `5
  0x04,0x2a,0x2a,0x2a,0x1e,0x00,	// a6
  0xfe,0x12,0x22,0x22,0x1c,0x00,	// b7
  0x1c,0x22,0x22,0x22,0x04,0x00,	// c8
  0x1c,0x22,0x22,0x12,0xfe,0x00,	// d9
  0x1c,0x2a,0x2a,0x2a,0x18,0x00,	// e0
  0x10,0x7e,0x90,0x80,0x40,0x00,	// f1
  0x30,0x4a,0x4a,0x4a,0x7c,0x00,	// g2
  0xfe,0x10,0x20,0x20,0x1e,0x00,	// h3
  0x00,0x22,0xbe,0x02,0x00,0x00,	// i4
  0x04,0x02,0x22,0xbc,0x00,0x00,	// j5
  0xfe,0x08,0x14,0x22,0x00,0x00,	// k6
  0x00,0x82,0xfe,0x02,0x00,0x00,	// l7
  0x3e,0x20,0x18,0x20,0x1e,0x00,	// m8
  0x3e,0x10,0x20,0x20,0x1e,0x00,	// n9
  0x1c,0x22,0x22,0x22,0x1c,0x00,	// o0
  0x3e,0x28,0x28,0x28,0x10,0x00,	// p1
  0x10,0x28,0x28,0x18,0x3e,0x00,	// q2
  0x3e,0x10,0x20,0x20,0x10,0x00,	// r3
  0x12,0x2a,0x2a,0x2a,0x04,0x00,	// s4
  0x20,0xfc,0x22,0x02,0x04,0x00,	// t5
  0x3c,0x02,0x02,0x04,0x3e,0x00,	// u6
  0x38,0x04,0x02,0x04,0x38,0x00,	// v7
  0x3c,0x02,0x0c,0x02,0x3c,0x00,	// w8
  0x22,0x14,0x08,0x14,0x22,0x00,	// x9
  0x30,0x0a,0x0a,0x0a,0x3c,0x00,	// y0
  0x22,0x26,0x2a,0x32,0x22,0x00, // z1
  0x00,0x10,0x6c,0x82,0x00,0x00,	// {2
  0x00,0x00,0xfe,0x00,0x00,0x00,	// |3
  0x00,0x82,0x6c,0x10,0x00,0x00,
  0x18,0x3c,0x7e,0xff,0x7e,0x3c}; //4

// Principle of operation: at any given time, the LEDs depict an image or
// animation effect (referred to as the "back" image throughout this code).
// Periodically, a transition to a new image or animation effect (referred
// to as the "front" image) occurs. During this transition, a third buffer
// (the "alpha channel") determines how the front and back images are
// combined; it represents the opacity of the front image. When the
// transition completes, the "front" then becomes the "back," a new front
// is chosen, and the process repeats.
byte imgData[2][numPixels * 3], // Data for 2 strips worth of imagery
alphaMask[numPixels], // Alpha channel for compositing images
backImgIdx = 0, // Index of 'back' image (always 0 or 1)
fxIdx[3]; // Effect # for back & front images + alpha
int fxVars[3][50]; // Effect instance variables (explained later)
// Countdown to next transition
int transitionTime; // Duration (in frames) of current transition
float fxFloats[3][4];
// function prototypes, leave these be :)
void renderEffect00(byte idx);
void renderEffect01(byte idx);
void renderEffect02(byte idx);
void renderEffect03(byte idx);
void renderAlpha00(void);
void renderAlpha01(void);
void renderAlpha02(void);
void renderAlpha03(void);
void callback();
byte gamma(byte x);
long hsv2rgb(long h, byte s, byte v);
char fixSin(int angle);
char fixCos(int angle);

// List of image effect and alpha channel rendering functions; the code for
// each of these appears later in this file. Just a few to start with...
// simply append new ones to the appropriate list here:


// ---------------------------------------------------------------------------

void setup() {


  // for (int thisReading = 0; thisReading < numReadings; thisReading++)
  // readings[thisReading] = 0;

  // Start up the LED strip. Note that strip.show() is NOT called here --
  // the callback function will be invoked immediately when attached, and
  // the first thing the calback does is update the strip.

  // Initialize the serial port.
  Serial.begin(115200);
  Serial.println();
  Serial.println("Send a");
  Serial.println("+ to press button");
  //  Serial.println("B to increase brightness, ");
  //  Serial.println("b to decrease brightness, ");
  Serial.println("D to enable compass debug,");
  Serial.println("d to disable compass debug");
  Serial.println("C to + color scheme");
  Serial.println("c to - color scheme");
  Serial.println("M to enter menu");
  Serial.println("m to go back to run");

  Serial.println("Starting the I2C interface.");
  Wire.begin(); // Start the I2C interface.

  // Serial.println("Constructing new HMC5883L");
  compass = HMC5883L(); // Construct a new HMC5883 compass.

  Serial.println("Setting scale to +/- 8.1 Ga");
  error = compass.SetScale(compassscale); // Set the scale of the compass. //orig 1.3
  if(error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));

  Serial.println("Setting measurement mode to continous.");
  error = compass.SetMeasurementMode(Measurement_Continuous); // Set the measurement mode to Continuous
  if(error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));


  strip.begin();
  Serial.println("LED Strip Online.");
  // Initialize random number generator from a floating analog input.
  randomSeed(analogRead(0));
  memset(imgData, 0, sizeof(imgData)); // Clear image data
  fxVars[backImgIdx][0] = 1; // Mark back image as initialized

  // Timer1 is used so the strip will update at a known fixed frame rate.
  // Each effect rendering function varies in processing complexity, so
  // the timer allows smooth transitions between effects (otherwise the
  // effects and transitions would jump around in speed...not attractive).
  Timer1.initialize();
  Timer1.attachInterrupt(callback, 1000000 / framerate); // x frames/second
  Serial.print("Timer1 set at ");
  Serial.print(framerate);
  Serial.println(" fps.");
  //  attachInterrupt(0, buttonpress, RISING);
}

void findplane(){
  MagnetometerScaled scaled = compass.ReadScaledAxis();
  if(abs(scaled.XAxis)>abs(scaled.YAxis)&&abs(scaled.XAxis)>abs(scaled.ZAxis)) //in plane 1
  {
    if(scaled.XAxis>0){
      plane=1;
    }
    else{
      plane=-1;
    }
  }
  if(abs(scaled.YAxis)>abs(scaled.XAxis)&&abs(scaled.YAxis)>abs(scaled.ZAxis)) //in plane 2

  {
    if(scaled.YAxis>0){
      plane=2;
    }
    else{
      plane=-2;
    }
  }

  if(abs(scaled.ZAxis)>abs(scaled.YAxis)&&abs(scaled.ZAxis)>abs(scaled.XAxis)) //in plane 3

  {
    if(scaled.ZAxis>0){
      plane=3;
    }
    else{
      plane=-3;
    }

  }
  if(compassdebug==1){
    Serial.println();
    Serial.print("plane:");
    Serial.println(plane);
  }
}

void compassread()
{
  // Retrive the raw values from the compass (not scaled).
  // MagnetometerRaw raw = compass.ReadRawAxis();
  // Retrived the scaled values from the compass (scaled to the configured scale).
  MagnetometerScaled scaled = compass.ReadScaledAxis();

  // Values are accessed like so:
  // int MilliGauss_OnThe_XAxis = scaled.XAxis;// (or YAxis, or ZAxis)
  xyheadinglast = xyheading;
  xzheadinglast = xzheading;
  yzheadinglast = yzheading;
  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  xyheading = atan2(scaled.YAxis, scaled.XAxis);
  xzheading = atan2(scaled.XAxis, scaled.ZAxis);
  yzheading = atan2(scaled.ZAxis, scaled.YAxis);
  xytravel = atan2(xyheading,xyheadinglast);
  xztravel = atan2(xzheading,xzheadinglast);
  yztravel = atan2(yzheading,yzheadinglast);
  // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your locatio

  // Find yours here: http://www.magnetic-declination.com/
  // Mine is: 2Ã¯Â¿Â½ 37' W, which is 2.617 Degrees, or (which we need) 0.0456752665 radians, I will use 0.0457
  // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
  // float declinationAngle = 0.0457;
  // heading += declinationAngle;

  // Correct for when signs are reversed.
  if(xyheading < 0)
    xyheading += 2*PI;

  // Check for wrap due to addition of declination.
  if(xyheading > 2*PI)
    xyheading -= 2*PI;

  // Convert radians to degrees for readability.
  xyheadingdegreeslast = xyheadingdegrees;
  xyheadingdegrees = xyheading * 180/M_PI;
  // runningaverage(xyheadingDegrees);


  // Correct for when signs are reversed.
  if(xzheading < 0)
    xzheading += 2*PI;

  // Check for wrap due to addition of declination.
  if(xzheading > 2*PI)
    xzheading -= 2*PI;

  // Convert radians to degrees for readability.
  xzheadingdegrees = xzheading * 180/M_PI;
  // Correct for when signs are reversed.
  if(yzheading < 0)
    yzheading += 2*PI;

  // Check for wrap due to addition of declination.
  if(yzheading > 2*PI)
    yzheading -= 2*PI;
  if(millis()>10000){
    //xydynamic calibration
    if (xyheadingdegrees>xyheadingdegreesmax||xyheadingdegreesmax==0){
      xyheadingdegreesmax=xyheadingdegrees;
    }
    else{
      if(xyheadingdegrees<xyheadingdegreesmin||xyheadingdegreesmin==0){
        xyheadingdegreesmin=xyheadingdegrees;
      }
    }
    xyheadingdegreescalibrated = map(xyheadingdegrees,xyheadingdegreesmin,xyheadingdegreesmax,0,360);
    //xzdynamic calibration
    if (xzheadingdegrees>xzheadingdegreesmax||xzheadingdegreesmax==0){
      xzheadingdegreesmax=xzheadingdegrees;
    }
    else{
      if(xzheadingdegrees<xzheadingdegreesmin||xzheadingdegreesmin==0){
        xzheadingdegreesmin=xzheadingdegrees;
      }
    }
    xzheadingdegreescalibrated = map(xzheadingdegrees,xzheadingdegreesmin,xzheadingdegreesmax,0,360);
    //yzdynamic calibration
    if (yzheadingdegrees>yzheadingdegreesmax||yzheadingdegreesmax==0){
      yzheadingdegreesmax=yzheadingdegrees;
    }
    else{
      if(yzheadingdegrees<yzheadingdegreesmin||yzheadingdegreesmin==0){
        yzheadingdegreesmin=yzheadingdegrees;
      }
    }
    yzheadingdegreescalibrated = map(yzheadingdegrees,yzheadingdegreesmin,yzheadingdegreesmax,0,360);
  }


  if (compassdebug==1){
    Serial.print("xy");
    Serial.println(xyheadingdegreesmin);
    Serial.println(xyheadingdegreesmax);
    Serial.println(xyheadingdegrees);
    Serial.print("xz");
    Serial.println(xzheadingdegrees);
    Serial.println(xzheadingdegreescalibrated);
    Serial.print("yz");
    Serial.println(yzheadingdegrees);
    Serial.println(yzheadingdegreescalibrated);
    delay(250);
  }



  // Convert radians to degrees for readability.
  // float yzheadingDegrees = yzheading * 180/M_PI;


  // Output the data via the serial port.
  // Output(raw, scaled, heading, headingDegrees);

  // Normally we would delay the application by 66ms to allow the loop
  // to run at 15Hz (default bandwidth for the HMC5883L).
  // However since we have a long serial out (104ms at 9600) we will let
  // it run at its natural speed.
  // delay(66);
  // Serial.print(xyheadingDegrees);
  // Serial.print(" xyDegrees \t");
  // Serial.print(xzheadingDegrees);
  // Serial.print(" xzDegrees \t");
  // Serial.print(yzheadingDegrees);
  // Serial.println(" yzDegrees \t");
  //figure out headingdelta
  //void runningAverage(int newval)
  /* if(count1==0){count1=1;}else{
   if(xyheadingDegrees>xyheadingDegreeslast){//indicates cw rotation or rollover from 359-0 in ccw rotation
   if(xyheadingDegreeslast>90||xyheadingDegreeslast<270){
   runningaverage(xyheadingDegrees-xyheadingDegreeslast);
   // xyheadingDegreesdelta=(xyheadingDegrees-xyheadingDegreeslast)+xyheadingDegreesdelta;
   }//indicates cw rotation w/o rollover
   else{
   runningaverage(xyheadingDegreeslast+(360-xyheadingDegrees));
   // xyheadingDegreesdelta=(xyheadingDegreeslast+(360-xyheadingDegrees))+xyheadingDegreesdelta;
   } //indicates ccw rotation with rollover
   }
   if(xyheadingDegrees<xyheadingDegreeslast){ //indicates ccw rotation or rollover from 0-359 in cw rotation
   if(xyheadingDegreeslast>90||xyheadingDegreeslast<270){
   runningaverage((xyheadingDegreeslast-xyheadingDegrees));
   // xyheadingDegreesdelta=(xyheadingDegreeslast-xyheadingDegrees)+xyheadingDegreesdelta;
   }//indicates ccw rotation w/o rollover
   else{
   runningaverage(xyheadingDegrees+(xyheadingDegreeslast-360));
   // xyheadingDegreesdelta=(xyheadingDegrees+(xyheadingDegreeslast-360))+xyheadingDegreesdelta;
   */
  //}; //indicates cw rotation with rollover
  // }; }
  /*
if(xyheadingDegreesdelta>90){
   xyheadingDegreesdelta=xyheadingDegreesdelta-90;
   count=count+1;
   xymillisdelta=xymillislast - millis();
   xymillislast=millis();
   Serial.println(xyheadingDegreesdelta );
   };
   */
  //Serial.println(average);
}

//void Output(MagnetometerRaw raw, MagnetometerScaled scaled, float heading, float headingDegrees)
//{
// Serial.print("Raw:\t");
// Serial.print(raw.XAxis);
// Serial.print(" ");
// Serial.print(raw.YAxis);
// Serial.print(" ");
// Serial.print(raw.ZAxis);
// Serial.print(" \tScaled:\t");

// Serial.print(scaled.XAxis);
// Serial.print(" ");
// Serial.print(scaled.YAxis);
// Serial.print(" ");
// Serial.print(scaled.ZAxis);

// Serial.print(" \tHeading:\t");
/// Serial.print(heading);
// Serial.print(" Radians \t");
// Serial.print(headingDegrees);
// Serial.println(" Degrees \t");
//}

void loop() {
  getSerial();
  findplane();
  compassread();
}
void menurender() {
  strip.show();
  byte *backPtr    = &imgData[backImgIdx][0],
  r, g, b;
  int  i;
  for(i=0; i<numPixels; i++) {
    r = gamma(*backPtr++);
    g = gamma(*backPtr++);
    b = gamma(*backPtr++);
    strip.setPixelColor(i, r, g, b);
  }
  menu();
}
void menu() {
  byte *ptr = &imgData[0][0];
  switch(menuphase){
  case 0:
    if(button==1){
      menuphase0=xyheadingdegreescalibrated/60;
      menuphase++;
    }
    else{
      for(int i=0; i<numPixels; i++) {
        long color=red;
        if(i<=xyheadingdegreescalibrated/60){
          *ptr++ = color >> 16;
          *ptr++ = color >> 8;
          *ptr++ = color;
        }
        else{
          // for(int i=0; i<numPixels; i++) {
          *ptr++=0;
          *ptr++=0;
          *ptr++=0;
        }
      }
    }
    break;
  case 1:
    if(button==1){
      menuphase1=xyheadingdegreescalibrated/60;
      menuphase++;
    }
    else{
      for(int i=0; i<numPixels; i++) {
        if(i<=menuphase0){
          long color=red;
          *ptr++ = color >> 16;
          *ptr++ = color >> 8;
          *ptr++ = color;
        }
        else{
          if(i>=menuphase0&&i<=menuphase0+(xyheadingdegreescalibrated/60)){
            long color=magenta;
            *ptr++ = color >> 16;
            *ptr++ = color >> 8;
            *ptr++ = color;
          }
          else{
            *ptr++=0;
            *ptr++=0;
            *ptr++=0;
          }
        }
      }
    }
    break;
  case 2:
    if(button==1){
      menuphase2=xyheadingdegreescalibrated/60;
      menuphase++;
    }
    else{
      for(int i=0; i<numPixels; i++) {
        if(i<=menuphase0){
          long color=red;
          *ptr++ = color >> 16;
          *ptr++ = color >> 8;
          *ptr++ = color;
        }
        else{
          if(i>=menuphase0&&i<=menuphase0+menuphase1){
            long color=magenta;
            *ptr++ = color >> 16;
            *ptr++ = color >> 8;
            *ptr++ = color;
          }
          else{
            if(i>=menuphase0+menuphase1&&i<=menuphase0+menuphase1+(xyheadingdegreescalibrated/60)){
              long color=blue;
              *ptr++ = color >> 16;
              *ptr++ = color >> 8;
              *ptr++ = color;
            }
            else{
              *ptr++=0;
              *ptr++=0;
              *ptr++=0;
            }
          }
        }
      }
    }
    break;
  case 3:
    if(button==1){
      menuphase3=xyheadingdegreescalibrated/60;
      menuphase++;
    }
    else{
      for(int i=0; i<numPixels; i++) {
        if(i<=menuphase0){
          long color=red;
          *ptr++ = color >> 16;
          *ptr++ = color >> 8;
          *ptr++ = color;
        }
        else{
          if(i>=menuphase0&&i<=menuphase0+menuphase1){
            long color=magenta;
            *ptr++ = color >> 16;
            *ptr++ = color >> 8;
            *ptr++ = color;
          }
          else{
            if(i>=menuphase0+menuphase1&&i<=menuphase0+menuphase1+menuphase2){
              long color=blue;
              *ptr++ = color >> 16;
              *ptr++ = color >> 8;
              *ptr++ = color;
            }
            else{
              if(i>=menuphase0+menuphase1+menuphase2&&i<=menuphase0+menuphase1+menuphase2+(xyheadingdegreescalibrated/60)){
                //   if(i>=menuphase0+menuphase1&&i<=menuphase0+menuphase1+(xyheadingdegreescalibrated/60)){
                long color=teal;
                *ptr++ = color >> 16;
                *ptr++ = color >> 8;
                *ptr++ = color;
              }
              else{
                *ptr++=0;
                *ptr++=0;
                *ptr++=0;
              }
            }
          }
        }
      }  
    }
    break;
  }
}
// Timer1 interrupt handler. Called at equal intervals; 60 Hz by default.
void callback() {
  // Very first thing here is to issue the strip data generated from the
  // *previous* callback. It's done this way on purpose because show() is
  // roughly constant-time, so the refresh will always occur on a uniform
  // beat with respect to the Timer1 interrupt. The various effects
  // rendering and compositing code is not constant-time, and that
  // unevenness would be apparent if show() were called at the end.
  strip.show();

  byte frontImgIdx = 1 - backImgIdx,
  *backPtr = &imgData[backImgIdx][0],
  r, g, b;
  int i;

  // Always render back image based on current effect index:
  (*renderEffect[fxIdx[backImgIdx]])(backImgIdx);

  // Front render and composite only happen during transitions...
  if(tCounter > 0) {
    // Transition in progress
    byte *frontPtr = &imgData[frontImgIdx][0];
    int alpha, inv;

    // Render front image and alpha mask based on current effect indices...
    (*renderEffect[fxIdx[frontImgIdx]])(frontImgIdx);
    (*renderAlpha[fxIdx[2]])();

    // ...then composite front over back:
    for(i=0; i<numPixels; i++) {
      alpha = alphaMask[i] + 1; // 1-256 (allows shift rather than divide)
      inv = 257 - alpha; // 1-256 (ditto)
      // r, g, b are placed in variables (rather than directly in the
      // setPixelColor parameter list) because of the postincrement pointer
      // operations -- C/C++ leaves parameter evaluation order up to the
      // implementation; left-to-right order isn't guaranteed.
      r = gamma((*frontPtr++ * alpha + *backPtr++ * inv) >> 8);
      g = gamma((*frontPtr++ * alpha + *backPtr++ * inv) >> 8);
      b = gamma((*frontPtr++ * alpha + *backPtr++ * inv) >> 8);
      strip.setPixelColor(i, r, g, b);
    }
  }
  else {
    // No transition in progress; just show back image
    for(i=0; i<numPixels; i++) {
      // See note above re: r, g, b vars.
      r = gamma(*backPtr++);
      g = gamma(*backPtr++);
      b = gamma(*backPtr++);
      strip.setPixelColor(i, r, g, b);
    }
  }

  // Count up to next transition (or end of current one):
  if(demo==1||tCounter>=0){
    tCounter++;
  }
  if(button==1){
    tCounter=0;
    button=0;
  }

  if(tCounter == 0) { // Transition start
    //fxIdx[frontImgIdx] = random((sizeof(renderEffect) / sizeof(renderEffect[0]))); //original random selection
    fxIdx[frontImgIdx]++;//instead of random now its sequential
    if(fxIdx[frontImgIdx]>=(sizeof(renderEffect) / sizeof(renderEffect[0]))){
      fxIdx[frontImgIdx]=0;
    }

    fxIdx[2] = random((sizeof(renderAlpha) / sizeof(renderAlpha[0])));
    transitionTime = random(120, 240); // depends on frame rate. if 240hz 120 frame = 1/2 sec
    fxVars[frontImgIdx][0] = 0; // Effect not yet initialized
    fxVars[2][0] = 0; // Transition not yet initialized
  }
  else if(tCounter >= transitionTime) { // End transition
    fxIdx[backImgIdx] = fxIdx[frontImgIdx]; // Move front effect index to back
    backImgIdx = 1 - backImgIdx; // Invert back index
    if(demo==0){ //works?
      tCounter = -1; // hold image on the edge
      button=0;
    }
    else{
      tCounter = random(-7200 ,-14400); // Hold image ? to ? seconds
    }
  }
}
// ---------------------------------------------------------------------------
// Image effect rendering functions. Each effect is generated parametrically
// (that is, from a set of numbers, usually randomly seeded). Because both
// back and front images may be rendering the same effect at the same time
// (but with different parameters), a distinct block of parameter memory is
// required for each image. The 'fxVars' array is a two-dimensional array
// of integers, where the major axis is either 0 or 1 to represent the two
// images, while the minor axis holds 50 elements -- this is working scratch
// space for the effect code to preserve its "state." The meaning of each
// element is generally unique to each rendering effect, but the first element
// is most often used as a flag indicating whether the effect parameters have
// been initialized yet. When the back/front image indexes swap at the end of
// each transition, the corresponding set of fxVars, being keyed to the same
// indexes, are automatically carried with them.

// Simplest rendering effect: fill entire image with solid color
long getschemacolor(uint8_t y){
  long color;
  color = pgm_read_dword(&eightcolorschema[colorschemeselector][y%8]);
  return color;
}

void hsvtest(byte idx) {
  if(fxVars[idx][0] == 0) {
    byte *ptr = &imgData[idx][0];
    for(int i=0; i<numPixels; i++) {
      long color;
      color = getschemacolor(random(0,8)); 
      *ptr++ = color >> 16;
      *ptr++ = color >> 8;
      *ptr++ = color;
    }
  }
}
void colorDrift(byte idx) {
  if(fxVars[idx][0] == 0) {
    fxVars[idx][1]=random(0,1536); //color were gonna write initally
    fxVars[idx][0] = 1; // Effect initialized
    fxVars[idx][2] =random(1,4); //increments of color drift per frame
    // fxVars[idx][2] =1; //increments of color drift per frame
  }
  byte *ptr = &imgData[idx][0];
  if(fxVars[idx][1]>1536-fxVars[idx][2]){
    fxVars[idx][1]=fxVars[idx][1]%1536;
  }
  fxVars[idx][1]+=fxVars[idx][2];
  // Serial.println(fxVars[idx][1]);
  //Serial.println(fxVars[idx][1]);
  for(int i=0; i<numPixels; i++) {
    long color;
    color = hsv2rgb(fxVars[idx][1],
    255, 255);
    *ptr++ = color >> 16;
    *ptr++ = color >> 8;
    *ptr++ = color;
  }
  // Serial.println(fxVars[idx][1]);
}
void sparkle(byte idx) {
  if(fxVars[idx][0] == 0) {
    fxVars[idx][1] = 0; // color scheme selector
    fxVars[idx][0]=1;
  }
  long color;
  byte *ptr = &imgData[idx][0];
  for(int i=0; i<numPixels; i++) {
    *ptr++ = color >> 16;
    *ptr++ = color >> 8;
    *ptr++ = color;
  }
}
void strobe(byte idx) {
  if(fxVars[idx][0] == 0) {
    fxVars[idx][1] = 0; //
    fxVars[idx][2] = random(0,2); //increments of color drift per frame
    fxVars[idx][3] = 0; //strobe indicator, 0 is nothing written for the frame and anything else is write
    fxVars[idx][4] = 0; //frame counter 0-120
    fxVars[idx][5] = random(0,2); //dutycycle, 1-5 for 10-90%
    fxVars[idx][6] = random(0,5);//effect type,0 is one color strobe,1 is 2 color,2 is 3 color
    fxVars[idx][7] = random(0,1);//if 1 replace black with second color 9
    fxVars[idx][8] = 60; // strobe duty cycle value
    fxVars[idx][9] = 0; //
    fxVars[idx][0] = 1; // Effect initialized
  }
  //fxVars[idx][7]++;
  //if(fxVars[idx][7]==fxVars[idx][8]){
  // fxVars[idx][7]=0;
  // fxVars[idx][5]++;}
  byte *ptr = &imgData[idx][0];
  fxVars[idx][1]+=fxVars[idx][2];
  if(fxVars[idx][1]>1536-fxVars[idx][1]){
    fxVars[idx][1]=fxVars[idx][1]%1536;
  }
  for(int i=0; i<numPixels; i++) {
    long color;
    //color = hsv2rgb(fxVars[idx][1],255, 255);
    color = getschemacolor(fxVars[idx][1]);
    if(fxVars[idx][3]==0){

      *ptr++ = 0;
      *ptr++ = 0;
      *ptr++ = 0;
    }
    else{
      *ptr++ = color >> 16;
      *ptr++ = color >> 8;
      *ptr++ = color;

    }

  }
  if(fxVars[idx][3]!=0){
    fxVars[idx][1]++;
    fxVars[idx][1]%=7;
  }
  //Serial.println(fxVars[idx][1]);
  switch(abs(fxVars[idx][5])) //dutycycle=0-9
  {
  case 0://10%ducy cycle
    if(fxVars[idx][4]%2==0){
      fxVars[idx][3]=1;
    }
    else{
      fxVars[idx][3]=0;
    }
    break;

  case 1://20%ducy cycle
    if(fxVars[idx][4]%5==0){
      fxVars[idx][3]=1;
    }
    else{
      fxVars[idx][3]=0;
    }
    break;
  case 2://30%ducy cycle
    if(fxVars[idx][4]%10==0){
      fxVars[idx][3]=1;
    }
    else{
      fxVars[idx][3]=0;
    }
    break;
  case 3://40%ducy cycle
    if(fxVars[idx][4]%40==0){
      fxVars[idx][3]=1;
    }
    else{
      fxVars[idx][3]=0;
    }
    break;
  case 4://50%ducy cycle
    if(fxVars[idx][4]%50==0){
      fxVars[idx][3]=1;
    }
    else{
      fxVars[idx][3]=0;
    }
    break;
  }
  if(fxVars[idx][4]>=100){ //keep track of frames passed
    fxVars[idx][4]=1;
  }
  else{
    fxVars[idx][4]=fxVars[idx][4]+1;
  }
  if(fxVars[idx][4]%24==1){
    // fxVars[idx][5]++;
  }
  if(fxVars[idx][5]>2){
    fxVars[idx][5]=-1;
  }
  //Serial.println(fxVars[idx][5]);
}
void RandomColorsEverywhere(byte idx) {
  byte *ptr = &imgData[idx][0];
  for (int i=0;i<numPixels*3; i++){
    byte r = random(256), g = random(256), b = random(256);
    *ptr++ =r,g,b;
  }
}

void twoatonce(byte idx){

  crazycounter++;
  if(crazycounter>30){
    crazycounter=0;
  }

  if(crazycounter>15){
    strobe(idx);
  }
  else{
    rainbowChase(idx);
  }

}

void fans(byte idx) {
  if(fxVars[idx][0] == 0) {
    int i;
    fxVars[idx][1]=random(1536); //color were gonna use to cycle
    fxVars[idx][2]=numPixels/random(2,8); //using this to determine the # of sections between 2 and 8
    fxVars[idx][3]=0;//frame counter operator. starts at 1 and is incremented every frame,
    fxVars[idx][4]=0;//# of frames until next change
    fxVars[idx][6]=fxVars[idx][2]*2;//number of different levels
    fxVars[idx][5]=-fxVars[idx][6]+1;// level operator
    fxVars[idx][7]=0;//using this to keep track of which section we're writing to, operator of fxVars[idx][2]. starts at 0
    fxVars[idx][8]=fxVars[idx][2];// this is the number of times to cut up the 1536 increment wheel. 2=opposite colors, 3 == a triangle, 4= a square
    //using fxVars[idx][2] here makes the whole stretch minus the remainder go once around the clolr wheel
    fxVars[idx][9]=0;//
    fxVars[idx][0]=1;// Effect initialized
  }
  if(fxVars[idx][0] == -1) { //re init
  }
  byte *ptr = &imgData[idx][0];
  long color;
  for(int i=0; i<numPixels/fxVars[idx][2]; i++) {
    for(int i=0; i<fxVars[idx][2]; i++) {
      if(fxVars[idx][6]/fxVars[idx][2]*i>abs(fxVars[idx][5])){//number of levels(frames) to change over nubmer of leds gives change per pixel
        // color = hsv2rgb(fxVars[idx][1]+((1536/fxVars[idx][8])*fxVars[idx][7]), 255, 255);
        color = getschemacolor(fxVars[idx][7]);
        *ptr++ = color >> 16;
        *ptr++ = color >> 8;
        *ptr++ = color;
      }
      else{
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
      }
    }
    fxVars[idx][7]++;
    fxVars[idx][5]++; //increment level operator
    if(fxVars[idx][5]>=fxVars[idx][6]){ //if level operator == number of levels
      fxVars[idx][5]=-fxVars[idx][6]+1;
    }
  }
  for(int i=0; i<numPixels%fxVars[idx][2]; i++) {// do the same thing here, this is for the remainder
    if(fxVars[idx][6]/fxVars[idx][2]*i>abs(fxVars[idx][5])){//number of levels to change over nubmer of leds gives change per pixel
      color = hsv2rgb(fxVars[idx][1]+((1536/fxVars[idx][8])*fxVars[idx][7]),
      255, 255);
      *ptr++ = color >> 16;
      *ptr++ = color >> 8;
      *ptr++ = color;
    }
    else{
      *ptr++ = 0;
      *ptr++ = 0;
      *ptr++ = 0;
    }
  }
  fxVars[idx][7]=0;
}
void skipAflash(byte idx) {
  // this will alternate which flash at different times
  if(fxVars[idx][0] == 1) {
    byte *ptr = &imgData[idx][0],
    r = random(256), g = random(256), b = random(256);
    int count=0;

    if (count = (2 + (2 * rand ()) %10)) {
      count +=1;
      for(int i=0; i<(numPixels/2); i++) {
        *ptr++ = r;
        *ptr++ = g;
        *ptr++ = b;
        byte r1= r, g1=g, b1=b;
        r=0;
        g=0;
        b=0;
        *ptr++ = r;
        *ptr++ = g;
        *ptr++ = b;
        r=r1;
        g=g1;
        b=b1;
      }
    }
    else if (count =( 3 + (2 * rand ()) %10)) {


      for(int j=0; j<(numPixels/2); j++) {
        byte r2, g2, b2;
        r=r2;
        g=g2;
        b=b2;
        r=0;
        g=0;
        b=0;
        *ptr++ = r;
        *ptr++ = g;
        *ptr++ = b;
        r2=r;
        g2=g;
        b2=b;
        r = random(256), g = random(256), b = random(256);
        *ptr++ = r;
        *ptr++ = g;
        *ptr++ = b;
      }
    }
  }

  fxVars[idx][0] = 1;
}




void Dice(byte idx){
  if(fxVars[idx][0] == 0) {
    fxVars[idx][1]=0;
    fxVars[idx][2]=1;
    fxVars[idx][3]=2;
    fxVars[idx][4]=3;
    fxVars[idx][5]=4;
    fxVars[idx][6]=5;
    fxVars[idx][7]=0;
    fxVars[idx][0]=1; //effect init
  }
  if(plane>0){
    fxVars[idx][7]=plane;
  }
  else{
    fxVars[idx][7]=abs(plane)*2;
  }
  byte *ptr = &imgData[idx][0];
  for(int i=0; i<numPixels; i++) {
    long color;
    // color = hsv2rgb(fxVars[idx][fxVars[idx][7]],255, 255);
    color = getschemacolor(fxVars[idx][7]);
    *ptr++ = color >> 16;
    *ptr++ = color >> 8;
    *ptr++ = color;
  }
}

void POV(byte idx) {
  const String Message[5] = {
    "KolaHoops.com ","MAKE ","HACK ","CREATE ",":)?#@&:("                  };
  const String led_chars_index =" ! #$%&'()*+,-./0123456789:;>=<?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[ ]^_`abcdefghijklmnopqrstuvwxyz{|}~~";
  if(fxVars[idx][0] == 0) {
    int i;
    fxVars[idx][1]=random(1536); //color were gonna use to cycle
    fxVars[idx][2]=8; //either 8 or 16 (scale of 1 or 2 ), used to determine # of pixels in height; our character table is 8 x 6
    fxVars[idx][3]=0;//frame counter operator. starts at 1 and is incremented every frame,
    fxVars[idx][4]=0;//# of frames until next change
    fxVars[idx][6]=6;//number of different levels or time. a level is incremented every x# of frames; character table is 8x6
    fxVars[idx][5]=0;// level operator gets a ++ every loop and is set to -9 when @ 10 and abs() when called so it oscillates
    fxVars[idx][7]=0;//using this to keep track of which section we're writing to, operator of fxVars[idx][2]. starts at 0
    fxVars[idx][8]=fxVars[idx][2];// this is the number of times to cut up the 1536 increment wheel. 2=opposite colors, 3 == a triangle, 4= a square
    //using fxVars[idx][2] here makes the whole stretch minus the remainder go once around the clolr wheel
    fxVars[idx][9]=0;// character counter
    fxVars[idx][10]=random(0,4);// determines message for the message array. 0 = KolaHoops.com, 1=make,2=hack,3=build, 4 = a bunch of symbols
    fxVars[idx][11]= random(0,10); //if greater than 5,change the message after it finishes
    fxVars[idx][0]=1;// Effect initialized
  }
  if(fxVars[idx][0] == -1) { //re init
  }
  fxVars[idx][3]++;
  byte *ptr = &imgData[idx][0];
  long color;
  for(int i=0; i<numPixels/fxVars[idx][2]; i++) {
    for(int i=0; i<fxVars[idx][2]; i++) {
      byte data=pgm_read_byte (&led_chars[led_chars_index.indexOf(Message[fxVars[idx][10]].charAt(fxVars[idx][9]))][fxVars[idx][5]]); //
      // if(data>>i==1){
      if((data>>i)&1){
        //led_chars_index.indexOf(Message.charAt(fxVars[idx][9]))
        // color = hsv2rgb(fxVars[idx][1]+((1536/fxVars[idx][8])*fxVars[idx][7]),255, 255);
        color = getschemacolor(fxVars[idx][7]);
        *ptr++ = color >> 16;
        *ptr++ = color >> 8;
        *ptr++ = color;
      }
      else{
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
      }
    }
    fxVars[idx][7]++;
  }
  for(int i=0; i<numPixels%fxVars[idx][2]; i++) {// do the same thing here, this is for the remainder
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 0;
  }
  if(fxVars[idx][3]>=fxVars[idx][4]){
    fxVars[idx][3]=0;
    fxVars[idx][5]++;
  }
  if(fxVars[idx][5]>=fxVars[idx][6]) // if level operator > level holder then increment character and check for overflow
  {
    fxVars[idx][5]=0;
    fxVars[idx][9]++;
    if(fxVars[idx][9]>=Message[fxVars[idx][10]].length()){
      fxVars[idx][9]=0;
    }
    //Serial.println(fxVars[idx][5]);
    fxVars[idx][7]=0;
  }
}

/*
void PeteOV(byte idx) {
 if(fxVars[idx][0] == 0) {
 int i;
 fxVars[idx][1]=random(1536); //color were gonna use to cycle
 fxVars[idx][2]=8; //either 8 or 16 (scale of 1 or 2 ), used to determine # of pixels in height; our character table is 8 x 6
 fxVars[idx][3]=0;//frame counter operator. starts at 1 and is incremented every frame,
 fxVars[idx][4]=30;//# of frames until next change
 fxVars[idx][6]=6;//number of different levels or time. a level is incremented every x# of frames; character table is 8x6
 fxVars[idx][5]=0;// level operator gets a ++ every loop and is set to -9 when @ 10 and abs() when called so it oscillates
 fxVars[idx][7]=0;//using this to keep track of which section we're writing to, operator of fxVars[idx][2]. starts at 0
 fxVars[idx][8]=fxVars[idx][2];// this is the number of times to cut up the 1536 increment wheel. 2=opposite colors, 3 == a triangle, 4= a square
 //using fxVars[idx][2] here makes the whole stretch minus the remainder go once around the clolr wheel
 fxVars[idx][9]=0;// character counter
 fxVars[idx][10]=random(0,4);// determines message for the message array. 0 = KolaHoops.com, 1=make,2=hack,3=build, 4 = a bunch of symbols
 fxVars[idx][11]= random(0,10); //if greater than 5,change the message after it finishes
 fxVars[idx][0]=1;// Effect initialized
 }
 if(fxVars[idx][0] == -1) { //re init
 }
 fxVars[idx][3]++;
 byte *ptr = &imgData[idx][0];
 long color;
 for(int i=0; i<numPixels/fxVars[idx][2]; i++) {
 for(int i=0; i<fxVars[idx][2]; i++) {
 byte data=0xfa;
 // if(data>>i==1){
 if((data>>i)&1){
 //led_chars_index.indexOf(Message.charAt(fxVars[idx][9]))
 color = hsv2rgb(((1536/fxVars[idx][8]+fxVars[idx][1])*fxVars[idx][7]),
 255, 255);
 *ptr++ = color >> 16;
 *ptr++ = color >> 8;
 *ptr++ = color;
 }
 else{
 *ptr++ = 0;
 *ptr++ = 0;
 *ptr++ = 0;
 }
 }
 fxVars[idx][7]++;
 }
 for(int i=0; i<numPixels%fxVars[idx][2]; i++) {// do the same thing here, this is for the remainder
 *ptr++ = 0;
 *ptr++ = 0;
 *ptr++ = 0;
 }
 if(fxVars[idx][3]>=fxVars[idx][4]){
 fxVars[idx][3]=0;
 fxVars[idx][5]++;
 }
 if(fxVars[idx][5]>=fxVars[idx][6]) // if level operator > level holder then increment character and check for overflow
 {
 fxVars[idx][5]=0;
 fxVars[idx][9]++;
 if(fxVars[idx][9]>=Message[fxVars[idx][10]].length()){
 fxVars[idx][9]=0;
 }
 //Serial.println(fxVars[idx][5]);
 fxVars[idx][7]=0;
 }
 }
 */
//simple dual pixel chasin in opposite directions
void SnakeChase(byte idx) { //brassman79 on github wrote this one!!!
  if(fxVars[idx][0] == 0) { // Initialize effect?
    fxVars[idx][1] = random(1536); // Random hue
    fxFloats[idx][0] = random(100,1000) / 1000.0 ; // random speed 0.0 - 1.0
    fxFloats[idx][1] = 1; // Current position
    fxVars[idx][4] = 4 + random(10); // random spread
    fxVars[idx][5] = fxVars[idx][4] * -2;

    fxVars[idx][6] = fxVars[idx][1]*(random(2,5))%1536; // Random hue far away from first color
    fxFloats[idx][2] = random(100,1000) / -1000.0 ; // random speed 0.0 - 1.0
    fxFloats[idx][3] = (float)numPixels; // Current position
    fxVars[idx][9] = 4 + random(10); // random spread
    fxVars[idx][10] = fxVars[idx][9] * -2;

    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = &imgData[idx][0], r1, g1, b1, r2, g2, b2, r, g, b;
  float distance = 0.0, modifier1 = 0.0,modifier2 = 0.0;
  int hue = 0, saturation = 0;
  long color1, color2, i;

  for(i=0; i<numPixels; i++) {
    hue = 255;
    modifier1 = 0;
    modifier2 = 0;

    if(i >= numPixels - fxVars[idx][4] && fxFloats[idx][1] <= fxVars[idx][4]) {
      modifier1 = numPixels;
    }
    if(i <= fxVars[idx][4] && fxFloats[idx][1] >= numPixels - fxVars[idx][4]) {
      modifier1 = -numPixels;
    }

    if(i >= numPixels - fxVars[idx][9] && fxFloats[idx][3] <= fxVars[idx][9]) {
      modifier2 = numPixels;
    }
    if(i <= fxVars[idx][9] && fxFloats[idx][3] >= numPixels - fxVars[idx][9]) {
      modifier2 = -numPixels;
    }

    distance = min(abs(i - fxFloats[idx][1] - modifier1), fxVars[idx][4]) * -2;
    saturation = map(distance, fxVars[idx][5], 0.0, 0, 255);
    color1 = hsv2rgb(fxVars[idx][1], hue, saturation);

    distance = min(abs(i - fxFloats[idx][3] - modifier2), fxVars[idx][9]) * -2;
    saturation = map(distance, fxVars[idx][10], 0.0, 0, 255);
    color2 = hsv2rgb(fxVars[idx][6], hue, saturation);


    r1 = color2 >> 16;
    g1 = color2 >> 8;
    b1 = color2;
    r2 = color1 >> 16;
    g2 = color1 >> 8;
    b2 = color1;
    r = min(r1 + r2, 255);
    g = min(g1 + g2, 255);
    b = min(b1 + b2, 255);
    *ptr++ = r;
    *ptr++ = g;
    *ptr++ = b;
  }

  fxFloats[idx][1] += fxFloats[idx][0];
  if(fxFloats[idx][1] >= numPixels){
    fxFloats[idx][1] -= numPixels;
  }
  fxFloats[idx][3] += fxFloats[idx][2];
  if(fxFloats[idx][3] <= 0){
    fxFloats[idx][3] += numPixels;
  }
}

void pacman(byte idx) { //hsv color chase for now
  if(fxVars[idx][0] == 0) {// using hsv for pacman
    fxVars[idx][1]=0;//get new pacman color
    // fxVars[idx][2]=fxVars[idx][1]+256;//get new 2nd color
    fxVars[idx][3]=numPixels/2;//pacman position
    fxVars[idx][4]=0;//lower edge
    fxVars[idx][5]=numPixels;//upper edge
    fxVars[idx][6]=2;//wait counter if at 120 fps 12 would be 1/10 of a second
    fxVars[idx][7]=0; //wait operator
    fxVars[idx][8]=random(4); //pcaman width
    fxVars[idx][0] = 1; // Effect initialized
  }
  if(fxVars[idx][0] == -1) { //re init
    // fxVars[idx][2]=fxVars[idx][1];
    fxVars[idx][1]++;//get new pacman color
    fxVars[idx][3]=numPixels/2;//pacman position
    fxVars[idx][4]=0;//lower edge
    fxVars[idx][5]=numPixels;//upper edge
    fxVars[idx][8]=random(4); //pcaman width
    fxVars[idx][0] = 1; // Effect initialized
  }

  int i;
  byte *ptr = &imgData[idx][0];
  for(i=0; i<numPixels; i++) {
    long color;
    if(i>=abs(fxVars[idx][3])-fxVars[idx][8]&&i<=abs(fxVars[idx][3])+fxVars[idx][8]){

      // color = hsv2rgb(fxVars[idx][1], 255, 255);
      color = getschemacolor(fxVars[idx][1]+1);
      *ptr++ = color >> 16;
      *ptr++ = color >> 8;
      *ptr++ = color;
    }
    else{
      if(i<=fxVars[idx][4]||i>=fxVars[idx][5]){
        // color = hsv2rgb(fxVars[idx][1], 255, 255);
        color = getschemacolor(fxVars[idx][1]+1);
        *ptr++ = color >> 16;
        *ptr++ = color >> 8;
        *ptr++ = color;
      }
      else{
        // color = hsv2rgb(fxVars[idx][2], 255, 255);
        color = getschemacolor(fxVars[idx][1]);
        *ptr++ = color >> 16;
        *ptr++ = color >> 8;
        *ptr++ = color;
      }
    }
  }

  if(fxVars[idx][3]>=numPixels){
    fxVars[idx][3]=-numPixels+1;
  }
  fxVars[idx][7]++;
  if(fxVars[idx][7]==fxVars[idx][6]){
    fxVars[idx][3]++;
    fxVars[idx][7]=0;
  }

  if(fxVars[idx][4]==fxVars[idx][5]){
    fxVars[idx][0]=-1;
  }
  else{
    if(abs(fxVars[idx][3])==fxVars[idx][4]){
      fxVars[idx][4]++;
      fxVars[idx][3]=fxVars[idx][4]+1;
    }
    else{
      if(abs(fxVars[idx][3])==fxVars[idx][5]){
        fxVars[idx][5]--;
        fxVars[idx][3]=((0-1)*fxVars[idx][5])+1;
      }
    }
  }
}

// Rainbow effect (1 or more full loops of color wheel at 100% saturation).
// Not a big fan of this pattern (it's way overused with LED stuff), but it's
// practically part of the Geneva Convention by now.
void rainbowChase(byte idx) {
  if(fxVars[idx][0] == 0) {

    // Number of repetitions (complete loops around color wheel); any
    // more than 4 per meter just looks too chaotic and un-rainbow-like.
    // Store as hue 'distance' around complete belt:
    fxVars[idx][1] = (1 + random(4 * ((numPixels + 31) / 32))) * 1536;
    // Frame-to-frame hue increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring. It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][2] = 4 + random(fxVars[idx][1]) / numPixels;
    // Reverse speed and hue shift direction half the time.
    if(random(2) == 0) fxVars[idx][1] = -fxVars[idx][1];
    if(random(2) == 0) fxVars[idx][2] = -fxVars[idx][2];
    fxVars[idx][3] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = &imgData[idx][0];
  long color, i;
  for(i=0; i<numPixels; i++) {
    color = hsv2rgb(fxVars[idx][3] + fxVars[idx][1] * i / numPixels,
    255, 255);
    *ptr++ = color >> 16;
    *ptr++ = color >> 8;
    *ptr++ = color;
  }
  fxVars[idx][3] += fxVars[idx][2];
}



void sineChase(byte idx) {

  if(fxVars[idx][0] == 0) {

    fxVars[idx][1] = random(1536); // Random hue
    // Number of repetitions (complete loops around color wheel);
    // any more than 4 per meter just looks too chaotic.
    // Store as distance around complete belt in half-degree units:
    fxVars[idx][2] = (1 + random(4 * ((numPixels + 31) / 32))) * 720;
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring. It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][3] = 4 + random(fxVars[idx][1]) / numPixels;
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = &imgData[idx][0];
  int foo;
  long color, i;
  for(long i=0; i<numPixels; i++) {
    foo = fixSin(fxVars[idx][4] + fxVars[idx][2] * i / numPixels);
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).
    color = (foo >= 0) ?
    hsv2rgb(fxVars[idx][1], 254 - (foo * 2), 255) :
    hsv2rgb(fxVars[idx][1], 255, 254 + foo * 2);
    *ptr++ = color >> 16;
    *ptr++ = color >> 8;
    *ptr++ = color;
  }
  fxVars[idx][4] += fxVars[idx][3];
}

// Data for American-flag-like colors (20 pixels representing
// blue field, stars and stripes). This gets "stretched" as needed
// to the full LED strip length in the flag effect code, below.
// Can change this data to the colors of your own national flag,
// favorite sports team colors, etc. OK to change number of elements.
/*#define AAA 255
 #define BBB 255
 #define CCC 64
 #define C_RED AAA, BBB, CCC
 #define C_GREEEN CCC, AAA, BBB
 #define C_BLUE BBB, CCC, AAA
 PROGMEM prog_uchar flagTable[] = {
 C_BLUE , C_GREEEN, C_BLUE , C_GREEEN, C_BLUE , C_GREEEN, C_BLUE,
 C_RED , C_GREEEN, C_RED , C_GREEEN, C_RED , C_GREEEN, C_RED ,
 C_GREEEN, C_RED , C_GREEEN, C_RED , C_GREEEN, C_RED };
 */

#define C_RED 160, 0, 0
#define C_WHITE 255, 255, 255
#define C_BLUE 0, 0, 100
PROGMEM prog_uchar flagTable[] = {
  C_BLUE , C_WHITE, C_BLUE , C_WHITE, C_BLUE , C_WHITE, C_BLUE,
  C_RED , C_WHITE, C_RED , C_WHITE, C_RED , C_WHITE, C_RED ,
  C_WHITE, C_RED , C_WHITE, C_RED , C_WHITE, C_RED };


// Wavy flag effect




void wavyFlag(byte idx) {
  long i, sum, s, x;
  int idx1, idx2, a, b;
  if(fxVars[idx][0] == 0) { // Initialize effect?
    fxVars[idx][1] = 720 + random(720); // Wavyness
    fxVars[idx][2] = 4 + random(10); // Wave speed
    fxVars[idx][3] = 200 + random(200); // Wave 'puckeryness'
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
  }
  for(sum=0, i=0; i<numPixels-1; i++) {
    sum += fxVars[idx][3] + fixCos(fxVars[idx][4] + fxVars[idx][1] *
      i / numPixels);
  }

  byte *ptr = &imgData[idx][0];
  for(s=0, i=0; i<numPixels; i++) {
    x = 256L * ((sizeof(flagTable) / 3) - 1) * s / sum;
    idx1 = (x >> 8) * 3;
    idx2 = ((x >> 8) + 1) * 3;
    b = (x & 255) + 1;
    a = 257 - b;
    *ptr++ = ((pgm_read_byte(&flagTable[idx1 ]) * a) +
      (pgm_read_byte(&flagTable[idx2 ]) * b)) >> 8;
    *ptr++ = ((pgm_read_byte(&flagTable[idx1 + 1]) * a) +
      (pgm_read_byte(&flagTable[idx2 + 1]) * b)) >> 8;
    *ptr++ = ((pgm_read_byte(&flagTable[idx1 + 2]) * a) +
      (pgm_read_byte(&flagTable[idx2 + 2]) * b)) >> 8;
    s += fxVars[idx][3] + fixCos(fxVars[idx][4] + fxVars[idx][1] *
      i / numPixels);
  }

  fxVars[idx][4] += fxVars[idx][2];
  if(fxVars[idx][4] >= 720) fxVars[idx][4] -= 720;
}





/*
void sineCompass(byte idx) {
 // Only needs to be rendered once, when effect is initialized:
 if(fxVars[idx][0] == 0) {
 Serial.println("effect=04");
 // fxVars[idx][1] = random(1536); // Random hue
 fxVars[idx][1] = 1; // Random hue
 // Number of repetitions (complete loops around color wheel);
 // any more than 4 per meter just looks too chaotic.
 // Store as distance around complete belt in half-degree units:
 // fxVars[idx][2] = (1 + random(4 * ((numPixels + 31) / 32))) * 720; //original
 fxVars[idx][2] = 720; //we will vary this by xy heading change speed, xyspeed will be between 0 and 4
 // Frame-to-frame increment (speed) -- may be positive or negative,
 // but magnitude shouldn't be so small as to be boring. It's generally
 // still less than a full pixel per frame, making motion very smooth.
 //fxVars[idx][3] = 4 + random(fxVars[idx][1]) / numPixels; //original
 fxVars[idx][3] = 0; //no rotation
 // Reverse direction half the time.
 if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
 fxVars[idx][4] = 0; // Current position
 fxVars[idx][0] = 1; // Effect initialized
 }
 fxVars[idx][4] = map(average,0,360,0,720*4); // Current position
 byte *ptr = &imgData[idx][0];
 int foo;
 long color, i;
 for(long i=0; i<numPixels; i++) {
 foo = fixSin(fxVars[idx][4] + fxVars[idx][2] * i / numPixels);
 // Peaks of sine wave are white, troughs are black, mid-range
 // values are pure hue (100% saturated).
 color = (foo >= 0) ? //black?
 hsv2rgb(fxVars[idx][1], 254 - (foo * 2), 255) : //white!
 hsv2rgb(fxVars[idx][1], 255, 254 + foo * 2); //color
 *ptr++ = color >> 16;
 *ptr++ = color >> 8;
 *ptr++ = color;
 }
 // fxVars[idx][4] += fxVars[idx][3];
 }
 */
void MonsterHunter(byte idx) {
  if(fxVars[idx][0] == 0) {
    fxVars[idx][1]=random(1536);
    fxVars[idx][3]=numPixels/2;//pacman position
    fxVars[idx][2]=random(1536);//get new color
    fxVars[idx][4]=0;//position of light
    fxVars[idx][7]=random(1536);
    fxVars[idx][5]=12;//wait steps
    fxVars[idx][8]=random(1536);
    fxVars[idx][6]=0;//wait operator
    fxVars[idx][9]=random(1536); //backgtround color
    fxVars[idx][10] =random(0,16); //increments of color drift per frame
    fxVars[idx][11] =random(0,4); //determine if sparkle3, strobe2 , colordrift1 or blank0 backgorund
    fxVars[idx][13] =random(0,4); //randomly determines the with of the pchasers 1=3 wide 2=5 wide 3=7 wide
    fxVars[idx][12] =0; //strobe operator
    fxVars[idx][0] = 1; // Effect initialized
  }

  if(fxVars[idx][11]>=1){
    fxVars[idx][9]= fxVars[idx][10]+fxVars[idx][9];
    if(fxVars[idx][9]>1536){
      fxVars[idx][9]=fxVars[idx][9]%1536;
    }
  }
  fxVars[idx][12] =!fxVars[idx][12]; //?
  byte *ptr = &imgData[idx][0];
  int l;
  for(int i=0,l=numPixels; i<numPixels,l>0; i++,l--) {
    long color;
    if(i>=fxVars[idx][4]-fxVars[idx][13]&&i<=fxVars[idx][4]+fxVars[idx][13]){
      color = getschemacolor(0);
      //color = hsv2rgb(fxVars[idx][1], 255, 255);
      *ptr++ = color >> 16;
      *ptr++ = color >> 8;
      *ptr++ = color;
    }
    else{
      if(numPixels-i>=fxVars[idx][4]-fxVars[idx][13]&&numPixels-i<=fxVars[idx][4]+fxVars[idx][13]){
        //color = hsv2rgb(fxVars[idx][2], 255, 255);
        color = getschemacolor(1);
        *ptr++ = color >> 16;
        *ptr++ = color >> 8;
        *ptr++ = color;
      }
      else{
        if((i+(numPixels/2))%numPixels>=fxVars[idx][4]-fxVars[idx][13]&&(i+(numPixels/2))%numPixels<=fxVars[idx][4]+fxVars[idx][13]){
          // color = hsv2rgb(fxVars[idx][7],255, 255);
          color = getschemacolor(2);
          *ptr++ = color >> 16;
          *ptr++ = color >> 8;
          *ptr++ = color;
        }
        else{
          if(((numPixels/2)-i+numPixels)%numPixels>=fxVars[idx][4]-fxVars[idx][13]&&((numPixels/2)-i+numPixels)%numPixels<=fxVars[idx][4]+fxVars[idx][13]){
            // color = hsv2rgb(fxVars[idx][8], 255, 255);
            color = getschemacolor(3);
            *ptr++ = color >> 16;
            *ptr++ = color >> 8;
            *ptr++ = color;
          }
          else{
            if(fxVars[idx][11]==0){
              *ptr++ = 0;
              *ptr++ =0;
              *ptr++ =0;
            }

            if(fxVars[idx][11]==1){
              // color = hsv2rgb(fxVars[idx][9],128, 128);
              color = getschemacolor(4);
              *ptr++ = color >> 16;
              *ptr++ = color >> 8;
              *ptr++ = color;
            }
            if(fxVars[idx][11]==2){
              //color = hsv2rgb(fxVars[idx][9], 128, 128);
              color = getschemacolor(5);
              if(fxVars[idx][12]==0){
                *ptr++ = 0;
                *ptr++ =0;
                *ptr++ =0;
              }
              else{
                *ptr++ = color >> 16;
                *ptr++ = color >> 8;
                *ptr++ = color;
              }
            }
            if(fxVars[idx][11]==3){

              color = hsv2rgb(random(1536),
              255, 128);
              *ptr++ = color >> 16;
              *ptr++ = color >> 8;
              *ptr++ = color;




            }
          }
        }
      }
    }
  }


  fxVars[idx][6]++;
  if(fxVars[idx][5]==fxVars[idx][6]){
    fxVars[idx][6]=0;
    fxVars[idx][4]++;
  }
  if(fxVars[idx][4]==numPixels){
    fxVars[idx][4]=0;
  }

}

// TO DO: Add more effects here...Larson scanner, etc.

// ---------------------------------------------------------------------------
// Alpha channel effect rendering functions. Like the image rendering
// effects, these are typically parametrically-generated...but unlike the
// images, there is only one alpha renderer "in flight" at any given time.
// So it would be okay to use local static variables for storing state
// information...but, given that there could end up being many more render
// functions here, and not wanting to use up all the RAM for static vars
// for each, a third row of fxVars is used for this information.

// Simplest alpha effect: fade entire strip over duration of transition.
void renderAlpha00(void) {
  byte fade = 255L * tCounter / transitionTime;
  for(int i=0; i<numPixels; i++) alphaMask[i] = fade;
}



// Straight left-to-right or right-to-left wipe
void renderAlpha01(void) {
  long x, y, b;
  if(fxVars[2][0] == 0) {
    fxVars[2][1] = random(1, numPixels); // run, in pixels
    fxVars[2][2] = (random(2) == 0) ? 255 : -255; // rise
    fxVars[2][0] = 1; // Transition initialized
  }

  b = (fxVars[2][2] > 0) ?
  (255L + (numPixels * fxVars[2][2] / fxVars[2][1])) *
    tCounter / transitionTime - (numPixels * fxVars[2][2] / fxVars[2][1]) :
  (255L - (numPixels * fxVars[2][2] / fxVars[2][1])) *
    tCounter / transitionTime;
  for(x=0; x<numPixels; x++) {
    y = x * fxVars[2][2] / fxVars[2][1] + b; // y=mx+b, fixed-point style
    if(y < 0) alphaMask[x] = 0;
    else if(y >= 255) alphaMask[x] = 255;
    else alphaMask[x] = (byte)y;
  }
}

// Dither reveal between images
void renderAlpha02(void) {
  long fade;
  int i, bit, reverse, hiWord;

  if(fxVars[2][0] == 0) {
    // Determine most significant bit needed to represent pixel count.
    int hiBit, n = (numPixels - 1) >> 1;
    for(hiBit=1; n; n >>=1) hiBit <<= 1;
    fxVars[2][1] = hiBit;
    fxVars[2][0] = 1; // Transition initialized
  }

  for(i=0; i<numPixels; i++) {
    // Reverse the bits in i for ordered dither:
    for(reverse=0, bit=1; bit <= fxVars[2][1]; bit <<= 1) {
      reverse <<= 1;
      if(i & bit) reverse |= 1;
    }
    fade = 256L * numPixels * tCounter / transitionTime;
    hiWord = (fade >> 8);
    if(reverse == hiWord) alphaMask[i] = (fade & 255); // Remainder
    else if(reverse < hiWord) alphaMask[i] = 255;
    else alphaMask[i] = 0;
  }
}

// TO DO: Add more transitions here...triangle wave reveal, etc.

// ---------------------------------------------------------------------------
// Assorted fixed-point utilities below this line. Not real interesting.

// Gamma correction compensates for our eyes' nonlinear perception of
// intensity. It's the LAST step before a pixel value is stored, and
// allows intermediate rendering/processing to occur in linear space.
// The table contains 256 elements (8 bit input), though the outputs are
// only 7 bits (0 to 127). This is normal and intentional by design: it
// allows all the rendering code to operate in the more familiar unsigned
// 8-bit colorspace (used in a lot of existing graphics code), and better
// preserves accuracy where repeated color blending operations occur.
// Only the final end product is converted to 7 bits, the native format
// for the LPD8806 LED driver. Gamma correction and 7-bit decimation
// thus occur in a single operation.
PROGMEM prog_uchar gammaTable[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4,
  4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7,
  7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11,
  11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 16, 16,
  16, 17, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22,
  23, 23, 24, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30,
  30, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 37, 37, 38, 38, 39,
  40, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 48, 49, 50,
  50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 58, 58, 59, 60, 61, 62,
  62, 63, 64, 65, 66, 67, 67, 68, 69, 70, 71, 72, 73, 74, 74, 75,
  76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99,100,101,102,104,105,106,107,108,
  109,110,111,113,114,115,116,117,118,120,121,122,123,125,126,127
};

// This function (which actually gets 'inlined' anywhere it's called)
// exists so that gammaTable can reside out of the way down here in the
// utility code...didn't want that huge table distracting or intimidating
// folks before even getting into the real substance of the program, and
// the compiler permits forward references to functions but not data.
inline byte gamma(byte x) {
  return pgm_read_byte(&gammaTable[x/brightness]);
  //return x/2;
}

// Fixed-point colorspace conversion: HSV (hue-saturation-value) to RGB.
// This is a bit like the 'Wheel' function from the original strandtest
// code on steroids. The angular units for the hue parameter may seem a
// bit odd: there are 1536 increments around the full color wheel here --
// not degrees, radians, gradians or any other conventional unit I'm
// aware of. These units make the conversion code simpler/faster, because
// the wheel can be divided into six sections of 256 values each, very
// easy to handle on an 8-bit microcontroller. Math is math, and the
// rendering code elsehwere in this file was written to be aware of these
// units. Saturation and value (brightness) range from 0 to 255.
long hsv2rgb(long h, byte s, byte v) {
  byte r, g, b, lo;
  int s1;
  long v1;

  // Hue
  h %= 1536; // -1535 to +1535
  if(h < 0) h += 1536; // 0 to +1535
  lo = h & 255; // Low byte = primary/secondary color mix
  switch(h >> 8) { // High byte = sextant of colorwheel
  case 0 :
    r = 255 ;
    g = lo ;
    b = 0 ;
    break; // R to Y
  case 1 :
    r = 255 - lo;
    g = 255 ;
    b = 0 ;
    break; // Y to G
  case 2 :
    r = 0 ;
    g = 255 ;
    b = lo ;
    break; // G to C
  case 3 :
    r = 0 ;
    g = 255 - lo;
    b = 255 ;
    break; // C to B
  case 4 :
    r = lo ;
    g = 0 ;
    b = 255 ;
    break; // B to M
  default:
    r = 255 ;
    g = 0 ;
    b = 255 - lo;
    break; // M to R
  }

  // Saturation: add 1 so range is 1 to 256, allowig a quick shift operation
  // on the result rather than a costly divide, while the type upgrade to int
  // avoids repeated type conversions in both directions.
  s1 = s + 1;
  r = 255 - (((255 - r) * s1) >> 8);
  g = 255 - (((255 - g) * s1) >> 8);
  b = 255 - (((255 - b) * s1) >> 8);

  // Value (brightness) and 24-bit color concat merged: similar to above, add
  // 1 to allow shifts, and upgrade to long makes other conversions implicit.
  v1 = v + 1;
  return (((r * v1) & 0xff00) << 8) |
    ((g * v1) & 0xff00) |
    ( (b * v1) >> 8);
}

// The fixed-point sine and cosine functions use marginally more
// conventional units, equal to 1/2 degree (720 units around full circle),
// chosen because this gives a reasonable resolution for the given output
// range (-127 to +127). Sine table intentionally contains 181 (not 180)
// elements: 0 to 180 *inclusive*. This is normal.

PROGMEM prog_char sineTable[181] = {
  0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 27, 28, 29, 30, 31, 32, 33, 34,
  35, 36, 37, 38, 39, 40, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
  67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 77, 78, 79, 80, 81,
  82, 83, 83, 84, 85, 86, 87, 88, 88, 89, 90, 91, 92, 92, 93, 94,
  95, 95, 96, 97, 97, 98, 99,100,100,101,102,102,103,104,104,105,
  105,106,107,107,108,108,109,110,110,111,111,112,112,113,113,114,
  114,115,115,116,116,117,117,117,118,118,119,119,120,120,120,121,
  121,121,122,122,122,123,123,123,123,124,124,124,124,125,125,125,
  125,125,126,126,126,126,126,126,126,127,127,127,127,127,127,127,
  127,127,127,127,127
};

char fixSin(int angle) {
  angle %= 720; // -719 to +719
  if(angle < 0) angle += 720; // 0 to +719
  return (angle <= 360) ?
  pgm_read_byte(&sineTable[(angle <= 180) ?
  angle : // Quadrant 1
  (360 - angle)]) : // Quadrant 2
  -pgm_read_byte(&sineTable[(angle <= 540) ?
  (angle - 360) : // Quadrant 3
  (720 - angle)]) ; // Quadrant 4
}

char fixCos(int angle) {
  angle %= 720; // -719 to +719
  if(angle < 0) angle += 720; // 0 to +719
  return (angle <= 360) ?
  ((angle <= 180) ? pgm_read_byte(&sineTable[180 - angle]) : // Quad 1
  -pgm_read_byte(&sineTable[angle - 180])) : // Quad 2
  ((angle <= 540) ? -pgm_read_byte(&sineTable[540 - angle]) : // Quad 3
  pgm_read_byte(&sineTable[angle - 540])) ; // Quad 4
}



void buttonpress(){
  if ((millis() - lastDebounceTime) > debounceDelay) {
    button=1;
  }
  lastDebounceTime = millis();
}

/*
void runningaverage(int newval) {
 // subtract the last reading:
 total= total - readings[index];
 // read from the sensor:
 readings[index] = newval;
 // add the reading to the total:
 total= total + readings[index];
 // advance to the next position in the array:
 index = index + 1;
 // if we're at the end of the array...
 if (index >= numReadings)
 // ...wrap around to the beginning:
 index = 0;
 // calculate the average:
 average = total / numReadings;
 // send it to the computer as ASCII digits
 // Serial.println(average);
 // delay(1); // delay in between reads for stability
 }
 */
void getSerial(){
  byte *ptr = &imgData[0][0];
  int num;
  if( readSerialString() ) {
    Serial.println(serInStr);
    char cmd = serInStr[0]; // first char is command
    char* str = serInStr;
    while( *++str == ' ' ); // got past any intervening whitespace
    num = atoi(str); // the rest is arguments (maybe)
    if( cmd == '+' ) {
      Serial.println("Next Pattern");
      button=1;
    }
    if( cmd == 'd' ) {
      compassdebug=0;
      Serial.println("disable compass serial output");
    }
    if( cmd == 'D' ) {
      compassdebug=1;
      Serial.println("enable compass serial output");
    }

    if( cmd == 'M' ) {
      Serial.println("Entering Menu");
      // for(int i=0; i<numPixels; i++) {
      // *ptr++=0;
      // *ptr++=0;
      // *ptr++=0;}
      // strip.show();}
      Timer1.attachInterrupt(menurender, 1000000/framerate);//redirect interrupt to menu
    }
    if( cmd == 'm' ) {
      Serial.println("Entering callback");
      Timer1.attachInterrupt(callback, 1000000/framerate);//redirect interrupt to callback
    }
    if( cmd == 'C' ) {
      colorschemeselector++;
      Serial.print("Color Scheme ");
      Serial.println(colorschemeselector);
    }
    if( cmd == 'c' ) {
      colorschemeselector--;
      Serial.print("Color Scheme ");
      Serial.println(colorschemeselector);
    }


    serInStr[0] = 0; // say we've used the string
  }
}
//read a string from the serial and store it in an array
//you must supply the array variable
uint8_t readSerialString()
{
  if(!Serial.available()) {
    return 0;
  }
  delay(10); // wait a little for serial data

  memset( serInStr, 0, sizeof(serInStr) ); // set it all to zero
  int i = 0;
  while (Serial.available()) {
    serInStr[i] = Serial.read(); // FIXME: doesn't check buffer overrun
    i++;
  }
  //serInStr[i] = 0; // indicate end of read string
  return i; // return number of chars read
}

#include <ctype.h>
uint8_t toHex(char hi, char lo)
{
  uint8_t b;
  hi = toupper(hi);
  if( isxdigit(hi) ) {
    if( hi > '9' ) hi -= 7; // software offset for A-F
    hi -= 0x30; // subtract ASCII offset
    b = hi<<4;
    lo = toupper(lo);
    if( isxdigit(lo) ) {
      if( lo > '9' ) lo -= 7; // software offset for A-F
      lo -= 0x30; // subtract ASCII offset
      b = b + lo;
      return b;
    } // else error
  } // else error
  return 0;
}






