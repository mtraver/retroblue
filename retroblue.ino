/* Arduino code used to turn a vintage rotary phone
 * into a 21st century Bluetooth accessory.
 * 
 * Project (most specifically, the code that detects
 * dialed numbers) based on tutorial and code at
 * https://www.sparkfun.com/products/retired/8929.
 */

#include <SoftwareSerial.h>

/* pins used to detect the signals from the phone */
const int pulsePin = 2;
const int dialingPin = 3;
const int hookPin = 4;

/* pins used to communicate with the Bluetooth module */
const int eventPin = 7;
const int resetPin = 12;

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

/* command and response line endings  */
const String COMMAND_TERMINATOR = "\r";
const String RESPONSE_TERMINATOR = "\r\n";

/* the string the Bluetooth module sends when a command is successful */
const String SUCCESS = "AOK";

/* the amount of time we give the Bluetooth module to send a response, in ms */
const int RESPONSE_WAIT_TIME = 3000;

/* Arduino Uno only has one hardware serial interface, so set up a software
 * serial interface for communicating with the Bluetooth module.
 *
 * RX = digital pin 10, TX = digital pin 11
 */
SoftwareSerial bluetooth(10, 11);

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);

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
  
  pinMode(resetPin, OUTPUT);
  pinMode(solenoidPin, OUTPUT);

  bluetooth.listen();
}

/* reads signals from the phone and updates dialing state variables */
void readState() {
  pulse = digitalRead(pulsePin);
  dialing = digitalRead(dialingPin);
  hook = digitalRead(hookPin);
}

/* properly terminates the given command, sends it to the Bluetooth module, and waits for a response */
String issueCommand(String command, boolean checkForTerminator) {
  Serial.println("Issuing command '" + command + "'");
  
  bluetooth.print(command + COMMAND_TERMINATOR);
  
  unsigned long startTime = millis();
  String buffer;
  while (startTime + RESPONSE_WAIT_TIME > millis()) {
    if (bluetooth.available() > 0) buffer.concat(char(bluetooth.read()));
    
    if (checkForTerminator && buffer.endsWith(RESPONSE_TERMINATOR)) {
      /* chop off terminator */
      buffer = buffer.substring(0, buffer.length() - RESPONSE_TERMINATOR.length());

      Serial.print("Got EOL. Received:");
      Serial.println(buffer);

      return buffer;
    }
  }
  
  Serial.println("End of wait. Received:");
  Serial.println(buffer);
  return buffer;
}

String dialNumber(String num) {
  return issueCommand("A," + num, true);
}

String acceptCall() {
  return issueCommand("C", true);
}

String endCall() {
  return issueCommand("E", true);
}

String getStatus() {
  return issueCommand("Q", true);
}

String getSettings() {
  return issueCommand("D", false);
}

String getModuleHelp() {
  return issueCommand("H", false);
}

String getFirmwareVersion() {
  return issueCommand("V", false);
}

/* updates ringing/call status */
void updateStatuses() {
  String status = getStatus();
  char lastChar = status.charAt(status.length() - 1);

  if (lastChar == '5') {
    Serial.println("Incoming call.");
    startRinging();
  } else {
    stopRinging();
  }

  Serial.print("Processed status: ");
  Serial.println(status);
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
  // Serial.print("Solenoid state: ");
  // Serial.println(solenoidOut);
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
        if (hook == ON_HOOK) return -1;
      }

      long elapsedTime = millis() - startTime;

      /* compute dialed number from elapsed time */
      return elapsedTime / TOTAL_PULSE_INTERVAL_TIME;
    }
  }

  return -1;
}

void loop() {
  readState();

  // TODO When an event happens, the pin is held low for 100ms. Should
  // we be checking how long it's low just to make sure that we don't
  // check the status if some noise happens to come through?
  int event = digitalRead(eventPin);
  if (event == 0) {
    Serial.println("Event detected!");
    updateStatuses();
  }

  if (ringState == RINGING) {
    doRingTick();
  }
  
  if (hook == OFF_HOOK) {
    delay(100);
    Serial.println("OFF hook");
    
    if (ringState == RINGING) {
      acceptCall();
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
      
      readState();
      if (hook == ON_HOOK) {
        Serial.println("hung up!");

        /* if we're hanging up during an active call, end the call */
        if (callState == IN_CALL) {
          endCall();
          callState = NOT_IN_CALL;
        }

        doDialNumber = false;
        break;
      }

      /* the phone dials a 0 as if it were a 10, so fix that */
      dialedNumber = dialedNumber % 10;
      
      numberString = numberString + dialedNumber;
      
      Serial.print(dialedNumber);
      if (numCount == 2 || numCount == 5) Serial.print("-");
    }
    
    Serial.print("\n");
    if (doDialNumber) {
      if (dialNumber(numberString) == SUCCESS) callState = IN_CALL;
    }
  }
}
