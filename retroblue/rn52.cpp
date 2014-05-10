/********************************************************
 * Implementation file for RN52 Bluetooth audio module support.
 *
 * Datasheet: http://ww1.microchip.com/downloads/en/DeviceDoc/rn-52-ds-1.1r.pdf
 * Command reference: http://ww1.microchip.com/downloads/en/DeviceDoc/rn-bt-audio-ug-2.0r.pdf
 *
 * Code developed in Arduino 1.0.5 on Arduino Uno.
 ********************************************************/

#include "rn52.h"
#include <Arduino.h>

const String RN52::COMMAND_TERMINATOR = "\r";
const String RN52::RESPONSE_TERMINATOR = "\r\n";

RN52::RN52(Stream* s) : serialPort(s) {}

String RN52::issueCommand(String command, boolean checkForTerminator) {  
  serialPort->print(command + COMMAND_TERMINATOR);
  
  unsigned long startTime = millis();
  String buffer;
  while (startTime + RESPONSE_WAIT_TIME > millis()) {
    if (serialPort->available() > 0) buffer.concat(char(serialPort->read()));
    
    if (checkForTerminator && buffer.endsWith(RESPONSE_TERMINATOR)) {
      /* chop off terminator */
      return buffer.substring(0, buffer.length() - RESPONSE_TERMINATOR.length());
    }
  }
  
  return buffer;
}

String RN52::dialNumber(String num) {
  return issueCommand("A," + num, true);
}

String RN52::acceptCall() {
  return issueCommand("C", true);
}

String RN52::endCall() {
  return issueCommand("E", true);
}

String RN52::status() {
  return issueCommand("Q", true);
}

String RN52::settings() {
  return issueCommand("D", false);
}

String RN52::help() {
  return issueCommand("H", false);
}

String RN52::firmwareVersion() {
  return issueCommand("V", false);
}
