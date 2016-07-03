/* Copyright (c) 2010 bildr community and 2014 John C G Sturdy

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

 Public domain software from SparkFun also used in the base.
*/

/* Bizarrely, this is needed to make the arduino compiler accept ifdefs */
#if 1
__asm volatile ("nop");
#endif

#define JCGS_DEBUG 1
#define DEBUG_BY_LED 1

#include "mpr121.h"
/* todo: probably switch to http://todbot.com/blog/2010/09/25/softi2cmaster-add-i2c-to-any-arduino-pins */
#include <Wire.h>

#include <Serial.h>

#ifdef JCGS_DEBUG
#include <SoftwareSerial.h>

SoftwareSerial debugSerial(7, 8); // RX, TX
#endif

int debug_led_pin = 13;

#ifdef DEBUG_BY_LED
void
debug_by_flashing(int count, int on, int off)
{
  int i;
  for (i = 0; i < count; i++) {
    delay(on * 100);
    digitalWrite(debug_led_pin, HIGH);
    delay(off * 100);
    digitalWrite(debug_led_pin, LOW);
  }
}

/* The pin used for each bit */
int bit_1;
int bit_2;
int bit_4;
int bit_8;

void
status_digit_setup(int b1, int b2, int b4, int b8)
{
  bit_1 = b1;
  bit_2 = b2;
  bit_4 = b4;
  bit_8 = b8;
  pinMode(bit_1, OUTPUT);
  pinMode(bit_2, OUTPUT);
  pinMode(bit_4, OUTPUT);
  pinMode(bit_8, OUTPUT);
}

void
status_digit(int digit)
{
  digitalWrite(bit_1, digit & 1);
  digitalWrite(bit_2, (digit >> 1) & 1);
  digitalWrite(bit_4, (digit >> 2) & 1);
  digitalWrite(bit_8, (digit >> 3) & 1);
}
#endif
int irq_pin = 2;  // Digital 2
boolean touch_states[12]; // to keep track of the previous touch states

/* up to 4 chips per Wire connection */
#define N_SENSOR_CHIPS 1

/* todo: two banks of sensor chips, on different i2c buses?  But the
   Arduino does TWI partly in hardware, so natively it can only do one
   I2C bus.  Perhaps I should just limit myself to 48 keys? */
int MPR_addresses[N_SENSOR_CHIPS] = { 0x5A
				      // , 0x5B, 0x5C, 0x5D
};


static int in_command = 0;

int
expect(char *expected) {
  // wait for the Mate to send back CMD
  while (*expected != '\0') {
    while (Serial.available() == 0) {
      /* do nothing */
    }
    if (Serial.read() != *expected) {
      return 0;
    }
    expected++;
  }
  return 1;
}

void
switch_to_command() {
  if (!in_command) {
    Serial.print("$$$\r");
    in_command = expect("CMD");
  }
}

void
switch_to_data() {
  if (in_command) {
    Serial.print("---\r");
    in_command = !expect("END");
  }
}

void
switch_to_SPP() {
  switch_to_command();
  Serial.print("S~,0\rR,1\r");
  delay(1000);
  in_command = 0;
}

void
switch_to_HID() {
  switch_to_command();
  Serial.print("S~,6\rR,1\r");
  delay(1000);
  in_command = 0;
}

void
loop(){
  readTouchInputs();
  if (touch_states[0] != 0) {
    digitalWrite(debug_led_pin, HIGH);
  } else {
    digitalWrite(debug_led_pin, LOW);
  }
}

void
setup() {
  pinMode(debug_led_pin,OUTPUT);
  digitalWrite(debug_led_pin, HIGH);
#ifdef JCGS_DEBUG
  debugSerial.begin(9600);
  debugSerial.print("Starting setup\n");
#endif
#ifdef DEBUG_BY_LED
  status_digit_setup(9, 10, 11, 12);
  status_digit(0);
#endif
  Serial.begin(115200);	   // The Bluetooth Mate defaults to 115200bps
  switch_to_command();	   /* must be done in the first 60 seconds */
  digitalWrite(debug_led_pin, LOW);
#ifdef DEBUG_BY_LED
  status_digit(1);
#endif

  /* This is for the modem setup
     https://learn.sparkfun.com/tutorials/using-the-bluesmirf/example-code-using-command-mode
     but with no rationale */
  Serial.println("U,9600,N"); // Temporarily Change the baudrate to 9600, no parity
  Serial.begin(9600);	      // Start bluetooth serial at 9600

  /* Bluetooth module setup */
  // https://www.sparkfun.com/datasheets/Wireless/Bluetooth/rn-bluetooth-um.pdf lists commands
  // todo: enable sniff mode?
  Serial.println("SN,vambracekeyboard");
  expect("AOK");
#ifdef DEBUG_BY_LED
  status_digit(2);
#endif

  /* todo: set up as bluetooth keyboard, which is based on USB HID */
  /* see http://www.kobakant.at/DIY/?p=3310
     see also http://www.adafruit.com/product/1535
     and http://forum.arduino.cc/index.php?topic=18587.0
  */

  switch_to_SPP();
  Serial.println("Test transmission");
  switch_to_HID();
#ifdef DEBUG_BY_LED
  status_digit(3);
#endif
  
  switch_to_data();

  pinMode(irq_pin, INPUT);
  digitalWrite(irq_pin, HIGH);	// enable pullup resistor

  Wire.begin();			/* A4 is SDA, A5 is SCL (http://arduino.cc/en/Main/arduinoBoardNano) */
#ifdef DEBUG_BY_LED
  status_digit(4);
#endif

  for (int sensor = 0; sensor < N_SENSOR_CHIPS; sensor++) {
    setup_one_mpr121(MPR_addresses[sensor]);
  }

#ifdef DEBUG_BY_LED
  debug_by_flashing(4, 5, 5);
#endif
  
  digitalWrite(debug_led_pin, LOW);
  
#ifdef JCGS_DEBUG
  debugSerial.print("Finished setup\n");
#endif
#ifdef DEBUG_BY_LED
  status_digit(5);
#endif
}

void
readTouchInputs() {
  if (!checkInterrupt()) {
    for (int sensor_chip = 0; sensor_chip < N_SENSOR_CHIPS; sensor_chip++) {

      // read the touch state from the MPR121
      Wire.requestFrom(MPR_addresses[sensor_chip], 2);

      byte LSB = Wire.read();
      byte MSB = Wire.read();

      uint16_t touched = ((MSB << 8) | LSB); // 16 bits that make up the touch states

      for (int electrode=0; electrode < 12; electrode++) {  // Check what electrodes were pressed
	int key = (sensor_chip * 12) + electrode;
	if (touched & (1<<electrode)) {

	  if (touch_states[key] == 0) {
	    // key was just touched
#ifdef DEBUG_BY_LED
	    debug_by_flashing(3, 8, 2);
#endif
#ifdef DEBUG_BY_LED
  status_digit(9);
#endif
#ifdef JCGS_DEBUG
	    debugSerial.print("key "); Serial.print(key); Serial.println(" was just touched\n");
#endif
	  } else if (touch_states[key] == 1) {
	    // key is still being touched
	  }

	  touch_states[key] = 1;
	} else {
	  if (touch_states[key] == 1) {
	    // key is no longer being touched
#ifdef DEBUG_BY_LED
	    debug_by_flashing(3, 2, 8);
#endif
#ifdef DEBUG_BY_LED
  status_digit(8);
#endif
#ifdef JCGS_DEBUG
	    debugSerial.print("key "); Serial.print(key); Serial.println(" is no longer being touched\n");
#endif
	  }
	  touch_states[key] = 0;
	}
      }
    }
  }
}

void
setup_one_mpr121(int address) {
  // http://www.hobbytronics.co.uk/datasheets/MPR121.pdf is useful for detail
  set_register(address, ELE_CFG, 0x00); // stop mode, allows setting other registers

  // Section A - Controls filtering when data is > baseline.
  set_register(address, MHD_R, 0x01);
  set_register(address, NHD_R, 0x01);
  set_register(address, NCL_R, 0x00);
  set_register(address, FDL_R, 0x00);

  // Section B - Controls filtering when data is < baseline.
  set_register(address, MHD_F, 0x01);
  set_register(address, NHD_F, 0x01);
  set_register(address, NCL_F, 0xFF);
  set_register(address, FDL_F, 0x02);

  // Section C - Sets touch and release thresholds for each electrode
  set_register(address, ELE0_T, TOU_THRESH);
  set_register(address, ELE0_R, REL_THRESH);

  set_register(address, ELE1_T, TOU_THRESH);
  set_register(address, ELE1_R, REL_THRESH);

  set_register(address, ELE2_T, TOU_THRESH);
  set_register(address, ELE2_R, REL_THRESH);

  set_register(address, ELE3_T, TOU_THRESH);
  set_register(address, ELE3_R, REL_THRESH);

  set_register(address, ELE4_T, TOU_THRESH);
  set_register(address, ELE4_R, REL_THRESH);

  set_register(address, ELE5_T, TOU_THRESH);
  set_register(address, ELE5_R, REL_THRESH);

  set_register(address, ELE6_T, TOU_THRESH);
  set_register(address, ELE6_R, REL_THRESH);

  set_register(address, ELE7_T, TOU_THRESH);
  set_register(address, ELE7_R, REL_THRESH);

  set_register(address, ELE8_T, TOU_THRESH);
  set_register(address, ELE8_R, REL_THRESH);

  set_register(address, ELE9_T, TOU_THRESH);
  set_register(address, ELE9_R, REL_THRESH);

  set_register(address, ELE10_T, TOU_THRESH);
  set_register(address, ELE10_R, REL_THRESH);

  set_register(address, ELE11_T, TOU_THRESH);
  set_register(address, ELE11_R, REL_THRESH);

  // Section D
  // Set the Filter Configuration
  // Set ESI2
  set_register(address, FIL_CFG, 0x04);

  // Section E
  // Electrode Configuration
  // Set ELE_CFG to 0x00 to return to standby mode
  set_register(address, ELE_CFG, 0x0C);  // Enables all 12 Electrodes


  // Section F
  // Enable Auto Config and auto Reconfig
  /*set_register(address, ATO_CFG0, 0x0B);
    set_register(address, ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   set_register(address, ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
    set_register(address, ATO_CFGT, 0xB5);*/  // Target = 0.9*USL = 0xB5 @3.3V

  set_register(address, ELE_CFG, 0x0C);

}

boolean
checkInterrupt(void) {
  return digitalRead(irq_pin);
}

void
set_register(int address, unsigned char r, unsigned char v) {
    Wire.beginTransmission(address);
    Wire.write(r);
    Wire.write(v);
    Wire.endTransmission();
}
