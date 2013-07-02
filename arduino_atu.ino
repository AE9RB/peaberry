// Copyright 2013 David Turnbull AE9RB
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This is an example application for Arduino to read band
// data from the Peaberry V2 ATU port. Typical usage is to
// change the band on your amplifier or switch antennas.

// No support circuitry is needed.
// Connect a 1/8" jack directly to Arduino pin.
// Leave the ring unconnected, it is not used.
#define ATU_0_PIN 2

void setup() {
  Serial.begin(9600);
}

// This example prints the band number to the serial
// port whenever a change is detected.
void loop() {
  static int band = 0;
  int i = atu_read();
  
  if (i) {
    if (band != i) {
      Serial.println(i);
    }
    band = i;
  }
}

// Returns a non-zero value when the read is complete.
int atu_read() {
  static int state = 5, data[4], previous;
  static long t;
  long m = micros();
  int i, ret = 0;
  
  if (state < 6) switch(state) {
  default:
    i = digitalRead(ATU_0_PIN);
    if (m - t > 7000) state = 5;
    if (previous == HIGH && i == LOW) {
      data[state] = m - t;
      state++;
    }
    if (previous == LOW && i == HIGH) {
      t = m;
    }
    previous = i;
    break;
  case 4:
    for (i=0; i<4; i++) {
      ret <<= 1;
      if (data[i] > 2750) ret |= 0x01;
    }
    //nobreak;
  case 5:
    t = m + 50000;
    state = 6;
    break;
  }
  else if (t - m < 0) switch(state) {
  case 6:
    t = m + 50000;
    digitalWrite(ATU_0_PIN, HIGH);
    state = 7;
    break;
  case 7:
    t = m + 5000;
    digitalWrite(ATU_0_PIN, LOW);
    state = 8;
    break;
  case 8:
    t = m;
    previous = LOW;
    state = 0;
    break;
  } 
  
  return ret;
}
