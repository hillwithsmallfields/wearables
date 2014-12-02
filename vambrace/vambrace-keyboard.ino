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

#include "mpr121.h"
/* todo: probably switch to http://todbot.com/blog/2010/09/25/softi2cmaster-add-i2c-to-any-arduino-pins */
#include <Wire.h>

int irqpin = 2;  // Digital 2
boolean touchStates[12]; // to keep track of the previous touch states

/* up to 4 chips per Wire connection */
#define N_SENSOR_CHIPS 1

int MPR_addresses[N_SENSOR_CHIPS] = { 0x5A
				      // , 0x5B, 0x5C, 0x5D
}

void
setup() {
  Serial.begin(115200);	   // The Bluetooth Mate defaults to 115200bps
  Serial.print("$$$");	   // Enter command mode
  delay(100);	    // Short delay, wait for the Mate to send back CMD
  Serial.println("U,9600,N"); // Temporarily Change the baudrate to 9600, no parity
  Serial.begin(9600);	      // Start bluetooth serial at 9600

  pinMode(irqpin, INPUT);
  digitalWrite(irqpin, HIGH);	// enable pullup resistor

  Wire.begin();

  for (int sensor = 0; sensor < N_SENSOR_CHIPS; sensor++) {
    setup_one_mpr121(MPR_addresses[sensor]);
  }
}

void
loop(){
  readTouchInputs();
}

void
readTouchInputs() {
  if (!checkInterrupt()) {
    for (int sensor = 0; sensor < N_SENSOR_CHIPS; sensor++) {

      // read the touch state from the MPR121
      Wire.requestFrom(MPR_addresses[sensor], 2);

      byte LSB = Wire.read();
      byte MSB = Wire.read();

      uint16_t touched = ((MSB << 8) | LSB); // 16 bits that make up the touch states

      for (int electrode=0; electrode < 12; electrode++) {  // Check what electrodes were pressed
	int key = (sensor * 12) + electrode
	if (touched & (1<<electrode)) {

	  if (touchStates[key] == 0) {
	    // key was just touched
	    Serial.print("key "); Serial.print(key); Serial.println(" was just touched");

	  } else if (touchStates[key] == 1) {
	    // key is still being touched
	  }

	  touchStates[key] = 1;
	} else {
	  if (touchStates[key] == 1) {
	    // key is no longer being touched
	    Serial.print("key "); Serial.print(key); Serial.println(" is no longer being touched");
	  }
	  touchStates[key] = 0;
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
  return digitalRead(irqpin);
}

void
set_register(int address, unsigned char r, unsigned char v) {
    Wire.beginTransmission(address);
    Wire.write(r);
    Wire.write(v);
    Wire.endTransmission();
}
