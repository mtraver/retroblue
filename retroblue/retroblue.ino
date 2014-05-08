/********************************************************
 * Arduino code used to turn a vintage rotary phone
 * into a 21st century Bluetooth accessory.
 * 
 * Project inspired by tutorial at
 * https://www.sparkfun.com/products/retired/8929.
 ********************************************************/

#include "rn52.h"
#include <SoftwareSerial.h>

const boolean DEBUG = true;

/* pins used to detect the signals from the phone */
const int pulsePin = 2;
const int dialingPin = 3;
const int hookPin = 4;

/* pins used to communicate with the Bluetooth module */
const int eventPin = 7;

/* pin used to activate the ringing solenoid */
const int solenoidPin = 5;

/* ringing state */
const int NOT_RINGING = 0;
const int RINGING = 1;
int ringState = NOT_RINGING;

/* used to track if we're in a call so we can tell the Bluetooth
 * module to end the call when the user hangs up the handset */
const int NOT_IN_CALL = 0;
const int IN_CALL = 1;
int callState = NOT_IN_CALL;

const int NUM_LOOP_ITERS_PER_RING_STATE = 3;

/* The number of times we've passed in the loop in the current solenoidOut
 * state. This is modulus NUM_LOOP_ITERS_PER_RING_STATE. */
int loopItersInRingState = 0;

/* true = solenoid extended; false = retracted */
boolean solenoidOut = true;

/* used to manage ring pulses */
const int RING_INTERVAL_LENGTH = 100;
const float PERCENT_RINGING = 0.6;
const float RINGING_TIME = RING_INTERVAL_LENGTH * PERCENT_RINGING;
int ringIntervalCount = 0;

/* state of pulse switch
 * 1 during dialing pulse
 * 0 otherwise
 */
int pulse;
const int PULSE = 1;
const int NO_PULSE = 0;

/* state of dial
 * 0 if a number is being dialed (i.e. any time dial is not in rest position)
 * 1 if dial is in rest position
 */
int dialing;
const int IS_DIALING = 0;
const int NOT_DIALING = 1;

/* state of hook
 * 0 if phone off hook
 * 1 if phone on hook
 */
int hook;
const int OFF_HOOK = 0;
const int ON_HOOK = 1;

/* Time in milliseconds that various parts of the dialing cycle take.
 * Used for detecting which number has been dialed based on the time
 * elapsed during a dialing cycle. These times should probably be tuned
 * for your specific phone.  */
const int PULSE_TIME = 67;
const int INTER_PULSE_TIME = 42;
const int TOTAL_PULSE_INTERVAL_TIME = PULSE_TIME + INTER_PULSE_TIME;

/* the string the Bluetooth module sends when a command is successful */
const String SUCCESS = "AOK";

/* constants returned by getDialedNumber */
const int INVALID_DIAL = -1;
const int TIMED_OUT = -2;

/* Arduino Uno only has one hardware serial interface, so set up a software
 * serial interface for communicating with the Bluetooth module.
 *
 * RX = digital pin 10, TX = digital pin 11
 */
SoftwareSerial bluetoothSerial(10, 11);

RN52 rn52 = RN52(&bluetoothSerial);

void setup() {
  if (DEBUG) Serial.begin(9600);
  bluetoothSerial.begin(9600);

  /* tell pin to use internal pull-up resistor */
  pinMode(pulsePin, INPUT);
  digitalWrite(pulsePin, HIGH);
  
  /* tell pin to use internal pull-up resistor */
  pinMode(dialingPin, INPUT);
  digitalWrite(dialingPin, HIGH);
  
  /* tell pin to use internal pull-up resistor */
  pinMode(hookPin, INPUT);
  digitalWrite(hookPin, HIGH);
  
  /* tell pin to use internal pull-up resistor */
  pinMode(eventPin, INPUT);
  digitalWrite(eventPin, HIGH);
  
  pinMode(solenoidPin, OUTPUT);

  bluetoothSerial.listen();
}

void print(String s) {
  if (DEBUG) Serial.print(s);
}

void println(String s) {
  if (DEBUG) Serial.println(s);
}

/* reads signals from the phone and updates dialing state variables */
void readState() {
  pulse = digitalRead(pulsePin);
  dialing = digitalRead(dialingPin);
  hook = digitalRead(hookPin);
}

/* updates ringing/call status */
void updateStatuses() {
  println("Getting Bluetooth module status...");
  String status = rn52.status();
  println("Received: " + status);
  
  char lastChar = status.charAt(status.length() - 1);

  if (lastChar == '5') {
    println("Incoming call.");
    startRinging();
  } else {
    stopRinging();
  }

  print("Processed status: ");
  println(status);
}

void startRinging() {
  ringState = RINGING;
}

void stopRinging() {
  ringState = NOT_RINGING;
  loopItersInRingState = 0;
  solenoidOut = true;
  digitalWrite(solenoidPin, solenoidOut ? LOW : HIGH);
}

void doRingTick() {
  if (ringIntervalCount < RINGING_TIME && loopItersInRingState == 0) {
      digitalWrite(solenoidPin, HIGH);
      solenoidOut = true;
  } else if (solenoidOut) {
    digitalWrite(solenoidPin, LOW);
    solenoidOut = false;
  }

  ringIntervalCount = (ringIntervalCount + 1) % RING_INTERVAL_LENGTH;
  loopItersInRingState = (loopItersInRingState + 1) % NUM_LOOP_ITERS_PER_RING_STATE;
  delay(20);
}

/* Detects and returns a dialed number based on the total time bewteen the
 * first pulse and the dial coming to rest. Will block until either a number
 * is dialed or the phone is hung up. Returns the dialed number (in range [1, 10]),
 * or -1 in the case of error or hang up. */
int getDialedNumber() {
  readState();
  while (dialing == IS_DIALING && hook == OFF_HOOK) {
    readState();
    if (pulse == PULSE) {
      long startTime = millis();

      while (dialing == IS_DIALING) {
        readState();
        if (hook == ON_HOOK) return INVALID_DIAL;
      }

      long elapsedTime = millis() - startTime;

      /* compute dialed number from elapsed time; mod 10 because the phone dials a 0 as if it were a 10 */
      int dialedNumber = elapsedTime / TOTAL_PULSE_INTERVAL_TIME;
      if (dialedNumber <= 10) {
        return dialedNumber % 10;
      } else {
        /* if we got a number greater than 10, something went wrong */
        return INVALID_DIAL;
      }
    }
  }

  return INVALID_DIAL;
}

void loop() {
  readState();

  // TODO When an event happens, the pin is held low for 100ms. Should
  // we be checking how long it's low just to make sure that we don't
  // check the status if some noise happens to come through?
  int event = digitalRead(eventPin);
  if (event == 0) {
    println("Event detected");
    updateStatuses();
  }

  if (ringState == RINGING) {
    doRingTick();
  }
  
  if (hook == OFF_HOOK) {
    delay(175);
    println("OFF hook");
    
    if (ringState == RINGING) {
      println("Accepting call...");
      String response = rn52.acceptCall();
      println("Received: " + response);
      
      stopRinging();
      callState = IN_CALL;
    }

    String numberString = "";
    boolean doDialNumber = true;
    
    for (int numCount = 0; numCount < 10; numCount++) {
      /* wait for the user to start dialing a number */
      readState();
      while (dialing == NOT_DIALING && hook == OFF_HOOK) {
        readState();
      }
      delay(15);

      int dialedNumber = getDialedNumber();
      delay(20);
      
      readState();
      if (hook == ON_HOOK) {
        println("ON hook");

        /* if we're hanging up during an active call, end the call */
        if (callState == IN_CALL) {
          println("Ending call...");
          String response = rn52.endCall();
          println("Received: " + response);
          
          callState = NOT_IN_CALL;
        }

        doDialNumber = false;
        break;
      }
      
      /* only record the latest number dialed if it's valid */
      if (dialedNumber != INVALID_DIAL) {
        numberString = numberString + dialedNumber;
      
        print(String(dialedNumber));
        if (numCount == 2 || numCount == 5) print("-");
      } else {
        numCount--;
      }
    }
    
    print("\n");
    if (doDialNumber) {
      println("Dialing " + numberString + "...");
      String response = rn52.dialNumber(numberString);
      println("Received: " + response);
      
      if (response == SUCCESS) callState = IN_CALL;
    }
  }
}
