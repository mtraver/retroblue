/********************************************************
 * Header file for RN52 Bluetooth audio module support.
 *
 * Code developed in Arduino 1.0.5 on Arduino Uno.
 ********************************************************/

#ifndef RN52_h
#define RN52_h

#include <Arduino.h>
#include <SoftwareSerial.h>

class RN52 {
public:
    RN52(Stream* s);
    String issueCommand(String command, boolean checkForTerminator);
    String dialNumber(String num);
    String acceptCall();
    String endCall();
    String status();
    String settings();
    String help();
    String firmwareVersion();

private:
    /* command and response line endings  */
    static const String COMMAND_TERMINATOR;
    static const String RESPONSE_TERMINATOR;
    
    /* the amount of time we give the Bluetooth module to send a response, in ms */
    static const int RESPONSE_WAIT_TIME = 3000;
    
    Stream* serialPort;
};

#endif
