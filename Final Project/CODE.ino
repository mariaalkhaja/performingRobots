#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include <Servo.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define delayVAL 10 // Time (in milliseconds) to pause between pixels
#define FACTORYRESET_ENABLE      0

// Which pin on the Arduino is connected to the NeoPixels
#define LED_PIN    A1
#define EYE_PIN    3
#define PIXELS_PIN 6

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 135
#define NUMPIXELS 49
#define EYEPIXELS 7

Adafruit_NeoPixel pixels (NUMPIXELS, PIXELS_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel eye (EYEPIXELS, EYE_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


Servo myservo;  // create servo object to control a servo

int analogPin = 6;
int pos = 0;   // servo position
int happy = 0;
int four = 0;

// Create the motor shield object
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

// Select which 'port' M1, M2, M3 or M4
Adafruit_DCMotor *myMotor = AFMS.getMotor(1);
Adafruit_DCMotor *myOtherMotor = AFMS.getMotor(2);

#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
#include <SoftwareSerial.h>
#endif

//#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

// Create the bluefruit object
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}


// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);

// the packet buffer
extern uint8_t packetbuffer[];




void setup(void)
{
  while (!Serial);  // required for Flora & Micro
  delay(500);

  AFMS.begin();  // create with the default frequency 1.6KHz

  // Set the speed to start, from 0 (off) to 255 (max speed)
  myMotor->setSpeed(200);
  myMotor->run(FORWARD);
  // turn on motor
  myMotor->run(RELEASE);

  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit App Controller Example"));
  Serial.println(F("-----------------------------------------"));

  // initialize NeoPixels
  pixels.begin();
  eye.begin ();
  strip.begin();

  strip.show();            // Turn OFF all pixels
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)


#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.


  myservo.attach(10);  // attaches the servo on pin 10 to the servo object

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }


  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
    delay(500);
  }

  Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set Bluefruit to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));
}




void loop(void)
{
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  if (len == 0) return;

  // Buttons
  if (packetbuffer[1] == 'B') {
    uint8_t buttnum = packetbuffer[2] - '0';
    boolean pressed = packetbuffer[3] - '0';
    Serial.print ("Button "); Serial.print(buttnum);
    if (pressed) {
      Serial.println(" pressed");

    } else {
      Serial.println(" released");
    }
  }

  // Buttons
  if (packetbuffer[1] == 'B') {

    boolean pressed = packetbuffer[3] - '0';

    // movement buttons
    if ((packetbuffer[2] >= '5') && (packetbuffer[2] <= '8')) {
      Serial.println("YES");

      // forward
      if ((packetbuffer[2] == '5') && pressed) {
        Serial.println("forward");
        myMotor->setSpeed(225);
        myOtherMotor->setSpeed(225);
        myMotor->run(FORWARD);
        myOtherMotor->run(FORWARD);
      }

      // backward
      else if ((packetbuffer[2] == '6') && pressed) {
        Serial.println("backward");
        myMotor->setSpeed(225);
        myOtherMotor->setSpeed(225);
        myMotor->run(BACKWARD);
        myOtherMotor->run(BACKWARD);
      }

      // left
      else if ((packetbuffer[2] == '7') && pressed) {
        Serial.println("right");
        myMotor->setSpeed(255);
        myMotor->run(FORWARD);
      }

      else if ((packetbuffer[2] == '8') && pressed) {
        Serial.println("left");
        myOtherMotor->setSpeed(225);
        myOtherMotor->run(FORWARD);
      }


      else {
        Serial.println(" stop");
        myMotor->run(RELEASE);
        myOtherMotor->run(RELEASE);
      }
    }


    else {
      Serial.println("HERE");

      ///////////////////////////////
      //            happy         //
      ///////////////////////////////

      if ((packetbuffer[2] == '1') && pressed) {
        pixels.clear();
        eye.clear();
        strip.clear();

        if ( happy == 0 ) {
          int c = 0;
          while (c != 13) {
            for (int i = 0; i < 133; i++) {
              if ((i % 13) == c) {
                strip.setPixelColor(i, strip.Color(50, 50, 50));
              }
            }
            for (int i = 0; i < 49; i++) {
              if ((i % 13) == c) {
                pixels.setPixelColor(i, strip.Color(50, 50, 50));
              }
            }
            for (int i = 0; i < 7; i++) {
              if ((i % 13) == c) {
                eye.setPixelColor(i, strip.Color(50, 50, 50));
              }
            }

            pixels.show(); // Send the updated pixel colors to the hardware.
            eye.show();
            strip.show();
            delay(200);
            c++;
          }
          happy ++;
        }

        else {
          pixels.setPixelColor(44, pixels.Color(255, 255, 28));
          pixels.setPixelColor(46, pixels.Color(30, 144, 255));
          pixels.setPixelColor(47, pixels.Color(30, 144, 255));
          pixels.setPixelColor(48, pixels.Color(255, 255, 28));

          eye.setPixelColor(2, eye.Color(255, 255, 28));
          eye.setPixelColor(4, eye.Color(255, 255, 28));
          eye.setPixelColor(5, eye.Color(255, 255, 28));
          eye.setPixelColor(6, eye.Color(255, 255, 28));

          pixels.setPixelColor(9,  pixels.Color(199, 21, 133));
          pixels.setPixelColor(16, pixels.Color(199, 21, 133));
          pixels.setPixelColor(18, pixels.Color(199, 21, 133));
          pixels.setPixelColor(24, pixels.Color(199, 21, 133));
          pixels.setPixelColor(27, pixels.Color(199, 21, 133));
          pixels.setPixelColor(32, pixels.Color(199, 21, 133));
          pixels.setPixelColor(36, pixels.Color(199, 21, 133));
          pixels.setPixelColor(37, pixels.Color(199, 21, 133));
          pixels.setPixelColor(38, pixels.Color(199, 21, 133));
          pixels.setPixelColor(39, pixels.Color(199, 21, 133));

          for (int i = 0; i < 135; i++) {
            strip.setPixelColor(i, strip.Color(0, 255, 140));
          }
        }


        pixels.show(); // Send the updated pixel colors to the hardware.
        eye.show();
        strip.show();

      }

      ///////////////////////////////
      //        talking            //
      ///////////////////////////////

      else if ((packetbuffer[2] == '2') && pressed) {
        pixels.clear();
        eye.clear();
        strip.clear();

        pixels.setPixelColor(42, pixels.Color(255, 255, 28));
        pixels.setPixelColor(43, pixels.Color(255, 255, 28));
        pixels.setPixelColor(44, pixels.Color(255, 255, 28));
        pixels.setPixelColor(45, pixels.Color(255, 255, 28));
        pixels.setPixelColor(46, pixels.Color(255, 255, 28));
        pixels.setPixelColor(47, pixels.Color(255, 255, 28));
        pixels.setPixelColor(48, pixels.Color(255, 255, 28));
        pixels.setPixelColor(49, pixels.Color(255, 255, 28));

        eye.setPixelColor(1, eye.Color(255, 255, 28));
        eye.setPixelColor(2, eye.Color(255, 255, 28));
        eye.setPixelColor(3, eye.Color(255, 255, 28));
        eye.setPixelColor(4, eye.Color(255, 255, 28));
        eye.setPixelColor(5, eye.Color(255, 255, 28));
        eye.setPixelColor(6, eye.Color(255, 255, 28));

        pixels.setPixelColor(3, pixels.Color(255, 255, 28));
        pixels.setPixelColor(4, pixels.Color(255, 255, 28));
        pixels.setPixelColor(5, pixels.Color(255, 255, 28));

        pixels.setPixelColor(14, pixels.Color(255, 255, 28));
        pixels.setPixelColor(11, pixels.Color(255, 255, 28));
        pixels.setPixelColor(19, pixels.Color(255, 255, 28));
        pixels.setPixelColor(23, pixels.Color(255, 255, 28));
        pixels.setPixelColor(31, pixels.Color(255, 255, 28));
        pixels.setPixelColor(28, pixels.Color(255, 255, 28));
        ////
        pixels.setPixelColor(37, pixels.Color(255, 255, 28));
        pixels.setPixelColor(38, pixels.Color(255, 255, 28));

        for (int i = 0; i < 133; i++) {
          if (i % 2 == 0) {
            strip.setPixelColor(i, strip.Color(255, 255, 0));
          }
          else {
            strip.setPixelColor(i, strip.Color(148, 0, 211));
          }
        }

        pixels.show(); // Send the updated pixel colors to the hardware.
        eye.show();
        strip.show();
      }


      ///////////////////////////////
      //         sad
      ///////////////////////////////

      else if ((packetbuffer[2] == '3') && pressed) {
        pixels.clear();
        eye.clear();
        strip.clear();

        eye.setPixelColor(0, eye.Color(30, 144, 255));
        eye.setPixelColor(5, eye.Color(30, 144, 255));
        eye.setPixelColor(6, eye.Color(30, 144, 255));
        eye.setPixelColor(7, eye.Color(30, 144, 255));


        pixels.setPixelColor(42, pixels.Color(30, 144, 255));
        pixels.setPixelColor(47, pixels.Color(30, 144, 255));
        pixels.setPixelColor(48, pixels.Color(30, 144, 255));
        pixels.setPixelColor(49, pixels.Color(30, 144, 255));

        pixels.setPixelColor(11, pixels.Color(30, 144, 255));
        pixels.setPixelColor(12, pixels.Color(30, 144, 255));
        pixels.setPixelColor(13, pixels.Color(30, 144, 255));
        pixels.setPixelColor(14, pixels.Color(30, 144, 255));
        pixels.setPixelColor(19, pixels.Color(30, 144, 255));
        pixels.setPixelColor(23, pixels.Color(30, 144, 255));
        pixels.setPixelColor(27, pixels.Color(30, 144, 255));
        pixels.setPixelColor(32, pixels.Color(30, 144, 255));
        pixels.setPixelColor(35, pixels.Color(30, 144, 255));
        pixels.setPixelColor(40, pixels.Color(30, 144, 255));

        for (int i = 0; i < 135; i++) {
          strip.setPixelColor(i, strip.Color(30, 144, 255));
        }

        pixels.show(); // Send the updated pixel colors to the hardware.
        eye.show();
        strip.show();
      }




      else if ((packetbuffer[2] == '4') && pressed) {
        int count = 0;
        pos = 180;

        //          dance           //
        if (four == 0) {

          strip.clear();
          checkBLE();

          ////////////////////
          //       1        //
          ///////////////////

          //  // Fill along the length of the strip in various colors..
          colorWipe(strip.Color(251,   60,   245), 5);
          checkBLE();
          colorWipe(strip.Color(108, 4,   168), 5);
          checkBLE();
          colorWipe(strip.Color(48,   159,   255), 5);
          checkBLE();

          // Do a theater marquee effect in various colors...
          theaterChase(strip.Color(249, 255, 66), 60); // White, half brightness
          checkBLE();
          //        theaterChase(strip.Color(127,   0,   0), 50); // Red, half brightness
          //        checkBLE();
          //        theaterChase(strip.Color(  0,   0, 127), 50); // Blue, half brightness


          ////////////////////
          //       2        //
          ///////////////////

          colorWipe(strip.Color(240,   240,   38), 10);
          checkBLE();
          colorWipe(strip.Color(86,   240,   117), 10);
          checkBLE();
          //        colorWipe(strip.Color(94,   240,   230), 10);

          // Do a theater marquee effect in various colors...
          theaterChase(strip.Color(48,   159,   255), 60); // White, half brightness
          checkBLE();

          ////////////////////
          //       3        //
          ///////////////////

          colorWipe(strip.Color(168,   29,   66), 1);
          RcolorWipe(strip.Color(136,   32,   168), 1);
          checkBLE();
          colorWipe(strip.Color(240,   128,   43), 1);
          RcolorWipe(strip.Color(34,   141,   168), 1);
          checkBLE();
          colorWipe(strip.Color(39,   168,   112), 0.8);
          RcolorWipe(strip.Color(240,   240,   38), 0.8);
          checkBLE();

          theaterChaseRainbow(60); // Rainbow-enhanced theaterChase variant
          checkBLE();

          theaterChase(strip.Color(100,   170,   200), 10); // White, half brightness
          theaterChase(strip.Color(100,   170,   200), 10); // White, half brightness
          checkBLE();

          RcolorWipe(strip.Color(150,   150,   0), 5);
          checkBLE();
          colorWipe(strip.Color(0,   150,   150), 5);
          checkBLE();

          ////////////////////
          //       4        //
          ///////////////////

          theaterChase(strip.Color(0,   150,   150), 130);
          checkBLE();

          RcolorWipe(strip.Color(136,   32,   168), 1);
          checkBLE();
          four ++;
        }

        else {
          //          glitch           //
          while (count < 28) {

            for (int i = 0; i < 20; i++) {
              int a = i * 2;
              int b = i * 3;
              pixels.setPixelColor(random(0, 49), pixels.Color(random(0, 255), random(0, 255), random(0, 255)));
              eye.setPixelColor(random(0, 8), eye.Color(30, 144, 255));
              strip.setPixelColor(random(0, i), strip.Color(random(0, 255), random(0, 255), random(0, 255)));
              strip.setPixelColor(random(i, a), strip.Color(random(0, 255), random(0, 255), random(0, 255)));
              strip.setPixelColor(random(a, b), strip.Color(random(0, 255), random(0, 255), random(0, 255)));
            }
            pixels.show(); // Send the updated pixel colors to the hardware.
            strip.show();
            eye.show();
            delay (130);
            pixels.clear();
            eye.clear();
            strip.clear();
            count ++;
          }

          for (int i = 0; i < 49; i++) {
            pixels.setPixelColor(i, pixels.Color(0, 0, 0));
            eye.setPixelColor(i, eye.Color(0, 0, 0));
            strip.setPixelColor(i, strip.Color(0, 0, 0));
            strip.setPixelColor(i * 2, strip.Color(0, 0, 0));
            strip.setPixelColor(i * 3, strip.Color(0, 0, 0));
            pixels.show(); // Send the updated pixel colors to the hardware.
            eye.show();
            strip.show();
          }
          // reset everything
          four = 0;
          happy = 0;
        }
      }
    }
  }
}





// functions for creating animated effects

// Fill strip pixels one after another with a color
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

// Fill strip pixels in reverse order one after another with a color
void RcolorWipe(uint32_t color, int wait) {
  for (int i = strip.numPixels(); i > 0; i--) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights
void theaterChase(uint32_t color, int wait) {
  for (int a = 0; a < 10; a++) { // Repeat 10 times...
    for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for (int c = b; c < strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for (int a = 0; a < 15; a++) { // Repeat 30 times...
    for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for (int c = b; c < strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}

// check bluetooth for messages
void checkBLE() {

  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  if (len == 0) return;

  boolean pressed = packetbuffer[3] - '0';

  // check for forward, backward, left, right
  if ((packetbuffer[2] >= '5') && (packetbuffer[2] <= '8')) {
    Serial.println("YES");
    if ((packetbuffer[2] == '5') && pressed) {
      Serial.println("forward");
      myMotor->setSpeed(130);
      myOtherMotor->setSpeed(130);
      myMotor->run(FORWARD);
      myOtherMotor->run(FORWARD);
    }

    else if ((packetbuffer[2] == '6') && pressed) {
      Serial.println("backward");
      myMotor->setSpeed(130);
      myOtherMotor->setSpeed(130);
      myMotor->run(BACKWARD);
      myOtherMotor->run(BACKWARD);
    }

    else if ((packetbuffer[2] == '7') && pressed) {
      Serial.println("right");
      myMotor->setSpeed(255);
      myMotor->run(FORWARD);
    }

    else if ((packetbuffer[2] == '8') && pressed) {
      Serial.println("left");
      myOtherMotor->setSpeed(225);
      myOtherMotor->run(FORWARD);
    }


    else {
      Serial.println(" stop");
      myMotor->run(RELEASE);
      myOtherMotor->run(RELEASE);
    }
  }
}

