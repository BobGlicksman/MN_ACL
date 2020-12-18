/*********************************************************************
 * MNCheckin
 * 
 * This program is used to verify and checkin members to the Maker Nexus
 * makerspace. The program waits for a correctly formated Mifare RFID card
 * to be presented. Two values are read from the card: clientID and UID. 
 * The program verifies that the CRM system (EzFacility) has a contact with
 * clientID and that the contact record has UID associated with it. This 
 * validates the card and the contact. The program then determines if the 
 * contact has a valid, paid-up membership. If all tests are good a green
 * light and beep are presented. If not a red light and a long buzz are 
 * presented.
 * 
 * RFID Card Functions
 * This program integrates the Adafruit PN532
 * RFID brakout board with Particle (Photon).  The Particle device
 * is interfaced to the Adafruit breakout board using I2C.  Wiring
 * is as follows;
 * 
 *  Photon powered via USB
 *  PN532 breakout board 5.0V to Photon VIN
 *  PN532 breakout board GND to Photon GND (both GND)
 *  PN532 breakout board SDA to Photon D0
 *  PN532 breakout board SCL to Photon D1 
 *  Both SCL and SDA pulled up to Photon +3.3v using 5.6kohm resistors
 *  PN532 breakout board IRQ to Photon D3
 *  Photon D4 not connected but reserved for "RST"
 *  Breakout board jumpers are set for I2C:  SEL 1 is OFF and SEL 0 is ON
 * 
 * A 16x2 lcd display as also connected to the Photon.  LCD wiring is:
 *  lcd VSS to GND.  lcd VDD to +3.3 volts. lcd V0 to wiper of a 10K pot.  
 *    One end of the pot is connected to +3.3. volts and the other end to GND.
 *  lcd RS to Photon A0. lcd RW to GND. lcd EN to Photon A1. lcd D0 - D3 are left unconnected.
 *  lcd D4 to Photon A2.  lcd D5 to Photon A3.  lcd D6 to Photon D5.  lcd D7 to Photon D6.
 *  LCD A to +3.3 volts.  lcd K to GND.
 * 
 * 
 * The program initially waits 5 seconds so that a tty port can be connected for
 * serial console communication.  A "trying to connect ..." message is then
 * printed to the serial port to show that things are ready.  Next, the Photon
 * communicates with the breakout board to read out and print the PN532 firmware
 * version data to show that the Photon is talking to the breakout board.  
 * 
 * This program determines if an ISO14443A card presented to the reader is
 * factory fresh or if it has been formatted as Maker Nexus.  Based upon the
 * determined format, the firmware then changes keys to reverse the format;
 * i.e. make factory fresh to MN or make MN to factory fresh.  After changing
 * the card format, this firmware then writes some data to blocks 0 and 1
 * and reads them back, in order to verfy the key changes.  If the new format
 * is factory fresh, the data written to both blocks is 16 bytes of 0x00.  However,
 * if the new format is MN, the data written to blocks 0 and 1 is test data 
 * that is different for each block.  Thus, the firmware toggles the card
 * back and forth between MN format and factory default format with each 
 * placement and removal of the test card.
 * 
 * 
 * Adapted from code at:
 *   https://github.com/adafruit/Adafruit-PN532/blob/master/examples
 * 
 * Once the clientID and UID have been stored in g_clientInfo the program will 
 * move on to the next steps and mark the clientID record as "checked in" in
 * the CRM system (EzFacility).
 * 
 * 
 * 
 * CheckIn
 * 
 * The program uses Particle.io webhooks to 
 * access and update information in the EZFacility CRM database used
 * by Maker Nexus. Associated with this file is a file called webhooks.txt
 * that contains the JSON code needed to recreate each Particle.io
 * webhook used by this code. Once the webhooks have been created 
 * you will need to modify the webhook ezfGetCheckInToken to have a valid 
 * EZFacility authentication token and username/password.
 * 
 * There are several cloud functions so this program can be tested
 * through the Particle.io console.
 * 
 * The primary call-in is the cloud function RFIDCardRead. Call this
 * with a two value, comma separated string of the form
 *     clientID,UID
 * where clientID is a valid clientID and UID is expected to be found 
 * in the associated record in the EZFacility CRM database. An example is:
 *     21613660,rt56
 * Currently the UID is ignored in this code.
 * 
 * After calling RFIDCardRead, a series of debug events are published
 * for your convenience. When the process successfully completes you
 * can verify the action by going to the EZFacility Admin console and 
 * running the report Check-in Details.
 * 
 * 
 * PRE RELEASE DEVELOPMENT LOG
 * 
 * (c) 2019; Team Practical Projects
 * 
 * Authors: Bob Glicksman, Jim Schrempp
 * 
 *  This is a list of functionality that we want to make sure is in the documentation 
 *      Cloud function SetDeviceType must be called to set up machine. Values are in eDeviceConfigType
 *      Device Type is shown on LCD at end of setup.  
 *      Version string now shows on LCD at end of setup()
 *      prefixed cloud functions with "cloud"
 *      client checkin now validates UID in card against EZF and checks contract status and amount due.
 *      JSON returned to Admin on "card info" now includes "Checkin" which tells what would happen if the
 *                card was used at a checkin terminal
 *      logToDB() which writes to a webhook, the webhook sends data to a url for logging
 *      Admin identifyCard now times out if card not presented in 15 seconds
 *      Set timezone and DST in setup() for use in logToDB()
 *      returned JSON and made Checkin msg "billing issue"
 *      added FirstName to logToDB for display use
 *      identify card now detects previous MN format
 *      resetCard will now make an MN card "factory fresh"
 *      moved all webhook callbacks to one routine
 *      LCD shows checked in or out based on our database
 *      second callback routine for mnlogdb... webhooks
 * 

 * 
 *  1.20 supports device type 4 woodshop door 
 *       device type -1 will buzz once. use this to know you're talking to the right box
 *       Fixed issue #15 where bad cards got denied and ok messages
 *       Fixed woodshop using old package data
 *  1.30 readTheCard now tests for block read failure and reports to user on LCD
 *       readTheCard restructured to minimize time card must be presented
 *  1.40 deployed into production
 *  1.41 added ShopBot to allowed woodshop packages
 *  1.42 has new packages code that relies on mustache template to be used at the webhook
 *  1.5  reads station config from Facility Database. Does not yet use the info.
 *  1.6  added last name to fdb logging
 *  1.61 removed enum for dev type, instead relying on fdb for configuration info
 *  1.7  published a checkin event on successful checkin. useful for lockbox
 *  1.71 includes check of RFID revocation on equipment station checkin 
 *  1.72 blinks admit led when RFID card is successfully read 
 *  1.73 new build with PRODUCTION switch in mnutils.h
 *  1.8  ezfReceiveClientByClientID has been reworked to handle the case where the response is more than one 
 *       part from Particle Cloud and the parts come out of order.
************************************************************************/
#define MN_FIRMWARE_VERSION 1.8

// Our UTILITIES
#include "mnutils.h"

#include "rfidkeys.h"

// Our rfid card UTILITIES
#include "mnrfidutils.h"

// Define the pins we're going to call pinMode on

int led = D0;  // You'll need to wire an LED to this one to see it blink.
int led2 = D7; // This one is the built-in tiny one to the right of the USB jack

//----------- Global Variables

String g_tokenResponseBuffer = "";
String g_cibmnResponseBuffer = "";
String g_cibcidResponseBuffer = "";
String g_clientPackagesResponseBuffer = "";
String g_queryMemberResult = ""; // JSON formatted name:value pairs.  ClientName, ClientID. Admin app will display all values.
                                 // Will also contain errorCode which app should check to be 0. See function cloudQueryMember.
String g_identifyCardResult = ""; // JSON formatted name:value pairs. ClientName, ClientID. Admin app will display all values.
                                 // Will also contain errorCode which app should check to be 0. See function cloudIdentifyCard.
bool g_checkInOutResponseValid = false;
String g_checkInOutResponseBuffer = "";
String g_checkInOutActionTaken = "";
bool g_identifyCardResultIsValid = false;

String g_recentErrors = "";
String debug2 = "";
int debug3 = 0;
int debug4 = 0;
int debug5 = 0;

String g_packages = ""; // Not implemented yet xxx



struct struct_authTokenCheckIn {
   String token = ""; 
   unsigned long goodUntil = 0;   // if millis() returns a value less than this, the token is valid
} g_authTokenCheckIn;

struct struct_clientPackages {
    bool isValid = false;
    String packagesString;
} g_clientPackages;




// ------------------   Forward declarations, when needed
//



// -------------------- UTILITIES -----------------------

template <size_t charCount>
void strcpy_safe(char (&output)[charCount], const char* pSrc)
{
    // Copy the string — don’t copy too many bytes.
    strncpy(output, pSrc, charCount);
    // Ensure null-termination.
    output[charCount - 1] = 0;
}

char * strcat_safe( const char *str1, const char *str2 ) 
{
    char *finalString = NULL;
    size_t n = 0;

    if ( str1 ) n += strlen( str1 );
    if ( str2 ) n += strlen( str2 );

    finalString = (char*) malloc( n + 1 );
    
    if ( ( str1 || str2 ) && ( finalString != NULL ) )
    {
        *finalString = '\0';

        if ( str1 ) strcpy( finalString, str1 );
        if ( str2 ) strcat( finalString, str2 );
    }

    return finalString;
}  


// ------------- firmwareupdatehandler
// Called by Particle OS when a firmware update is about to begin
//
// Will put a message on the LCD screen and turn on the red LED
void firmwareupdatehandler(system_event_t event, int data) {
    switch (data) {
    case firmware_update_begin:
        writeToLCD("Firmware update","in progress");
        digitalWrite(READY_LED,LOW);
        digitalWrite(ADMIT_LED,LOW);
        digitalWrite(REJECT_LED,HIGH);
        break;
    case firmware_update_complete:
        //writeToLCD("Firmeware update","complete");  // xxx this didn't get called
        break;
    case firmware_update_failed:
        //writeToLCD("Firmware update","failed");  // xxx this is called even on successful update??
        break;
    }
}

/***************
 * heartbeatLEDs
 * 
 * Called from Loop() 
 * 
 * Will heartbeat the D7 LED so we can tell that our code is alive
 * 
 */
void heartbeatLEDs() {
    
    static unsigned long lastBlinkTimeD7 = 0;
    static int ledState = HIGH;
    const int blinkInterval = 500;  // in milliseconds
    
    
    if (millis() - lastBlinkTimeD7 > blinkInterval) {
        
        if (ledState == HIGH) {
            ledState = LOW;
        } else {
            ledState = HIGH;
        }
        
        digitalWrite(led, ledState);   // Turn ON the LED pins
        digitalWrite(led2, ledState);
        lastBlinkTimeD7 = millis();
    
    }
    
}

// ---------- Reboot Manager ----------
// rebootSet()
//   Called in the main loop with a 0 parameter
//   Checks to see if a reboot is pending and if so, is it time to do the reboot.
// 
//   Pass in a number of milliseconds to wait for a reboot and it will not 
//   reboot but instead it will set the reboot timer.
//   
void rebootSet(long millisecondsToWait) {

    static unsigned long rebootTime = 4294967290;  // makes it so we don't reboot as soon as the system starts

    if (millisecondsToWait > 0) {

        //set the reboot time
        rebootTime = millis() + millisecondsToWait;
    
    } else if (millis() > rebootTime) {

        System.reset();

    }

}

// ------------- Set Device Type ---------
// Called with a number to set device type to determine behavior
// Valid values are in eDeviceConfigType
//
int cloudSetDeviceType(String data) {

    int deviceType = data.toInt();

    debugEvent("devtypeinput: " + String(deviceType));

    if (deviceType == -1) {
        buzzerGoodBeepOnce();
        return EEPROMdata.deviceType;
    } else if ((deviceType) or (data == "0")) {
        int oldType = EEPROMdata.deviceType;
        logToDB("DeviceTypeChange", String(deviceType),0,"","");
        EEPROMdata.deviceType = deviceType;
        EEPROMWrite(); // push the new devtype into EEPROM 
        writeToLCD("Changed Type","rebooting");
        digitalWrite(REJECT_LED, HIGH);
        // reset variable so mainloop does not change LCD display 
        // (I admit this is a bit of a hack, but we are going to reboot right away anyway and
        //  until we do, we are technically the oldType. The type only changes on reboot.)
        EEPROMdata.deviceType = oldType;  
        rebootSet(1000);
        return 0;
    }

    return 1;

}

/*************
 * reportCardError(cardType)
 *  
 * Alert the user that we had a problem reading the RFID card 
 * 
*/
void reportCardError(uint8_t cardType){

    switch (cardType) {
    case 255:
        writeToLCD("why here?","no card present");
        break;

    case 0:
        Serial.println("\n\nCard is type factory fresh\n");
        logToDB("CardTypeFactoryFresh","",0,"","");
        writeToLCD("Card is not MN","(fresh format)");
        buzzerBadBeep();
        delay(1000);
        break;
    
    case 1:
        // MN formatted card, nothing to report
        #ifdef TEST
            Serial.println("\n\nCard is type Maker Nexus formatted\n");
        #endif
        break;

    case 2:
        // Not an MN format
        Serial.println("\n\nCard is not a known format\n");
        logToDB("CardTypeUnknown","",0,"","");
        writeToLCD("Card type","Unknown");
        buzzerBadBeep();
        delay(1000);
        break;     

    case 3:
        // old format of MN card
        Serial.println("\n\nCard is type Maker Nexus old formatted\n");
        logToDB("CardTypeOldMN","",0,"","");
        writeToLCD("Card is old MN"," ");
        buzzerBadBeep();
        delay(1000);
        break;

    default:
        Serial.println("\n\nCard is type is unexpected\n");
        logToDB("CardTypeUnexpected","",0,"","");
        writeToLCD("Card type","unexpected");
        buzzerBadBeep();
        delay(1000);
        break;
    }

    return;

}

//--------------- particleCallbackMNLOGDB --------------------
// 
// This routine  is registered with the particle cloud to receive any
// events that begin with this device and "mnlogdb" or "fdb". This routine will accept
// the callback and then call the appropriate handler.
//
void particleCallbackMNLOGDB (const char *event, const char *data) {

   // hold off on debugEvent publishing while callback is active
    allowDebugToPublish = false;

    String eventName = String(event);
    String myDeviceID = System.deviceID();
    
    // NOTE: NEVER call particle publish (incuding debugEvent) from any
    // routine called from here. Your Particle  processor will  panic

    if (eventName.indexOf(myDeviceID + "mnlogdbCheckInOut") >= 0) {
    
        mnlogdbCheckInOutResponse(event, data );       

    } else if (eventName.indexOf(myDeviceID + "mnlogdbCheckInOutError") >= 0) {

        debugEvent("mnlogdbCheckinError: " + String(data));

    } else if (eventName.indexOf(myDeviceID + "fdbGetStationConfig") >= 0) {

        fdbReceiveStationConfig(event, data);

    } else {

        debugEvent("unknown particle event 2: " + String(event));

    }

    allowDebugToPublish = true;
    
}


//--------------- particleCallbackEZF --------------------
// 
// This routine  is registered with the particle cloud to receive any
// events that begin with this device and "ezf". This routine will accept
// the callback and then call the appropriate handler.
//
void particleCallbackEZF (const char *event, const char *data) {

    // hold off on debugEvent publishing while callback is active
    allowDebugToPublish = false;

    String eventName = String(event);
    String myDeviceID = System.deviceID();
    
    // NOTE: NEVER call particle publish (incuding debugEvent) from any
    // routine called from here. Your Particle  processor will  panic

    if (eventName.indexOf(myDeviceID + "ezfCheckInToken") >= 0) {
    
        ezfReceiveCheckInToken(event, data );       

    } else if (eventName.indexOf(myDeviceID + "ezfClientByMemberNumber") >= 0) {

        ezfReceiveClientByMemberNumber(event, data );

    } else if (eventName.indexOf(myDeviceID + "ezfClientByClientID") >= 0) {

        ezfReceiveClientByClientID(event, data );

    } else if (eventName.indexOf(myDeviceID + "ezfCheckInClient") >= 0) {

        // known webhook, but we don't do anything with the return code   
        
    } else if (eventName.indexOf(myDeviceID + "ezfGetPackagesByClientID") >= 0) {

        ezfReceivePackagesByClientID(event, data);

    } else {

        debugEvent("unknown particle event 1: " + String(event));
    }

    allowDebugToPublish = true;
    
}

// ------------- Get Authorization Token ----------------

// ezfGetCheckInTokenCloud
// 
//   A debug routine to call from Particle Console to get a token.
//
int ezfGetCheckInTokenCloud (String data) {
    
    ezfGetCheckInToken();
    return 0;
    
}

/**
 * ezfGetCheckInToken
 * 
 * Called to make sure g_authTokenCheckIn is valid.
 * If the token is still valid, this routine does nothing.
 * If the token has expired (or was never initialized) then this starts 
 * the process by calling the webhook ezfCheckInToken. This will not ask for
 * a token if the previous request for a token was less than 20 seconds ago.
 * 
 * Caller should wait until length of g_authTokenCheckIn > 0 after calling this
 * routine.
*/

int ezfGetCheckInToken () {
    
    static unsigned long lastRequest = 0;

    if (g_authTokenCheckIn.goodUntil > millis()){

        // the current token is still good 
        // do nothing

    } else {

        // current token has expired, consider getting a new one 
        if (lastRequest + 20000 > millis()) {

            // we asked for a token less than 20 seconds ago
            // do nothing

        } else {

            // we haven't asked for a token in a while, so ask for one 
            g_authTokenCheckIn.token = "";
            g_tokenResponseBuffer = "";
            Particle.publish("ezfCheckInToken", "", PRIVATE);
            lastRequest = millis();
        
        }

    }

    return 0;
    
}

/********
 * ezfReceiveCheckInToken
 * 
 * Called from particleCallbackEZF
 * Accumulates the responses until a valid JSON is found
 * Puts results in g_authTokenCheckIn
 * 
*/
void ezfReceiveCheckInToken (const char *event, const char *data)  {
    
    // accumulate response data
    g_tokenResponseBuffer = g_tokenResponseBuffer + data;

    static int partsCnt = 0; //xxx
    partsCnt++;
    debugEvent ("Received CI token part "+ String(partsCnt) + String(data).substring(0,15) );
    
    const int capacity = JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> docJSON;
   
    char temp[3000]; //This has to be long enough for an entire JSON response
    strcpy_safe(temp, g_tokenResponseBuffer.c_str());
    
    // will it parse?
    DeserializationError err = deserializeJson(docJSON, temp );
    JSONParseError =  err.c_str();
    if (!err) {
        //We have valid full JSON response (all parts), get the token
        g_authTokenCheckIn.token = String(docJSON["access_token"].as<char*>());
        g_authTokenCheckIn.goodUntil = millis() + docJSON["expires_in"].as<int>()*1000 - 5000;   // set expiry five seconds early
        
        debugEvent ("have token now " + String(millis()) + "  Good Until  " + String(g_authTokenCheckIn.goodUntil) );
   
    }
}



// ------------ ClientInfo Utility Functions -------------------

/*********
 * clientInfoToJSON
 * 
 * Creates a JSON package based on the values in g_clientInfo and g_cardData
 * Includes any passed in error code and message.
 * 
 * parameters
 *      errCode / errMsg  - get put into JSON
 *      includeCardData - if true, g_cardData information is included
 * 
 * Returns JSON string
*/
 
String clientInfoToJSON(int errCode, String errMsg, bool includeCardData){

    //xxx maybe we should not pass in errcode/msg now that readCard does not call testCard

    const size_t capacity = JSON_OBJECT_SIZE(15);
    DynamicJsonDocument doc(capacity);

    doc["ErrorCode"] = errCode;
    doc["ErrorMessage"] = errMsg.c_str();
    String fullName = g_clientInfo.firstName + " " + g_clientInfo.lastName;
    doc["Name"] = fullName.c_str();
    doc["Member Number"] = g_clientInfo.memberNumber.c_str();
    doc["ClientID"] = g_clientInfo.clientID;
    doc["Status"] = g_clientInfo.contractStatus.c_str();

    String allowCheckin = isClientOkToCheckIn();
    if (allowCheckin.length() == 0){
        doc["Checkin"] = "Allowed";
    } else {
        doc["Checkin"] = allowCheckin.c_str();
    }

    if (includeCardData) {
        if (g_cardData.isValid) {
            String msg1 = isCardUIDValid();
            String msg2 = g_cardData.cardStatus + " " + msg1; 
            doc["Card Status"] = msg2.c_str();
        }
    }

    char JSON[1200];
    serializeJson(doc,JSON );
    String rtnValue = String(JSON);
    return rtnValue;

}

/********************************/
// clientInfoFromJSON
//   Parse the client JSON from EZF to load g_clientInfo
//   The EZF get client info by client ID does not return an array.
int clientInfoFromJSON(String data){

const size_t capacity = 3*JSON_ARRAY_SIZE(2) + 2*JSON_ARRAY_SIZE(3) + 10*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(20) + 1050;
    DynamicJsonDocument docJSON(capacity);
   
    char temp[3000]; //This has to be long enough for an entire JSON response
    strcpy_safe(temp, g_cibcidResponseBuffer.c_str());
    
    // will it parse?
    DeserializationError err = deserializeJson(docJSON, temp );
    JSONParseError =  err.c_str();
    if (!err) {
        
        // xxx this should really be in a common routine with (search deserialize client)
        clearClientInfo();

        g_clientInfo.clientID = docJSON["ClientID"].as<int>();
            
        g_clientInfo.contractStatus = docJSON["MembershipContractStatus"].as<char*>();
            
        String fieldName = docJSON["CustomFields"][0]["Name"].as<char*>();
            
        if (fieldName.indexOf("RFID Card UID") >= 0) {
            g_clientInfo.RFIDCardKey = docJSON["CustomFields"][0]["Value"].as<char*>(); 
        }
        
        g_clientInfo.lastName = String(docJSON["LastName"].as<char*>());
        g_clientInfo.firstName = String(docJSON["FirstName"].as<char*>()); 

        g_clientInfo.memberNumber = String(docJSON["MembershipNumber"].as<char*>());  

        g_clientInfo.amountDue  = docJSON["AmountDue"]; 

        g_clientInfo.isValid = true;

        return 0;

    } else {
        debugEvent("JSON parse error " + JSONParseError);
        return 1;
    }
}

/**************************/
// clientInfoFromJSONArray
//   Parse the client JSON array from EZF to load g_clientInfo
//   Member Number is now supposed to be unique but the get API by memberNumber
//   still returns an array.
//
int clientInfoFromJSONArray (String data) {
    
    // try to parse it. Return 1 if fails, else load g_clientInfo and return 0.
    
    DynamicJsonDocument docJSON(2048);
   
    char temp[3000]; //This has to be long enough for an entire JSON response
    strcpy_safe(temp, g_cibmnResponseBuffer.c_str());
    
    // will it parse?
    DeserializationError err = deserializeJson(docJSON, temp );
    JSONParseError =  err.c_str();
    if (!err) {
        
         // is the memberid unique?
         JsonObject root_1 = docJSON[1];
        if (root_1["ClientID"].as<int>() != 0)  {  // is this the correct test?
            
            // should not be possible in EZFacility
            // member id is not unique
            g_recentErrors = "More than one client info in JSON ... " + g_recentErrors;
            writeToLCD("Err. Member Num", "not unique");
            buzzerBadBeep();
            
        } else {
            // xxx this should really be in a common routine with (search deserialize client)

            clearClientInfo();

            JsonObject root_0 = docJSON[0];

            g_clientInfo.clientID = root_0["ClientID"].as<int>();

            if (g_clientInfo.clientID == 0) {

                g_clientInfo.firstName = "Member not found";
                g_clientInfo.isValid = true;

            } else {
            
                g_clientInfo.contractStatus = root_0["MembershipContractStatus"].as<char*>();
                
                String fieldName = root_0["CustomFields"][0]["Name"].as<char*>();
                
                if (fieldName.indexOf("RFID Card UID") >= 0) {
                g_clientInfo.RFIDCardKey = root_0["CustomFields"][0]["Value"].as<char*>(); 
                }

                g_clientInfo.lastName = String(root_0["LastName"].as<char*>());
                g_clientInfo.firstName = String(root_0["FirstName"].as<char*>());

                g_clientInfo.memberNumber = String(root_0["MembershipNumber"].as<char*>());

                g_clientInfo.amountDue = root_0["AmountDue"]; 
                
                g_clientInfo.isValid = true;
            }

        }
        
        return 0;
        
    } else {
        
        debugEvent("JSON parse error " + JSONParseError);
        return 1;
    }
    
}






// ------------- Get Client Info by MemberNumber ----------------

/**********************
 * ezfClientByMemberNumber
 *  
 * parameters
 *    data - numeric string of member number as found in EZFacility
 * 
 * Returns the member number passed in
 * 
 * Creates JSON for memberNumber and calls ezfClientByMemberNumber web hook
 * 
 * Caller should wait for particleCallbackEZF()
 * 
*/
int ezfClientByMemberNumber (String data) {
    
    // if a value is passed in, set g_memberNumber
    String temp = String(data);
    if (temp == "") {
        g_recentErrors = "No Member Number passed in. ... " + g_recentErrors;
        return -999;
    } 
    
    g_cibmnResponseBuffer = "";  // reset last answer

    clearClientInfo();

    // Create parameters in JSON to send to the webhook
    const int capacity = JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> docJSON;
    
    docJSON["access_token"] = g_authTokenCheckIn.token.c_str();
    docJSON["memberNumber"] = temp.c_str();
    
    char output[1000];
    serializeJson(docJSON, output);
    
    int rtnCode = Particle.publish("ezfClientByMemberNumber",output, PRIVATE );
    if (rtnCode){} //XXX
    
    return g_clientInfo.memberNumber.toInt();
}

/**********************
 * ezfReceiveClientByMemberNumber
 * 
 * called by particleCallbackEZF()
*/
void ezfReceiveClientByMemberNumber (const char *event, const char *data)  {
    
    g_cibmnResponseBuffer = g_cibmnResponseBuffer + String(data);

    debugEvent("clientInfoPart "); // + String(data));
    
    clientInfoFromJSONArray(g_cibmnResponseBuffer); // try to parse it

}


// ------------- Get Client Info by ClientID ----------------
// Called by the Particle Console for debug use.
// 
// parameters
//   Data is a numeric string of client ID.
//
// This routine will call ezfClientByClientID to fire off the webhook.
//
// Caller should wait for particleCallbackEZF() to get a response
// When called from Particle Console the result will eventually show up 
// in the variable ClientInfo
//
int ezfClientByClientIDCloud (String data) {
    
    // if a value is passed in, set g_memberNumber
    String temp = String(data);
    if (temp == "") {
        debugEvent("No Client ID passed in from cloud function ");
        // We don't change g_clientID
        return 1;
    } 
    
    if (temp.toInt() == 0){
        debugEvent("Cloud function passed in non-numeric or 0 clientID");
        // We don't change g_clientID
        return 1;
        
    }
    
    ezfClientByClientID(temp.toInt());
    
    return 0;
    
}

/*****************************
 * ezfClientByClientID
 * 
 * Called by particleCallbackEZF()
 */
int ezfClientByClientID (int clientID) {
    
    g_cibcidResponseBuffer = "";  // reset last answer

    clearClientInfo();

    // Create parameters in JSON to send to the webhook
    const int capacity = JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> docJSON;
    
    docJSON["access_token"] = g_authTokenCheckIn.token.c_str();
    docJSON["clientID"] = clientID;
    
    char output[1000];
    serializeJson(docJSON, output);
    
    Particle.publish("ezfClientByClientID",output, PRIVATE );
    
    return 0;
}

/*****************************
 * ezfReceiveClientByClientID
 * 
 * Called by particleCallbackEZF()
 */
void ezfReceiveClientByClientID (const char *event, const char *data)  {

    // Note: the full response might be in several parts and they may not come in 
    // order. 

    debugEvent ("clientInfoPart " + String(data));

    // Temp workaround
    const int MAX_CLIENT_INFO_PIECES = 4;
    static String pieces[MAX_CLIENT_INFO_PIECES] = {"","","",""};
    static unsigned long receiveFirstMS = 0;    // ms when the first package comes in 
    const unsigned long MAX_MSG_WAIT_MS = 14000; // we must get all parts within 14 seconds

    if (millis() - receiveFirstMS > MAX_MSG_WAIT_MS) {
        // we have gone too long, so this must be a new set of parts
        // reset all our local state
        debugEvent("clearing client info receive");
        receiveFirstMS = millis(); 
        for (int i=0; i < MAX_CLIENT_INFO_PIECES; i++){
            pieces[i] = "";
        }
    } 

    String eventName = String(event);

    // what part number do we have?
    String partNumberString = eventName.substring(eventName.lastIndexOf("/")+1,eventName.length());
    int partNumberInt = partNumberString.toInt(); // if this fails, the value is 0 - grrr

    // store the part in our static variables
    if (partNumberInt < MAX_CLIENT_INFO_PIECES) {
        pieces[partNumberInt] = String(data);
    } else {
        // we must have gotten more than MAX_CLIENT_INFO_PIECES pieces and we don't handle that
        // so set the timer so that when a new part comes in we'll clear all this out
        receiveFirstMS = 0;
    }

    // if we have a piece 0, then concatenate and pass off to see if we have a full JSON
    if (pieces[0].length() > 10) {
        // all responses are at least 10 characters long
        g_cibcidResponseBuffer = ""; // if some parts are not here, the parse test will fail
        for (int i=0; i < MAX_CLIENT_INFO_PIECES; i++) {
            g_cibcidResponseBuffer = g_cibcidResponseBuffer + pieces[i];
        }
    }
    
    clientInfoFromJSON(g_cibcidResponseBuffer);
}



// ----------------- GET PACKAGES BY CLIENT ID ---------------

// clearClientPackages
//   called to reset g_clientPackages to initial state
void clearClientPackages() {
    g_clientPackages.isValid = false;
    g_clientPackages.packagesString = "";
}

// ezfGetPackagesByClientID
//    Creates a JSON package for clientID and triggers the webhook
//    ezfGetPackagesByClientID
// 
//    After calling, wait for response to come in via particleCallbackEZF()
//
int ezfGetPackagesByClientID (int clientID) {

    g_clientPackagesResponseBuffer = "";
    g_packages = "";
    clearClientPackages();

    // Create parameters in JSON to send to the webhook
    const int capacity = JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> docJSON;
    
    docJSON["access_token"] = g_authTokenCheckIn.token.c_str();
    docJSON["clientID"] = clientID;
    
    char output[1000];
    serializeJson(docJSON, output);
    
    int rtnCode = Particle.publish("ezfGetPackagesByClientID",output, PRIVATE );
    
    return rtnCode;
}

/*****************************************
 * ezfReceivePackagesByClientID
 * 
 * Called from particleCallbackEZF() when it gets a response from
 * the get packages webhook. 
 * 
 * This function accumulates the response packages until it finds the
 * string EndOfPackages. Then it puts the response string in g_clientPackages.packagesString 
 * The response string is a concatenated list of all the package ReservationTypes
 * returned from the CRM system. This list contains all the machines the client
 * has permission to use.
*/

void ezfReceivePackagesByClientID (const char *event, const char *data)  {
    
    static int partCnt = 0;
    if (g_clientPackagesResponseBuffer.length() == 0) {
        partCnt = 0;
    }
    partCnt++;

    g_clientPackagesResponseBuffer = g_clientPackagesResponseBuffer + String(data);
    debugEvent ("PackagesPart " + String(partCnt) + ": " + String(data));

    if (g_clientPackagesResponseBuffer.indexOf("EndOfPackages") > 0 ) {
        g_clientPackages.packagesString = g_clientPackagesResponseBuffer;
        g_clientPackages.isValid = true;
        g_clientPackagesResponseBuffer = "";
    }
        
}

// ---------- mnlogdbCheckInOut ---------------
//
// call when a client is allowed in. This will call the mnlogdbCheckInOut webhook.
//
// params 
//    clientID that is going to be checking in or out
//    firstName  that will be displayed on the "checked in" web page
//
// Wait for response to particleCallbackMNLOGDB()
//
void mnlogdbCheckInOut(int clientID, String firstName, String lastName) {

    g_checkInOutResponseValid = false;
    g_checkInOutResponseBuffer = "";
    logCheckInOut("checkin allowed","",clientID,firstName,lastName);

}

// -------------- mnlogdbCheckInResponse -----------
//
// Called by particleCallbackMNLOGDB()
//
// Handles response from call to checkinout webhook
//
// sets global variable to say if person was checked in or checked out
// for use by loopCheckin
//
void mnlogdbCheckInOutResponse (const char *event, const char *data)  {

    g_checkInOutResponseBuffer = g_checkInOutResponseBuffer + data;

    // get content of <ActionTaken> tags
    int tagStartPos = g_checkInOutResponseBuffer.indexOf("<ActionTaken>");
    int valueEndPos = g_checkInOutResponseBuffer.indexOf("</ActionTaken>");
    if ( (tagStartPos < 0) || (valueEndPos <0) ){
        // Didn't find the tags from the php script, must not have it all yet

    } else {

        String actionTaken = g_checkInOutResponseBuffer.substring(tagStartPos + 13, valueEndPos );
        //xxx debugEvent("action tag value:" + actionTaken);

        // store results and return
        g_checkInOutActionTaken = actionTaken;
        g_checkInOutResponseValid = true;

    }
}

// ----------------- clearStationConfig
//
//
//
void clearStationConfig() {

    g_stationConfig.isValid = false;
    g_stationConfig.deviceType = 0;
    g_stationConfig.deviceName = "";
    g_stationConfig.LCDName = "";
    g_stationConfig.photoDisplay = "";
    g_stationConfig.logEvent = "";
    g_stationConfig.OKKeywords = "";

}

// --------------------- fdbGetStationConfig ------------
//
//
//
void fdbGetStationConfig() {

    clearStationConfig();
    String output = String(EEPROMdata.deviceType);
    Particle.publish("fdbGetStationConfig", output, PRIVATE);

}

// ----------------- fdbReceiveStationConfig ----------
//
//  fdbReceiveStationConfig
//
void fdbReceiveStationConfig(const char *event, const char *data) {

    const int capacity = JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> docJSON;
   
    char temp[3000]; //This has to be long enough for an entire JSON response
    strcpy_safe(temp, data);

    // will it parse?
    DeserializationError err = deserializeJson(docJSON, temp );
    JSONParseError =  err.c_str();
    if (!err) {
        JsonObject root_0 = docJSON[0];
        //We have valid full JSON response 
        if (root_0["deviceType"].as<int>() == EEPROMdata.deviceType) {

            int devType = root_0["deviceType"].as<int>();
            String deviceName =  root_0["deviceName"].as<char*>();
            String LCDName = root_0["LCDName"].as<char*>();
            String logEvent = root_0["logEvent"].as<char*>();
            String photoDisplay = root_0["photoDisplay"].as<char*>();
            String OKKeywords = root_0["OKKeywords"].as<char*>();

            setStationConfig(
                devType,
                deviceName,
                LCDName,
                logEvent,
                photoDisplay,
                OKKeywords
            );

        } else {
            debugEvent("Station Config deviceType does not match. Expected: " 
                    + String((int) EEPROMdata.deviceType) 
                    + " received: " 
                    + String(root_0["deviceType"].as<int>()) );
            writeToLCD("Station Config", "data is bad");
            // the admin loop will be waiting for the isValid member to be true.
        }
   
    } else {
        debugEvent ("Staion Config JSON parse error. " + String(temp));
        // the admin loop will be waiting for the isValid member to be true.
    }

}



// ----------------- ezfCheckInClient -------------------
// 
// Constructs a JSON for clientID and calls particle webhook ezfCheckInClient
//
// After calling wait for callback to particleCallbackEZF()
//
int ezfCheckInClient(String clientID) {
    
     // Create parameters in JSON to send to the webhook
    const int capacity = JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> docJSON;
    
    docJSON["access_token"] = g_authTokenCheckIn.token.c_str();
    docJSON["clientID"] = clientID.c_str();
    
    char output[1000];
    serializeJson(docJSON, output);
    
    int rtnCode = Particle.publish("ezfCheckInClient",output, PRIVATE );
    
    return rtnCode;
}



// ------------------- CLOUD TESTING CALLS ----------------


// -------- cloudRFIDCardRead -------
// Called to simulate a card read. 
// Pass in: clientID,cardUID
int cloudRFIDCardRead (String data) {
    
    int commaLocation = data.indexOf(",");
    if (commaLocation == -1) {
        // bad input data
        debugEvent("card read: no comma found ");
        return 1;
    }
    
    String clientID = data.substring(0,commaLocation);
    String cardUID = data.substring(commaLocation+1, data.length());
    debugEvent ("card read. client/UID: " + clientID + "/" + cardUID);
    
    int clientIDInt = clientID.toInt();

    if (clientIDInt == 0) {
        // bad data
        debugEvent ("card read: clientID is 0 or not int");
        return 1;
    }
    
    if (cardUID.length() < 1) {
        // bad data
        debugEvent ("card read: cardUID length is 0");
        return 1;
    }
    
    // setting this global variable will cause the main loop to begin the checkin process.
    // these  will be set to "" when the checkin process is done or gives up.
    g_cardData.clientID = clientIDInt;
    g_cardData.UID = cardUID;
    debugEvent("card read success: " + String(g_cardData.clientID) + "/" + g_cardData.UID );
    
    return 0;
}

// ------------ isCardUIDValid -------------------
// This routine tests if the card UID matches the value from the CRM
// these are stored in g_cardData and g_clientInfo
// 
// Retuns "" if card UID is valid and a 16 or less character message
//     if not. Message suitable for display to user.
String isCardUIDValid() {

    if ( ( g_cardData.UID.length() == 0) && (g_clientInfo.RFIDCardKey.length() == 0 ) ) {
        // card and EZF both have null card UIDs so skip the UID Tests below 
        // we assume the card is ok 

    } else { 

        debugEvent("cardUID:" + g_cardData.UID);
        debugEvent("clientRFID:" + g_clientInfo.RFIDCardKey);

        if  ( g_cardData.UID.length() != g_clientInfo.RFIDCardKey.length() ){
            // Different UID length is an issue. 
            return "Card revoked 1";
        }

        if  ( g_clientInfo.RFIDCardKey.indexOf(g_cardData.UID) == -1 ) {
            // The UIDs don't match 
            return "Card revoked 2";
        }
    }
    return "";
}

// ------------ isClientOkToCheckIn --------------
//  This routine is where we check to see if we should allow
//  the client to check in or if we should deny them. This
//  routine will use g_clientInfo and g_cardData to make the determination.
//    
//  Returns "" if client is good and a 16 or less character message
//      if not. Message suitable for display to user.
//
String isClientOkToCheckIn (){

    // Test for a good account status

    if ( (g_clientInfo.contractStatus.indexOf("Active") == 0) && (g_clientInfo.amountDue < 151) ) {
        // active contract, less than one month's money due 
        return "";
    }

    // We are going to reject, let's figure out why

    String allowInMessage = "Unknown reject"; 

    if (g_clientInfo.contractStatus.length() == 0) {

        allowInMessage = "No Contract"; // how did they get a badge if they never had a contract?

    } else if (g_clientInfo.amountDue > 0) {  

        allowInMessage = "Billing Issue"; // they owe more than $151
    
    } else {

        allowInMessage = "C: " + g_clientInfo.contractStatus; // Frozen, suspended, etc 

    }

    return allowInMessage;

}

// ------------ isClientOkForEquip --------------
//  This routine is where we check to see if we should allow
//  the client to use the equipment or if we should deny them. This
//  routine will use g_clientPackages and g_stationConfig.OKKeywords
//  to make the determination.
//    
//  Returns "" if client is good and a 16 or less character message
//      if not. Message suitable for display to user.
//
String isClientOkForEquip (){

    String keywords[10];
    int numKeywords = 0;

    if (g_stationConfig.OKKeywords.length() == 0) {
        //no keywords 
    } else {
        //split OKKeywords and check each against the packagesString
        int currentComma = -1;
        int nextComma = 0;
        do {
            nextComma = g_stationConfig.OKKeywords.indexOf(",",currentComma+1);
            if (nextComma == -1) {
                //last keyword 
                nextComma =  g_stationConfig.OKKeywords.length();
            } 
            //another keyword
            keywords[numKeywords] = g_stationConfig.OKKeywords.substring(currentComma+1,nextComma).trim();
            debugEvent("found keyword:" + keywords[numKeywords]); //xxx
            numKeywords++;
            currentComma = nextComma;
        } while ( nextComma < (int) g_stationConfig.OKKeywords.length() );
    }

    // test client packages for the presence of any of the OK Keywords
    bool keywordFound = false;
    if (numKeywords == 0) {
        keywordFound = true;
    } else {
        for (int i=0; i<numKeywords; i++) {
            if (g_clientPackages.packagesString.indexOf(keywords[i]) > -1 ) {
                keywordFound = true;
            }
        }
    }

    // return result
    if (keywordFound) {
        return "";
    } else {
        return "Package Not Found";
    }

}


/*************** FUNCTIONS FOR USE IN REAL CARD READ APPLICATIONS *******************************/




// ------------ Functions for the Admin app 
//
//  These functions are called by the Admin Android app
//

// cloudQueryMember - cloud function
//    memberNumber string to be lookup up in the CRM
//    results will be in cloud variable g_queryMemberResult
// returns:
//  0 = query accepted
//	1 = query is underway
//	2 = query is done and results are in the cloud variable (including any error report)
//	3 = query was already underway, this query is rejected
//	4 = memberNumber was not a number or was 0
//	5 = other error, see LCD panel on device
int cloudQueryMember(String data ) {

    // xxx after a timeout here the lcd goes to idle message. If you  call this again  with
    // the same number then it thinks the previous data is the result and returns 2, correctly, 
    // but  the display remains in idle  message 

    int queryMemberNum = data.toInt();
    if (queryMemberNum == 0){
        // error, data is not a number or it really is 0
        return 4;
    }

    if ((g_clientInfo.isValid) && (g_clientInfo.memberNumber.startsWith(data))) {
        // we have a result for this query data 
        return 2;
    }

    switch (g_adminCommand) {
    case acIDLE:
        // at this point we have a member number in data and we are acIDLE
        // clear the result and issue the command, the admin loop
        // will see this and take action 
        if (g_adminCommand == acIDLE) {
            g_queryMemberResult = "";
            g_adminCommandData = data;
            g_adminCommand = acGETUSERINFO;
            return 0;
        }
        break;
    
    default:
        // not acIDLE
        if (g_adminCommandData.startsWith(data)) {
            // asking for status on the query in progress
            return 1;
        } else {
            // must be trying to start a new query, and we are busy
            return 3;

        }
        break;
    }

    return 5; // should never get here.
};

// ----------------------- cloudIdentifyCard --------------------
//
// The client calls this to start the identify card process then
// continues to call as long as status 1 is returned.
// Status 2 indicates that data is now ready in the cloud variable cardInfo 
// Status 3 indicates an error 
//
int cloudIdentifyCard (String data) {
    
    static bool haveReturnedADoneStatus = false;

    if (g_adminCommand == acIDLE) {

        if (!haveReturnedADoneStatus) {
            
            if (g_clientInfo.isValid)  {   
                // We have good data and we have not yet alerted the client
                haveReturnedADoneStatus = true;
                return 2;

            } else if (g_identifyCardResultIsValid) {

                haveReturnedADoneStatus = true;
                return 2;

            } else {
                haveReturnedADoneStatus = true;
                return 1;
            
            }

        } else {
            // we are going to start a new identify card process 
            clearClientInfo();
            clearCardData();
            g_identifyCardResult = "";
            g_identifyCardResultIsValid = false;
            haveReturnedADoneStatus = false;
            g_adminCommandData = data;
            g_adminCommand = acIDENTIFYCARD; // admin loop will see this and change state
            return 1;
        }

    } else if (g_adminCommand == acIDENTIFYCARD){
        haveReturnedADoneStatus = false;
        return 1; // still working on it
    } else if  (g_clientInfo.isValid) {  //(g_identifyCardResultIsValid) {
        // it is possible to get here, but timing makes it unlikely. Once we have
        // good data we go quickly to acIDLE
        haveReturnedADoneStatus = true;
        return 2; // got the data
    } else {
        haveReturnedADoneStatus = true;
        return 3; // uknown error
    }
};

// ----------------------- cloudResetCardToFresh ------------------------
// cloudResetCardToFresh - cloud function
//     data is ignored
// returns:
//  	0 = ready, follow instructions on LCD
//	    1 = error contacting the hardwarebad card
//      2 = device busy with other operation
int cloudResetCardToFresh(String data) {
    if (g_adminCommand != acIDLE) {
        return 2;
    }
    g_adminCommand = acRESETCARDTOFRESH;
    return 0;

}

// ----------------------- cloudBurnCard ------------------------
// cloudBurnCard - cloud function
//     data is the string representation of the clientID for this card
//         it must match the value in g_clientInfo
// returns:
//  	0 = card burned successfully
//	    1 = error contacting the hardwarebad card; burn unsuccessful
//	    2 = clientID does not match clientID from last queryMember call
//      3 = data is not a number or is 0
int cloudBurnCard(String data){

    int clientID = data.toInt();
    if (clientID == 0) {
        // bad client ID 
        return 2;
    }

    if (g_clientInfo.clientID != clientID) {
        // this is not the clientID data we have
        return 1;
    }

    g_adminCommand = acBURNCARDNOW; // Admin loop will pick this up and burn the card
    return 0;

}

// ---------------------- LOOP FOR equipment station ---------------------
// ----------------------                  ----------------------
// This function is called from main loop when configured for any device except admin or checkin 
//
void loopEquipStation() {    
    //WoodshopLoopStates
    enum wslState {
        wslINIT,
        wslWAITFORCARD, 
        wslREQUESTTOKEN,
        wslWAITFORTOKEN, 
        wslASKFORCLIENTINFO, 
        wslWAITFORCLIENTINFO,
        wslWAITFORCLIENTPACKAGES, 
        wslCHECKEQUIPPERMISSION, 
        wslSHOWEQUIPRESULT,
        wslERROR
    };
    
    static wslState wsloopState = wslINIT;
    static unsigned long processStartMilliseconds = 0; 
    
    switch (wsloopState) {
    case wslINIT:
        writeToLCD(g_stationConfig.LCDName + " Ready"," ");
        wsloopState = wslWAITFORCARD;
        break;
    case wslWAITFORCARD: {
        digitalWrite(READY_LED,HIGH);
        writeToLCD("Badge in for",g_stationConfig.LCDName);
        enumRetStatus retStatus = readTheCard();
        if (retStatus == COMPLETE_OK) {
            // move to the next step
            wsloopState = wslREQUESTTOKEN;
            digitalWrite(READY_LED,LOW);
            digitalWrite(ADMIT_LED,HIGH);
            delay(200);
            digitalWrite(ADMIT_LED,LOW);
            }
        break;
    }
    case wslREQUESTTOKEN: 
        // request a good token from ezf
        ezfGetCheckInTokenCloud("junk");
        processStartMilliseconds = millis();
        wsloopState = wslWAITFORTOKEN;
        break;

    case wslWAITFORTOKEN:
        // waiting for a good token from ezf
        if (g_authTokenCheckIn.token.length() != 0) {
            // we have a token
            wsloopState = wslASKFORCLIENTINFO;
        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("took too long to get token, checkin aborts");
                processStartMilliseconds = 0;
                writeToLCD("Timeout token", "Try Again");
                buzzerBadBeep();
                delay(2000);

                // log this to DB 
                logToDB("timeout waiting for token","",g_clientInfo.clientID,"","");
                wsloopState = wslWAITFORCARD;
            }
        //Otherwise we stay in this state
        }
        break;

    case wslASKFORCLIENTINFO:  {

        // ask for client details
        ezfClientByClientID(g_cardData.clientID);
        processStartMilliseconds = millis();
        wsloopState = wslWAITFORCLIENTINFO;
        break;
        
    }
    case wslWAITFORCLIENTINFO: {

        // got client data, 
        String msg1, msg2, logmsg;
        msg1="";
        msg2="";
        logmsg="";

        if ( g_clientInfo.isValid ) {

            if (g_clientInfo.contractStatus.indexOf("Active") == -1) {

                // membership not active, deny checkin
                msg1 = "Membership";
                msg2 = "not Active";
                logmsg = "Membership not active";

            }

            logmsg = isCardUIDValid();
            if (logmsg.length() > 0) {

                //card revoked, deny checkin
                msg1 = "RFID Card";
                msg2 = "validation fail";
                logmsg = isCardUIDValid();

            } 
         
            if (msg1.length() > 0) {

                //deny checkin
                writeToLCD(msg1, msg2);
                buzzerBadBeep();
                delay(2000);

                //log this to DB
                logToDB(logmsg,"",g_clientInfo.clientID,"","");
                wsloopState = wslWAITFORCARD; 

            } else {
                    //membership is ok, let's move on
                    processStartMilliseconds = millis();
                    ezfGetPackagesByClientID(g_cardData.clientID);
                    wsloopState = wslWAITFORCLIENTPACKAGES;
                }

        } else {
            // client info is not valid yet
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("15 second timer exeeded, clientinfo aborts");
                processStartMilliseconds = 0;
                writeToLCD("Timeout clientInfo", "Try Again");
                buzzerBadBeep();
                digitalWrite(REJECT_LED,HIGH);
                delay(2000);
                digitalWrite(REJECT_LED,LOW);

                // log this to DB 
                logToDB("timeout waiting for client info","",g_clientInfo.clientID,"","");

                wsloopState = wslWAITFORCARD;
            }
        } // Otherwise we stay in this state
        break;
    }

    case wslWAITFORCLIENTPACKAGES: 
        if (g_clientPackages.isValid ){

            wsloopState = wslCHECKEQUIPPERMISSION;

        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 30000) {
                debugEvent("30 second timer exeeded for packages, aborts");
                processStartMilliseconds = 0;
                writeToLCD("Timeout packages", "Try Again");
                buzzerBadBeep();
                digitalWrite(REJECT_LED,HIGH);
                delay(2000);
                digitalWrite(REJECT_LED,LOW);

                // log this to DB 
                logToDB("timeout waiting for client packages","",g_clientInfo.clientID,"","");

                wsloopState = wslWAITFORCARD;
            }
        } // Otherwise we stay in this state

        break;

    case wslCHECKEQUIPPERMISSION: {

        String allowInMessage = isClientOkForEquip();
        debugEvent("allowinmsg:" + allowInMessage);

        if ( allowInMessage.length() > 0 ) {
            //client account is not good for woodshop
            writeToLCD(allowInMessage,"See Manager");
            buzzerBadBeep();
            digitalWrite(REJECT_LED,HIGH);
            delay(2000);
            digitalWrite(REJECT_LED,LOW);
            
            // log this to DB 
            logToDB(g_stationConfig.LCDName + " denied", allowInMessage, g_clientInfo.clientID, "","");

            wsloopState = wslWAITFORCARD;
            
        } else {
        
            debugEvent (g_stationConfig.LCDName + " checkin");
            
            // log this to our DB 
            processStartMilliseconds = millis();
            logToDB(g_stationConfig.logEvent, "", g_clientInfo.clientID, g_clientInfo.firstName, g_clientInfo.lastName );
            
            wsloopState = wslSHOWEQUIPRESULT;

        }
        break;
    } 

    case wslSHOWEQUIPRESULT:{

        if (millis() - processStartMilliseconds > 5000) {
            // took too long to get answer from our logging database
            String fullName = g_clientInfo.firstName + " " + g_clientInfo.lastName;
            writeToLCD("Thanks for a tap", g_stationConfig.LCDName + " Ok" );
            digitalWrite(ADMIT_LED,HIGH);
            buzzerGoodBeepTwice();
            delay(1000);
            digitalWrite(ADMIT_LED,LOW);
            debugEvent("Timeout writing to log DB - woodshop result");
            wsloopState = wslWAITFORCARD;

        } else {

            //publish the checkin for other devices that may be listening for it
            String msg = "{\"deviceType\":" + String(EEPROMdata.deviceType) + 
                ",\"secret\":" + String(checkinEventSecret) + "}";   // XXX should use arduinoJSON here
            Particle.publish("checkin",msg,PRIVATE);

            writeToLCD(g_stationConfig.LCDName + " allowed", "Be Safe");
            digitalWrite(ADMIT_LED,HIGH);
            buzzerGoodBeepTwice();
            delay(2000);
            digitalWrite(ADMIT_LED,LOW);
            wsloopState = wslWAITFORCARD;
            break; // xxx

            // xxx will we do checkin/out of woodshop?
            // we have a response from our logging database
            //when we hear back from the logging database we know what to display
           
            if (g_checkInOutActionTaken.indexOf("Checked In") == 0) { 
                String fullName = g_clientInfo.firstName + " " + g_clientInfo.lastName;
                writeToLCD("Welcome","to " + g_stationConfig.LCDName);
                digitalWrite(ADMIT_LED,HIGH);
                buzzerGoodBeepTwice();
                delay(1000);
                digitalWrite(ADMIT_LED,LOW);
            } else {
                String fullName = g_clientInfo.firstName + " " + g_clientInfo.lastName;
                writeToLCD("Goodbye",fullName.substring(0,15));
                digitalWrite(ADMIT_LED,HIGH);
                buzzerGoodBeeps3UpDownUp();
                delay(1000);
                digitalWrite(ADMIT_LED,LOW);
            }
            
            wsloopState = wslWAITFORCARD;

        } 
        // just stay in this state
        break;
    }
    case wslERROR:
        break;

    default:
        break;
    } //switch (mainloopState)
    


}



// ---------------------- LOOP FOR CHECKIN ---------------------
// ----------------------                  ----------------------
// This function is called from main loop when configured for Check In station. 
void loopCheckIn() {    
    //CheckInLoopStates
    enum cilState {
        cilINIT,
        cilWAITFORCARD, // we spend most time in this state 
        cilREQUESTTOKEN,
        cilWAITFORTOKEN, 
        cilASKFORCLIENTINFO, 
        cilWAITFORCLIENTINFO, 
        cilCHECKINGIN, 
        cilSHOWINOROUT,  // persist the message on the display 
        cilERROR
    };
    
    static cilState cilloopState = cilINIT;
    static unsigned long processStartMilliseconds = 0; 
    
    switch (cilloopState) {
    case cilINIT:
        writeToLCD("Checkin Ready"," ");
        cilloopState = cilWAITFORCARD;
        break;
    case cilWAITFORCARD: {
        digitalWrite(READY_LED,HIGH);
        writeToLCD("Place card on","spot to checkin");
        enumRetStatus retStatus = readTheCard();
        if (retStatus == COMPLETE_OK) {
            // card was read and data obtained, move to the next step
            writeToLCD("checking...", " ");
            cilloopState = cilREQUESTTOKEN;
            digitalWrite(READY_LED,LOW);
            }
        // otherwise just stay in this state wating for a good card presentation and read
        break;
    }
    case cilREQUESTTOKEN: 
        // request a good token from ezf, might already have a valid token 
        // and this will return immediately 
        ezfGetCheckInTokenCloud("junk"); // xxx not a cloud function
        processStartMilliseconds = millis();
        cilloopState = cilWAITFORTOKEN;
        break;

    case cilWAITFORTOKEN:
        // waiting for a good token from ezf
        if (g_authTokenCheckIn.token.length() != 0) {
            // we have a token
            cilloopState = cilASKFORCLIENTINFO;
        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("took too long to get token, checkin aborts");
                processStartMilliseconds = 0;
                writeToLCD("Timeout token", "Try Again");
                buzzerBadBeep();
                delay(2000);

                // log this to DB 
                logToDB("timeout waiting for token","",g_clientInfo.clientID,"","");
                cilloopState = cilWAITFORCARD;
            }
        //Otherwise we stay in this state
        }
        break;

    case cilASKFORCLIENTINFO:  {

        // ask for client details
        ezfClientByClientID(g_cardData.clientID);
        processStartMilliseconds = millis();
        cilloopState = cilWAITFORCLIENTINFO;
        break;
        
    }
    case cilWAITFORCLIENTINFO: {
  
        if ( g_clientInfo.isValid ) {
            // got client data, let's move on
            cilloopState = cilCHECKINGIN;
        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("15 second timer exeeded, checkin aborts");
                processStartMilliseconds = 0;
                writeToLCD("Timeout clientInfo", "Try Again");
                buzzerBadBeep();
                digitalWrite(REJECT_LED,HIGH);
                delay(2000);
                digitalWrite(REJECT_LED,LOW);

                // log this to DB 
                logToDB("timeout waiting for client info","",g_clientInfo.clientID,"","");

                cilloopState = cilWAITFORCARD;
            }
        } // Otherwise we stay in this state
        break;
    }
    case cilCHECKINGIN: {

        // If the client meets all critera, check them in

        String cardValidMessage = isCardUIDValid(); // check card has not been revoked
        debugEvent("cardValidmsg:" + cardValidMessage);

        if (cardValidMessage.length() > 0) {
            //card UID does not match CRM. 
            writeToLCD(cardValidMessage,"See Manager");
            buzzerBadBeep();
            digitalWrite(REJECT_LED,HIGH);
            delay(2000);
            digitalWrite(REJECT_LED,LOW);

            // log this to DB 
            logToDB("checkin denied",cardValidMessage,g_clientInfo.clientID,g_clientInfo.firstName,g_clientInfo.lastName);

            cilloopState = cilWAITFORCARD;

        } else {
            // card UID is ok, what about account?
            String allowInMessage = isClientOkToCheckIn();
            debugEvent("allowinmsg:" + allowInMessage);

            if ( allowInMessage.length() > 0 ) {
                //client account status is bad
                writeToLCD(allowInMessage,"See Manager");
                buzzerBadBeep();
                digitalWrite(REJECT_LED,HIGH);
                delay(2000);
                digitalWrite(REJECT_LED,LOW);
                
                // log this to DB 
                logToDB("checkin denied",allowInMessage,g_clientInfo.clientID,g_clientInfo.firstName,g_clientInfo.lastName);

                cilloopState = cilWAITFORCARD;
                
            } else {
                // all looks good, checkin to ezfacility
                debugEvent ("SM: now checkin client");
                // tell EZF to check someone in 
                // xxx should we only do this on a checkin, not a checkout?
                ezfCheckInClient(String(g_clientInfo.clientID));
                
                // log this to our DB 
                processStartMilliseconds = millis();
                // call our DB and find out if this is checkin or checkout 
                mnlogdbCheckInOut(g_clientInfo.clientID,g_clientInfo.firstName,g_clientInfo.lastName );
                
                cilloopState = cilSHOWINOROUT;

            }
        }
        break;
    }
    case cilSHOWINOROUT:{

        if (millis() - processStartMilliseconds > 5000) {
            // took too long to get answer from our logging database
            String fullName = g_clientInfo.firstName + " " + g_clientInfo.lastName;
            writeToLCD("Thanks for a tap",fullName.substring(0,15) );
            digitalWrite(ADMIT_LED,HIGH);
            buzzerGoodBeepTwice();
            delay(1000);
            digitalWrite(ADMIT_LED,LOW);
            debugEvent("Timeout looking for action tag");
            cilloopState = cilWAITFORCARD;

        } else if (g_checkInOutResponseValid) {
            // we have a response from our logging database
            //when we hear back from the logging database we know what to display
            if (g_checkInOutActionTaken.indexOf("Checked In") == 0){ 
                String fullName = g_clientInfo.firstName + " " + g_clientInfo.lastName;
                writeToLCD("Welcome",fullName.substring(0,15));
                digitalWrite(ADMIT_LED,HIGH);
                buzzerGoodBeepTwice();
                delay(1000);
                digitalWrite(ADMIT_LED,LOW);
            } else {
                String fullName = g_clientInfo.firstName + " " + g_clientInfo.lastName;
                writeToLCD("Goodbye",fullName.substring(0,15));
                digitalWrite(ADMIT_LED,HIGH);
                buzzerGoodBeeps3UpDownUp();
                delay(1000);
                digitalWrite(ADMIT_LED,LOW);
            }
            
            cilloopState = cilWAITFORCARD;
        } else {
            // just stay in this state
        }
        break;
    }
    case cilERROR:
        break;

    default:
        break;
    } //switch (mainloopState)
    


}

// adminIdentifyCard
//
// called from admin loop when admin command is acIDENTIFYCARD
//
// will get data from RFID card and then retrieve member info from CRM
// and put is as JSON for retrival from particle cloud
//
// JSON has at least clientID, name but can have more fields.
// also has errorCode of:
//	0 = member found and membership data is in the JSON
//	2 = member not found in EZFacility
//	3 = more than one member found in EZfacility
//	4 = other error.
void adminIdentifyCard() {
enum idcState {
        idcINIT,
        idcWAITFORCARD,
        idcWAITFORTOKEN,
        idcWAITFORINFO,
        idcDISPLAYINGINFO,
        idcCLEANUP
    }
    static idcState = idcINIT;
    static unsigned long processStartMilliseconds = 0; 

    switch (idcState) {
    case idcINIT:
        //writeToLCD("Whose Card?", "Init");
        //delay(1000);
        processStartMilliseconds = millis();
        digitalWrite(READY_LED,HIGH);
        idcState = idcWAITFORCARD;
        break;

    case idcWAITFORCARD: 
        if(millis() - processStartMilliseconds > 15000){
            // timeout waiting for a card, so go idle state
            idcState = idcCLEANUP;
        } else  {
            writeToLCD("Whose Card?", " ");
            g_identifyCardResultIsValid = false;

            // test the card to determine its type
            uint8_t cardType = testCard();

            if (cardType == 255) {
                // no card presented. 
                // do nothing and remain in this state
            } else if (cardType == 1) {
                // MN formatted card, so continue process
                enumRetStatus retStatus = readTheCard();
                if (retStatus == COMPLETE_OK) {
                    writeToLCD("read card cID:",String(g_cardData.clientID));
                    // move to the next step

                    // request a good token from ezf
                    ezfGetCheckInTokenCloud("junk");
                    processStartMilliseconds = millis();

                    idcState = idcWAITFORTOKEN;
                    digitalWrite(READY_LED,LOW);

                } else {
                    writeToLCD("Card read fail",g_cardData.cardStatus);
                    g_identifyCardResult = clientInfoToJSON(1,"Card read failed",true);
                    g_identifyCardResultIsValid = true;
                    idcState = idcCLEANUP;
                } 

            } else {
                // got a card, but not a type we want
                // report card error
                reportCardError(cardType);
                g_identifyCardResult = clientInfoToJSON(1,"Card read failed",true);
                g_identifyCardResultIsValid = true;  //xxx how do we get the right answer into the JSON?
                idcState = idcCLEANUP;
            }
        }
        break;

    case idcWAITFORTOKEN:
        // waiting for a good token from ezf
        if (g_authTokenCheckIn.token.length() != 0) {
            // we have a token, ask for the client data
            processStartMilliseconds = millis();
            ezfClientByClientID(g_cardData.clientID);
            idcState = idcWAITFORINFO;
        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("took too long to get token, user info aborts");
                processStartMilliseconds = 0; // XXX do we need this ?
                writeToLCD("Timeout token", "Try Again");
                buzzerBadBeep();
                delay(2000);
                idcState = idcCLEANUP;
            }
        //Otherwise we stay in this state
        }
        break;

    case idcWAITFORINFO: 
  
        if ( g_clientInfo.isValid ) {
            // got client data, let's move on
            g_identifyCardResult = clientInfoToJSON(0,"OK", true);
            String fullName = g_clientInfo.firstName + " " + g_clientInfo.lastName;
            writeToLCD("Card is for",fullName.substring(0,15));
            buzzerGoodBeepOnce();
            processStartMilliseconds = millis();
            idcState = idcDISPLAYINGINFO;

        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("15 second timer exeeded, whois aborts");
                processStartMilliseconds = 0;
                writeToLCD("Timeout clientInfo", "Try Again");
                buzzerBadBeep();
                delay(2000);
                idcState = idcCLEANUP;
            }
        } // Otherwise we stay in this state
        break;

    case idcDISPLAYINGINFO: {
        if (millis() - processStartMilliseconds > 3000) {
            idcState = idcCLEANUP;
        }
        // if it hasn't been five seconds, stay in this state
        break;
    }

    case idcCLEANUP:
        writeToLCD("idcard done", " ");
        g_adminCommandData = "";
        g_adminCommand = acIDLE; 
        idcState = idcINIT;
        break;
    
    default:
        break;

    }

    return;
}


/*********************************************
 * 
 * adminGetUserInfo
 * Called from admin loop when admin command is acGETUSERINFO (cloudQueryMember)
 *
 * parameters
 *    if both parameters are set, only clientID will be used 
 *    to use memberNumber, pass in clientID = 0
 *
 * calls CRM for the user data and formats it as JSON
 * for the admin client to get via particle cloud.
 * JSON has at least clientID, name but can have more fields.
 * JSON also has errorCode of:
 *	0 = member found and membership data is in the JSON
 *	2 = member not found in EZFacility
 *	3 = more than one member found in EZfacility
 *	4 = other error.
*/
void adminGetUserInfo(int clientID, String memberNumber) {
    enum guiState {
        guiINIT,
        guiIDLE,
        guiWAITFORTOKEN,
        guiASKFORCLIENTINFO,
        guiWAITFORINFO,
        guiFORMATINFO,
        guiDISPLAYINGINFO,
        guiCLEANUP
    }
    static guiState = guiINIT;
    static unsigned long processStartMilliseconds = 0; 

    switch (guiState) {

    case guiINIT:

        guiState = guiIDLE;
        break;

    case guiIDLE: 
        writeToLCD("Signing in","to EZFacility");
        // request a good token from ezf
        ezfGetCheckInTokenCloud("junk");
        processStartMilliseconds = millis();
        guiState = guiWAITFORTOKEN;
        break;

    case guiWAITFORTOKEN:
        // waiting for a good token from ezf
        if (g_authTokenCheckIn.token.length() != 0) {
            // we have a token
            guiState = guiASKFORCLIENTINFO;
        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("took too long to get token, user info aborts");
                processStartMilliseconds = 0;
                writeToLCD("Timeout token", "Try Again");
                buzzerBadBeep();
                delay(2000);
                guiState = guiCLEANUP;
            }
        //Otherwise we stay in this state
        }
        break;

    case guiASKFORCLIENTINFO:

        processStartMilliseconds = millis();
        if (clientID != 0) {
            ezfClientByClientID(clientID);
        } else {
            ezfClientByMemberNumber(memberNumber);
        }
        guiState = guiWAITFORINFO;
        break;

    case guiWAITFORINFO:

        if ( g_clientInfo.isValid ) {
            // got client data, let's move on
            guiState = guiFORMATINFO;
            
        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("15 second timer exeeded, get client info error");
                processStartMilliseconds = 0;
                writeToLCD("Timeout clientInfo", "Try Again");
                buzzerBadBeep();
                delay(2000);
                guiState = guiCLEANUP;
            }
        } // Otherwise we stay in this state
        break;

    case guiFORMATINFO: {

        g_queryMemberResult = clientInfoToJSON(0, "OK", false);

        String fullName = g_clientInfo.firstName + " " + g_clientInfo.lastName;
        writeToLCD("Selected:",fullName.substring(0,16));

        processStartMilliseconds = millis();
        guiState = guiDISPLAYINGINFO;

        // we got a response
        break;
    }

    case guiDISPLAYINGINFO:
        if ((millis() - processStartMilliseconds > 15000) || !g_clientInfo.isValid) {
            // either the burn completed and cleared clientInfo or we've waited 
            // too long for a burn
            guiState = guiCLEANUP;
        }
        //stay in this state
        break;
    
    case guiCLEANUP:
        writeToLCD(" ", " ");
        g_adminCommandData = "";
        g_adminCommand = acIDLE;
        guiState = guiIDLE; // xxx
        break;

    default:
        writeToLCD("Admin err", "unknown state");
        break;
    }

}

// ---------------- loopUndefinedDevice ------
// This is called over and over from the main loop when the
// device is configured as type 0, undefined. This is most 
// likely when the device is first set up or is undergoing some 
// hardware debug.
void loopUndefinedDevice() {

    digitalWrite(READY_LED,HIGH);
    digitalWrite(ADMIT_LED,HIGH);
    digitalWrite(REJECT_LED,HIGH);

}

// --------------- loopAdmin -------------
// This is called over and over from the main loop if the 
// device is configured to be an admin device.
//
void loopAdmin() {

    // the loop state is in g_adminCommand and not here
    // because the state is set by calls from the admin app on Android device

    static bool init = true; // xxx make an init state
    static bool LCDSaysIdle = false;

    if (init) {  // xxx change to an init state
        init = false;
        writeToLCD("Admin Idle", " ");
    }

    switch (g_adminCommand) {

    case acIDLE:
        if (!LCDSaysIdle){
            LCDSaysIdle = true;
            writeToLCD("Use Android", "app");
        }
        break;

    case acGETUSERINFO:
        LCDSaysIdle = false;
        adminGetUserInfo(0, g_adminCommandData); 
        break;

    case acIDENTIFYCARD:
        LCDSaysIdle = false;
        adminIdentifyCard();
        break;

    case acBURNCARDNOW: {
        LCDSaysIdle = false;
        burnCardNow(g_clientInfo.clientID, g_clientInfo.RFIDCardKey);
        break;

    case acRESETCARDTOFRESH: {
        LCDSaysIdle = false;
        resetCardToFreshNow();
    }
    }

    default:
        break;
    }
   


}

// ---------- XXX cloud funtion to help troubleshoot the LCD scramble error 
int cloudInitLCD (String data){
    lcd.begin(16,2);
    return 0;
}


// --------------------- SETUP -------------------
// This routine runs only once upon reset
void setup() {

    Particle.variable ("ClientID", g_clientInfo.clientID);
    Particle.variable ("ContractStatus",g_clientInfo.contractStatus);
    Particle.variable ("recentErrors",g_recentErrors);
    Particle.variable ("JSONParseError", JSONParseError);
      
    int success = 0; 
    if (success) {}; // xxx to remove warning, but should we check this below?

    // The following are useful for debugging
    //Particle.variable ("RFIDCardKey", g_clientInfo.RFIDCardKey);
    //Particle.variable ("g_Packages",g_packages);
    //success = Particle.function("GetCheckInToken", ezfGetCheckInTokenCloud);
    // xxx
    //Particle.variable ("debug2", debug2);
    //Particle.variable ("debug3", debug3);
    //Particle.variable ("debug4", debug4);
    success = Particle.function("ClientByMemberNumber", ezfClientByMemberNumber);
    success = Particle.function("ClientByClientID", ezfClientByClientIDCloud);

    success = Particle.function("InitLCD", cloudInitLCD);
 
    // Used by all device types
    success = Particle.function("SetDeviceType", cloudSetDeviceType);
    // xxx limit of 4 handlers??? Particle.subscribe(System.deviceID() + "RFIDLoggingReturn", RFIDLoggingReturn, MY_DEVICES);

    // Used to test CheckIn
    success = Particle.function("RFIDCardRead", cloudRFIDCardRead);

    // Needed for all devices
    Particle.subscribe(System.deviceID() + "ezf",particleCallbackEZF, MY_DEVICES);
    Particle.subscribe(System.deviceID() + "mnlogdb",particleCallbackMNLOGDB, MY_DEVICES); // older
    Particle.subscribe(System.deviceID() + "fdb",particleCallbackMNLOGDB, MY_DEVICES); // newer


    // Used by device types for machine usage permission validation
    //success = Particle.function("PackagesByClientID",ezfGetPackagesByClientID);
    //Particle.subscribe(System.deviceID() + "ezfGetPackagesByClientID",ezfReceivePackagesByClientID, MY_DEVICES);

    // Used by Admin Device
    Particle.function("queryMember",cloudQueryMember);
    Particle.variable("queryMemberResult",g_queryMemberResult);
    Particle.function("burnCard",cloudBurnCard);
    Particle.function("resetCard",cloudResetCardToFresh);
    Particle.function("identifyCard",cloudIdentifyCard);
    Particle.variable("identifyCardResult",g_identifyCardResult); // xxx should be queryCardInfoResult


    System.on(firmware_update, firmwareupdatehandler);

    // Gawd, dealing with dst!
    Time.zone(-8); //PST We are not dealing with DST in this device
    bool yesDST = false;
    if ( (Time.month() > 3) && (Time.month() < 11) ) {
        yesDST = true;
    }
    if ( (Time.month() == 3) && (Time.day() > 10 ) ) {
        yesDST = true;
    }
    if ( (Time.month() == 3) && (Time.day() == 10) && (Time.hour() > 2) ){
        yesDST = true;
    }
    if ( (Time.month() == 11) && (Time.day() < 3 ) ) {
        yesDST = true;
    }
    if ( (Time.month() == 11) && (Time.day() == 3) && (Time.hour() < 2) ){
        yesDST = true;
    }
    if (yesDST) {
        Time.beginDST();
        debugEvent("DST is set"); // xxx
    } 

    // read EEPROM data for device type 
    EEPROMRead();

    // Initialize D0 + D7 pin as output
    // It's important you do this here, inside the setup() function rather than outside it 
    // or in the loop function.
    pinMode(led, OUTPUT);
    pinMode(led2, OUTPUT);
    
    pinMode(READY_LED, OUTPUT);
    pinMode(ADMIT_LED, OUTPUT);
    pinMode(REJECT_LED, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(ONBOARD_LED_PIN, OUTPUT);   // the D7 LED
    
    digitalWrite(READY_LED, LOW);
    digitalWrite(ADMIT_LED, LOW);
    digitalWrite(REJECT_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(ONBOARD_LED_PIN,LOW);
 
#ifdef LCD_PRESENT
    lcd.begin(16,2);
#endif
    writeToLCD("MN Checkin","StartUp");

#ifdef TEST
    delay(5000);    // delay to get putty up
    Serial.println("trying to connect ....");
#endif

#ifdef RFID_READER_PRESENT 

    mnrfidInit();
    
#endif
    
    writeToLCD("Initializing","Particle Cloud");

   
    logToDB("restart",String(MN_FIRMWARE_VERSION),0,"","" );

    //Show all lights
    writeToLCD("Init all LEDs","should blink");
    digitalWrite(READY_LED,HIGH);
    digitalWrite(ADMIT_LED,HIGH);
    digitalWrite(REJECT_LED,HIGH);
    writeToLCD("XXXXXXXXXXXXXXXX","XXXXXXXXXXXXXXXX");
    delay(1000);
    writeToLCD("","");
    digitalWrite(READY_LED,LOW);
    digitalWrite(ADMIT_LED,LOW);
    digitalWrite(REJECT_LED,LOW);

    g_secretKeysValid = false;

    //RFIDKeysJSON from include file
    responseRFIDKeys("junk", RFIDKeysJSON); //xxx remove parameter

    // Signal ready to go
    writeToLCD(g_stationConfig.LCDName,"ver. " + String(MN_FIRMWARE_VERSION));
    buzzerGoodBeepTwice();
    //flash the D7 LED twice
    for (int i = 0; i < 2; i++) {
        digitalWrite(ONBOARD_LED_PIN, HIGH);
        delay(500);
        digitalWrite(ONBOARD_LED_PIN, LOW);
        delay(500);
    } 


}

// This routine gets called repeatedly, like once every 5-15 milliseconds.
// Spark firmware interleaves background CPU activity associated with WiFi + Cloud activity with your code. 
// Make sure none of your code delays or blocks for too long (like more than 5 seconds), or weird things can happen.
void loop() {

    // Main Loop State
    enum mlsState {
        mlsASKFORSTATIONCONFIG,
        mlsWAITFORSTATIONCONFIG,
        mlsASKFORTOKEN,
        mlsDEVICELOOP, 
        mlsERROR
    };
    
    static mlsState mainloopState = mlsASKFORSTATIONCONFIG;
    static unsigned long processStart = 0;
    
    switch (mainloopState) {
    case mlsASKFORSTATIONCONFIG:
        if ((EEPROMdata.deviceType == DEVICETYPE_UNDEFINED) || (EEPROMdata.deviceType == DEVICETYPE_CHECKIN)) {
           
            // if type is undefined or checkin, then don't need config.
            if (EEPROMdata.deviceType == DEVICETYPE_UNDEFINED) {
                setStationConfig( DEVICETYPE_UNDEFINED, "Undefined","Undefined","Undefined","","");
            } else if (EEPROMdata.deviceType == DEVICETYPE_CHECKIN) {
                setStationConfig( DEVICETYPE_CHECKIN, "CheckIn", "Check In","Check In","","");
            } else {
                setStationConfig( 99999,"code error 1","code error 1","code error 1","","" );
            }

            mainloopState = mlsASKFORTOKEN;

        } else {
            
            // type is other than undefined or checkin, get config.
            // Get config info from the Facility database
            processStart = millis();
            fdbGetStationConfig();
            mainloopState = mlsWAITFORSTATIONCONFIG;

        }

        break;

    case mlsWAITFORSTATIONCONFIG:
        if (millis() - processStart > 20000 ) {

            // Failed to get station config
            writeToLCD("Get Station","Config failed");
            mainloopState = mlsERROR;

        } else if (g_stationConfig.isValid) {

            mainloopState = mlsASKFORTOKEN;
        }
        // otherwise stay in this state 

        break;
    
    case mlsASKFORTOKEN: 
        ezfGetCheckInToken();  // This is non blocking, call it to preemptively get a token 
        mainloopState = mlsDEVICELOOP; // initialization is over
        break;

    case mlsDEVICELOOP: {

        // Once the intial steps have occurred, the main loop will stay
        // in this state forever. It calls out to the correct device code and
        // the "loop" continues there
        switch (EEPROMdata.deviceType) {
        case DEVICETYPE_UNDEFINED:
            loopUndefinedDevice();
            break;
        case DEVICETYPE_CHECKIN:
            loopCheckIn();
            break;
        case DEVICETYPE_ADMIN:
            loopAdmin();
            break;
        default:
            // all other stations
            loopEquipStation();
            break;
        }
        break;
    }
    case mlsERROR: 
        mainloopState = mlsERROR; // No way out of this except a reboot
        break;

    default:
        writeToLCD("Unknown","mainLoopState");
        mainloopState = mlsERROR;
        break;
    }
    
    heartbeatLEDs(); // heartbead on D7 LED 

    debugEvent("");  // need this to pump the debug event process

    // for the hours of 8am to midnight, see if we need a new token
    // we do this test so that we always have a good token 
    // the ezfGetCheckInToken() will make sure we don't call the cloud too often
    if ((Time.hour() >= 8) && (Time.hour() <= 23)) {
        ezfGetCheckInToken();
    }

    // Reboot once a day.
    if ( (Time.hour() == 1) && (Time.minute() == 5)) {   
        // Since we only check the Minute, set the reboot to be at least one minute from now
        long rebootDelay = (int) random(70000,30000); // 70 seconds - 5 minutes
        rebootSet(rebootDelay); 
    }
    rebootSet(0); // check to see if a reboot should happen
}

