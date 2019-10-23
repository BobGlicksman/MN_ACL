// This file holds common utilities that are used by modules in the RFID project
// of Maker Nexus
//
// (CC BY-NC-SA 3.0 US) 2019 Maker Nexus   https://creativecommons.org/licenses/by-nc-sa/3.0/us/
//

#include "application.h"

//#define TEST     // uncomment for debugging mode
#define RFID_READER_PRESENT  // uncomment when an RFID reader is connected
#define LCD_PRESENT  // uncomment when an LCD display is connected

// This #include statement was automatically added by the Particle IDE.
#include <LiquidCrystal.h>
extern LiquidCrystal lcd;

//Required to get ArduinoJson to compile
#define ARDUINOJSON_ENABLE_PROGMEM 0
#include <ArduinoJson.h>
//https://arduinojson.org/v6/assistant/

#ifndef MNUTILS_H
#define MNUTILS_H

#define IRQ_PIN D3
#define RST_PIN D4  // not connected
#define ONBOARD_LED_PIN D7

#define READY_LED D4   // xxx second definition of this pin...
#define ADMIT_LED A4
#define REJECT_LED A5
#define BUZZER_PIN D2
#define MN_EPROM_ID 453214  //This value is written to the EEPROM to show that the
                            //data is ours. Change this and every device will have to
                            //be configured again.

typedef enum  {
    IN_PROCESS = 1,
    COMPLETE_OK = 2,
    COMPLETE_FAIL = 3,
    IDLE =4
} enumRetStatus;

typedef enum {
    acIDLE = 1,
    acGETUSERINFO = 2,
    acIDENTIFYCARD = 3,
    acBURNCARDNOW = 4,
    acRESETCARDTOFRESH = 5
} enumAdminCommand;

extern enumAdminCommand g_adminCommand;
extern String g_adminCommandData;

typedef enum {
    UNDEFINED_DEVICE = 0,
    CHECK_IN_DEVICE = 1,
    ADMIN_DEVICE = 2,
    BOSSLASER = 3,
    WOODSHOP = 4
} enumDeviceConfigType;

// you can add more fields to the end of this structure, just don't change
// the order or size of any existing fields.
struct structEEPROMdata {
    int MN_Identifier = 11111111;  // set to MN_EPROM_ID when we write to EEPROM
    enumDeviceConfigType deviceType = UNDEFINED_DEVICE;  // Hold the value that drives the behavior of 
                         // this device. Set via Cloud Function. 
} ;

// xxx this should be accessed via a function call in the module and removed from global scope
extern structEEPROMdata EEPROMdata; 

struct  struct_clientInfo {  // holds info on the current client
    String lastName = "";           // lastName
    String firstName = "";      // just the first lastName
    bool isValid = false;        // when true this sturcture has good data in it
    int clientID = 0;           // numeric value assigned by EZFacility. Guaranteed to be unique
    String RFIDCardKey = "";    // string stored in EZFacility "custom fields". We may want to change this name
    String memberNumber = "";   // string stored in EZFacility. May not be unique
    String contractStatus = ""; // string returned from EZF. Values we know of: Active, Frozen, Cancelled, Suspended
    int amountDue = 0;          // from EZF
} ;

extern struct_clientInfo g_clientInfo;

extern String JSONParseError;

extern bool allowDebugToPublish;  // when false, debugEvent will hold off on publishing
                                   // needed when in a partcle cloud callback routine
 

void buzzerBadBeep();

void buzzerGoodBeep();

void buzzerGoodBeeps2();

void buzzerGoodBeeps3();

void writeToLCD(String line1, String line2);

//-------------- Particle Publish Routines --------------
// These routines are used to space out publish events to avoid being throttled.
//

int particlePublish (String eventName, String data); 

// Call with any message. It will be buffered and published to the particle could as 
// a debugEvent. View them in the Particle.io console.
// Call this from the main loop with a null message to keep the debug events going 
void debugEvent (String message);

// writes message to a webhook that will send it on to a cloud database
// These have to be throttled to less than one per second on each device
// Parameters:
//    logEvent - a short reason for logging ("checkin","reboot","error", etc)
//    logData - optional freeform text up to 250 characters
//    clientID - optional if this event was for a particular client 
void logToDB(String logEvent, String logData, int clientID, String clientFirstName);

// similar to logToDB, but calls the webhook RFIDLogCheckInOut
void logCheckInOut(String logEvent, String logData, int clientID, String clientFirstName);

// This is the return called by Particle cloud when the RFIDLogging webhook completes
//
void RFIDLoggingReturn (const char *event, const char *data);

// Write to the EEPROM 
//
// 
void EEPROMWrite ();

// Read from EEPROM
//
//
void EEPROMRead();

// Convert eDeviceConfigType to human readable for LCD
//
String deviceTypeToString(enumDeviceConfigType deviceType);

void clearClientInfo();

#endif