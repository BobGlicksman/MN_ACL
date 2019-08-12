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
 * 1.01: Added tests to readCard and beep buzzer if read is bad (ie, returns clientID of 0).
 *       Also changed LCD messages a bit to reflect this new testing.
 *       readCard no longer blocks waiting for a card presentation; instead it
 *          returns with IN_PROCESS
 *       Cleaned up my quick implementation when Bob and I were first testing.
 * 1.02: Realized that having a state machine in the checkin code was confusing and not
 *          the way to go. Changed to have the only state machine in the main loop and
 *          now the code is much cleaner and easier to read. The states are all explicit.
 * 
 * (c) 2019; Team Practical Projects
 * 
 * Authors: Bob Glicksman, Jim Schrempp
 * version 1.02; 8/4/2019.
 * 
 * 
************************************************************************/

//#define TEST     // uncomment for debugging mode
#define RFID_READER_PRESENT  // uncomment when an RFID reader is connected
#define LCD_PRESENT  // uncomment when an LCD display is connected

//Required to get ArduinoJson to compile
#define ARDUINOJSON_ENABLE_PROGMEM 0
#include <ArduinoJson.h>
//https://arduinojson.org/v6/assistant/

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_PN532.h>

// This #include statement was automatically added by the Particle IDE.
#include <LiquidCrystal.h>

// ----------- RFID Constants and Globals -------------------

#define IRQ_PIN D3
#define RST_PIN D4  // not connected
#define LED_PIN D7

#define READY_LED D4
#define ADMIT_LED A4
#define REJECT_LED A5
#define BUZZER_PIN D2

// Sector to use for testing
const int SECTOR = 3;	// Sector 0 is manufacturer data and sector 1 is real MN data

// Encryption keys for testing
uint8_t DEFAULT_KEY_A[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t DEFAULT_KEY_B[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t MN_SECRET_KEY_A[6] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5 };
uint8_t MN_SECRET_KEY_B[6] = { 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5 };

// Access control bits for a sector
    // the default is block 0, 1, and 2 of the sector == 0x00
	// the default for block 3 (trailer) of the sector is 0x01
	// the desired MN bits for block 0, 1, and 2 of the sector == 0x04
	// the desired MN bits for block 3 (trailer) of the sector is 0x03
	// byte 9 is user data.  We will make this byte 0x00
	// compute ACB bytes using http://calc.gmss.ru/Mifare1k/
uint8_t DEFAULT_ACB[4] = {0xFF, 0x07,0x80, 0x00};
uint8_t MN_SECURE_ACB[4] = {0x7C, 0x37,0x88, 0x00};

// Global variables to hold card info
uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};  // 7 byte buffer to store the returned UID
uint8_t uidLength;  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

// Global variable for messages
String message = "";

// Data for testing
uint8_t TEST_PAT_1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint8_t TEST_PAT_2[] = {255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240};


// instantiate in I2C mode   
Adafruit_PN532 nfc(IRQ_PIN, RST_PIN); 

// instatiate the LCD
LiquidCrystal lcd(A0, A1, A2, A3, D5, D6);



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
  

// Define the pins we're going to call pinMode on

int led = D0;  // You'll need to wire an LED to this one to see it blink.
int led2 = D7; // This one is the built-in tiny one to the right of the USB jack


//----------- Global Variables

String g_tokenResponseBuffer = "";
String g_cibmnResponseBuffer = "";
String g_cibcidResponseBuffer = "";
String g_packagesResponseBuffer = "";

String JSONParseError = "";
String g_recentErrors = "";
String debug2 = "";
int debug3 = 0;
int debug4 = 0;
int debug5 = 0;

String g_packages = ""; // Not implemented yet

struct struct_cardData {
    int clientID = 0;
    String UID = "";
} g_cardData;


struct struct_authTokenCheckIn {
   String token = ""; 
   unsigned long goodUntil = 0;   // if millis() returns a value less than this, the token is valid
} g_authTokenCheckIn;

struct  struct_clientInfo {  // holds info on the current client
    bool isValid = false;        // when true this sturcture has good data in it
    int clientID = 0;           // numeric value assigned by EZFacility. Guaranteed to be unique
    String RFIDCardKey = "";    // string stored in EZFacility "custom fields". We may want to change this name
    String memberNumber = "";   // string stored in EZFacility. May not be unique
    String contractStatus = ""; // string returned from EZF. Values we know of: Active, Frozen, Cancelled, Suspended
} g_clientInfo;

typedef enum eRetStatus {
    IN_PROCESS = 1,
    COMPLETE_OK = 2,
    COMPLETE_FAIL = 3,
    IDLE =4
} eRetStatus;


// ------------------   Forward declarations, when needed
//


//-------------- Particle Publish Routines --------------
// These routines are used to space out publish events to avoid being throttled.
//

int particlePublish (String eventName, String data) {
    
    static unsigned long lastSentTime = 0;
    
    if (millis() - lastSentTime > 1000 ){
        // only publish once a second
        
        Particle.publish(eventName, data);
        lastSentTime = millis();
        
        return 0;
        
    } else {
        
        return 1;
        
    }
}

String messageBuffer = "";
// Call this from the main look with a null message
void debugEvent (String message) {
    
    if (message.length() > 0 ){
        // a message was passed in
        
        messageBuffer =  message + " | " + messageBuffer;
        
        if (messageBuffer.length() > 600 ) {
            // message buffer is too long
            
            messageBuffer = messageBuffer.substring(0,570) + " | " + "Some debug messages were truncated";
        }
        
    }
    
    if (messageBuffer.length() > 0) {
        
        // a message buffer is waiting
    
        int rtnCode = particlePublish ("debugX", messageBuffer);
        
        if ( rtnCode == 0 ) {
        
            // it succeeded
            messageBuffer = "";
        
        }
    }
}



// ------------- Get Authorization Token ----------------

int ezfGetCheckInTokenCloud (String data) {
    
    ezfGetCheckInToken();
    return 0;
    
}

int ezfGetCheckInToken () {
    
    if (g_authTokenCheckIn.goodUntil < millis() ) {
        // Token is no longer good    
        g_authTokenCheckIn.token = "";
        g_tokenResponseBuffer = "";
        Particle.publish("ezfCheckInToken", "");
    
    }

    return 0;
    
}

void ezfReceiveCheckInToken (const char *event, const char *data)  {
    
    // accumulate response data
    g_tokenResponseBuffer = g_tokenResponseBuffer + String(data);
    
    debugEvent ("Receive CI token " + String(data) );
    
    const int capacity = JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> docJSON;
   
    char temp[3000]; //This has to be long enough for an entire JSON response
    strcpy_safe(temp, g_tokenResponseBuffer.c_str());
    
    // will it parse?
    DeserializationError err = deserializeJson(docJSON, temp );
    JSONParseError =  err.c_str();
    if (!err) {
        //We have valid JSON, get the token
        g_authTokenCheckIn.token = String(docJSON["access_token"].as<char*>());
        g_authTokenCheckIn.goodUntil = millis() + docJSON["expires_in"].as<int>()*1000 - 5000;   // set expiry five seconds early
        
        debugEvent ("now " + String(millis()) + "  Good Until  " + String(g_authTokenCheckIn.goodUntil) );
   
    }
}

// ------------ ClientInfo Utility Functions -------------------

void clearClientInfo() {
    
    g_clientInfo.isValid = false;
    g_clientInfo.clientID = 0;
    g_clientInfo.RFIDCardKey = "";
    g_clientInfo.contractStatus = "";
    
}

int parseClientInfoJSON (String data) {
    
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
            
            // member id is not unique
            g_recentErrors = "More than one client info in JSON ... " + g_recentErrors;
            
        } else {
         
            JsonObject root_0 = docJSON[0];
            g_clientInfo.clientID = root_0["ClientID"].as<int>();
            
            g_clientInfo.contractStatus = root_0["MembershipContractStatus"].as<char*>();
            
            String fieldName = root_0["CustomFields"][0]["Name"].as<char*>();
            
            if (fieldName.indexOf("RFID Card UID") >= 0) {
               g_clientInfo.RFIDCardKey = root_0["CustomFields"][0]["Value"].as<char*>(); 
            }
            
            g_clientInfo.isValid = true;
            
        }
        
        return 0;
        
    } else {
        
        debugEvent("JSON parse error " + JSONParseError);
        return 1;
    }
    
}


// ------------- Get Client Info by MemberNumber ----------------

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
    
    int rtnCode = Particle.publish("ezfClientByMemberNumber",output );
    if (rtnCode){} //XXX
    
    return g_clientInfo.memberNumber.toInt();
}


void ezfReceiveClientByMemberNumber (const char *event, const char *data)  {
    
    g_cibmnResponseBuffer = g_cibmnResponseBuffer + String(data);
    debugEvent("clientInfoPart " + String(data));
    
    parseClientInfoJSON(g_cibmnResponseBuffer); // try to parse it

}


// ------------- Get Client Info by ClientID ----------------
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
    
    int rtnCode = Particle.publish("ezfClientByClientID",output );
    
    return 0;
}


void ezfReceiveClientByClientID (const char *event, const char *data)  {
    
    g_cibcidResponseBuffer = g_cibcidResponseBuffer + String(data);
    debugEvent ("clientInfoPart " + String(data));
    
    const size_t capacity = 3*JSON_ARRAY_SIZE(2) + 2*JSON_ARRAY_SIZE(3) + 10*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(20) + 1050;
    DynamicJsonDocument docJSON(capacity);
   
    char temp[3000]; //This has to be long enough for an entire JSON response
    strcpy_safe(temp, g_cibcidResponseBuffer.c_str());
    
    // will it parse?
    DeserializationError err = deserializeJson(docJSON, temp );
    JSONParseError =  err.c_str();
    if (!err) {

        g_clientInfo.clientID = docJSON["ClientID"].as<int>();
            
        g_clientInfo.contractStatus = docJSON["MembershipContractStatus"].as<char*>();
            
        String fieldName = docJSON["CustomFields"][0]["Name"].as<char*>();
            
        if (fieldName.indexOf("RFID Card UID") >= 0) {
            g_clientInfo.RFIDCardKey = docJSON["CustomFields"][0]["Value"].as<char*>(); 
        }
            
        g_clientInfo.isValid = true;
        
    }
}

// ----------------- GET PACKAGES BY CLIENT ID ---------------


int ezfGetPackagesByClientID (String notused) {

    g_packagesResponseBuffer = "";
    g_packages = "";

    // Create parameters in JSON to send to the webhook
    const int capacity = JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> docJSON;
    
    docJSON["access_token"] = g_authTokenCheckIn.token.c_str();
    docJSON["clientID"] = String(g_clientInfo.clientID).c_str();
    
    char output[1000];
    serializeJson(docJSON, output);
    
    int rtnCode = Particle.publish("ezfGetPackagesByClientID",output );
    
    return rtnCode;
}


void ezfReceivePackagesByClientID (const char *event, const char *data)  {
    
    g_packagesResponseBuffer = g_packagesResponseBuffer + String(data);
    debugEvent ("PackagesPart" + String(data));
    
    DynamicJsonDocument docJSON(2048);
   
    char temp[3000]; //This has to be long enough for an entire JSON response
    strcpy_safe(temp, g_packagesResponseBuffer.c_str());
    
    // will it parse?
    DeserializationError err = deserializeJson(docJSON, temp );
    JSONParseError =  err.c_str();
    if (!err) {
        
        
    }
        
}

// ----------------- CHECK IN CLIENT -------------------
int ezfCheckInClient(String clientID) {
    
     // Create parameters in JSON to send to the webhook
    const int capacity = JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> docJSON;
    
    docJSON["access_token"] = g_authTokenCheckIn.token.c_str();
    docJSON["clientID"] = clientID.c_str();
    
    char output[1000];
    serializeJson(docJSON, output);
    
    int rtnCode = Particle.publish("ezfCheckInClient",output );
    
    return rtnCode;
}



// ------------------- CLOUD TESTING CALLS ----------------

// Called to simulate a card read. 
// Pass in: clientID,cardUID

int RFIDCardReadCloud (String data) {
    
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

// ------------ isClientOkToCheckIn --------------
//  This routine is where we check to see if we should allow
//  the client to check in or if we should deny them. This
//  routine will use g_clientInfo to make the determination.
//    
//  Returns True if client is good and false if not
//
bool isClientOkToCheckIn (){

    // Test for a good account status
    bool allowIn = false;
    if (g_clientInfo.contractStatus.length() > 0) {
        if (g_clientInfo.contractStatus.indexOf("Active") >= 0) {   
            allowIn = true; 
        }
    }

    return allowIn;

}

void heartbeatLEDs() {
    
    static unsigned long lastBlinkTime = 0;
    static int ledState = HIGH;
    const int blinkInterval = 500;  // in milliseconds
    
    
    if (millis() - lastBlinkTime > blinkInterval) {
        
        if (ledState == HIGH) {
            ledState = LOW;
        } else {
            ledState = HIGH;
        }
        
        digitalWrite(led, ledState);   // Turn ON the LED pins
        digitalWrite(led2, ledState);
        lastBlinkTime = millis();
    
    }
    
}

/*************** FUNCTIONS FOR USE IN REAL CARD READ APPLICATIONS *******************************/

/**************************************************************************************
 * createTrailerBlock():  creates a 16 byte data block from two 6 byte keys and 4 bytes
 *  of access control data. The data blcok is assembled in the global array "data"
 * 
 *  params:
 *      keya:  pointer to a 6 byte array of unsigned bytes holding key A
 *      keyb:  pointer to a 6 byte array of unsigned bytes holding key B
 *      acb:   pointer to a 4 byte array of unsigned bytes holding the access control bits
 *              for the sector.
 *      data: pointer to a 16 byte array of unsigned bytes holding the blcok data
 * 
****************************************************************************************/
void createTrailerBlock(uint8_t *keya, uint8_t *keyb, uint8_t *acb, uint8_t *data) {
    for (int i = 0; i < 6; i++) {	// key A in bytes 0 .. 5
        data[i] = keya[i];
    }
    
    for (int i = 0; i < 4; i++) {	// ACB in bytes 6, 7, 8, 9
	   data[i + 6] = acb[i];
    }
    
    for (int i = 0; i < 6; i++) {	// key B in bytes 10 .. 15
	   data[i + 10] = keyb[i];
    }
} // end of createTrailerBlock()

/**************************************************************************************
 * writeTrailerBlock():  writes a 16 byte data block to the trailer block of the 
 *  indicated sector.  NOTE: Mifare Classic 1k card is assumed.  The sector trailer
 *  block is block 3 relative to the sector and each sector contains 4 blocks.  NOTE:
 *  the trailer block must first be authenticated before this write will work.
 * 
 *  params:
 *      sector: the sector number (int)
 *      data: pointer to a 16 byte array of unsigned bytes to write to the sector
 *          trailer block
 * 
 *  return:
 *      the result of the write attempt (uint8_t)
 *      
****************************************************************************************/
uint8_t writeTrailerBlock(int sector, uint8_t *data) {
    
    // compute the absolute block number from the sector number
    // the trailer block is block 3 relative to the sector
    int blockNum = (sector * 4) + 3;
    
   // write the block
    uint8_t success = nfc.mifareclassic_WriteDataBlock (blockNum, data);
    return success;
    
} // end of writeTrailerBlock()

/**************************************************************************************
 * authenticateBlock():  authenticates a data block relative to the current SECTOR
 *  using the indicated key
 * 
 *  params:
 *      blockNum: the number of the block relative to the SECTOR (0, 1 or 2)
 *      keyNum: 0 for keyA or 1 for keyB
 *      key: pointer to a 6 byte array holding the value of the key
 * 
 *  return:
 *      the result of the authentication attempt (uint8_t)
 *      
****************************************************************************************/
uint8_t authenticateBlock(int blockNum, uint8_t keyNum, uint8_t *key) {
    // check that the relative block number is 0, 1, 2 or 3 only
    if(blockNum > 3) {
        #ifdef TEST
            Serial.print("bad relative block number: ");
            Serial.println(blockNum);
            delay(1000);
        #endif
        
        return 0;   // return with error indication
    }
    
    // compute the absolute block number:  (SECTOR * 4) + relative block number
    uint32_t absoluteBlockNumber = (SECTOR * 4) + blockNum;
    
    // check that keyNum is 0 (key A) or 1 (key B)
    if(keyNum > 1) {
        #ifdef TEST
            Serial.println("bad key number.  Must be 0 or 1");
            delay(1000);
        #endif
        
        return 0;   // return with error indication
    }
    
    // key is valid
    #ifdef TEST
        Serial.print("trying to authenticate block number: ");
        Serial.print(absoluteBlockNumber);
        Serial.print(" using key ");
        if(keyNum == 0){
            Serial.println("A ");
        } else {
            Serial.println("B ");
        }
        // print out the key value
        nfc.PrintHexChar(key, 6);
        delay(1000);
    #endif
    
    // call the authentication library function
    uint8_t success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, absoluteBlockNumber, keyNum, key);
    
    #ifdef TEST
        if(success) {
           Serial.println("authentication succeeded");
        } else {
            Serial.println("authentication failed");
        }
        delay(1000);
    #endif
        
    return success;
}  //end of authenticateBlock()

/**************************************************************************************************
 * readBlockData(uint8_t *data, int blockNum,  uint8_t keyNum, uint8_t *key):  Read out the data from the 
 *  specified relative block number using the indicated key to authenticate the block prior
 *  to trying to read data.  NOTE: the function ensures that it is only possible to read from blocks
 *  0, 1 or 2 relative to the sector. 
 * 
 *  params:
 *      data: pointer to a 16 byte array to hold the returned data     
 *      blockNum:  the block number relative to the current SECTOR
 *      keyNum:  0 for key A; 1 for key B
 *      key: pointer to a 6 byte array holding the value of the reding key
 * 
 * return:
 *      0 (false) if operation failed
 *      1 (true) if operation succeeded
 *   
 **************************************************************************************************/
 bool readBlockData(uint8_t *data, int blockNum,  uint8_t keyNum, uint8_t *key) {
    bool authOK = false;
    bool readOK = false;
    
    // check that the relative block number is 0, 1, 2 only
    if(blockNum > 2) {
        #ifdef TEST
            Serial.print("bad relative block number: ");
            Serial.println(blockNum);
            delay(1000);
        #endif
        
        return 0;   // return with error indication
    }
    
    // compute the absolute block number:  (SECTOR * 4) + relative block number
    uint32_t absoluteBlockNumber = (SECTOR * 4) + blockNum;
    
    // first need to authenticate the block with a key
    authOK = authenticateBlock(blockNum, keyNum, key);
     
    if (authOK == true) {  // Block is authenticated so we can read the data
        readOK = nfc.mifareclassic_ReadDataBlock(absoluteBlockNumber, data);
        
        if(readOK == true) {
            #ifdef TEST
                Serial.println("\nData read OK\n");
                delay(1000);
            #endif 
            
            return true;   // successful read
        } else {
            #ifdef TEST
                Serial.println("\nData read failed!\n");
                delay(1000);
            #endif 
            
            return false;   // failed  read            
        }
        
    } else {
        #ifdef TEST
            Serial.println("\nBlock authentication failed!\n");
            delay(1000);
        #endif
        
        return false;
    }

 }  // end of readBlockData()
 
 /**************************************************************************************************
 * writeBlockData(uint8_t *data, int blockNum,  uint8_t keyNum, uint8_t *key):  Write a block of 
 *  data to the specified relative block number using the indicated key to authenticate the block prior
 *  to trying to write data.  NOTE: the function ensures that it is only possible to write to blocks
 *  0, 1 or 2 relative to the sector.  This function is not used to change keys or access control
 *  bits (writing to the sector trailer block).
 * 
 *  params:
 *      data: pointer to a 16 byte array containg the data to write     
 *      blockNum:  the block number relative to the current SECTOR
 *      keyNum:  0 for key A; 1 for key B
 *      key: pointer to a 6 byte array holding the value of the write key
 * 
 * return:
 *      0 (false) if operation failed
 *      1 (true) if operation succeeded
 *   
 **************************************************************************************************/
 bool writeBlockData(uint8_t *data, int blockNum,  uint8_t keyNum, uint8_t *key) {
    bool authOK = false;
    bool writeOK = false;
    
    // check that the relative block number is 0, 1, 2 only
    if(blockNum > 2) {
        #ifdef TEST
            Serial.print("bad relative block number: ");
            Serial.println(blockNum);
            delay(1000);
        #endif
        
        return 0;   // return with error indication
    }
    
    // compute the absolute block number:  (SECTOR * 4) + relative block number
    uint32_t absoluteBlockNumber = (SECTOR * 4) + blockNum;
    
    // first need to authenticate the block with a key
    authOK = authenticateBlock(blockNum, keyNum, key);
     
    if (authOK == true) {  // Block is authenticated so we can read the data
        writeOK = nfc.mifareclassic_WriteDataBlock(absoluteBlockNumber, data);
        
        if(writeOK == true) {
            #ifdef TEST
                Serial.println("\nData written OK\n");
                delay(1000);
            #endif 
            
            return true;   // successful write
        } else {
            #ifdef TEST
                Serial.println("\nData write failed!\n");
                delay(1000);
            #endif 
            
            return false;   // failed  write            
        }
        
    } else {
        #ifdef TEST
            Serial.println("\nBlock authentication failed!\n");
            delay(1000);
        #endif
        
        return false;
    }

 }  // end of writeBlockData()
 
 /**************************************************************************************************
  * changeKeys(uint8_t *oldKey, uint8_t *newKayA, uint8_t *newKayB, uint8_t *newACB):  changes the
  *  sector trailer bock of the current SECTOR with news keys and new access control bits.  A
  *  currently valid key is needed to authentication with.
  * 
  * params:
  *     oldKeyNum: 0 for keyA, 1 for keyB.  Old key needed to authenticate for writing
  *     oldKey: pointer to a byte array containing the current key needed for writing the sector
  *         trailer block
  *     newKeyA: pointer to a 6 byte array containing the new key A to write
  *     newKeyB: pointer to a 6 byte array containing the new key B to write
  *     newACB: pointer to a 4 byte array containing the new access control bits to write
  * 
  * return:
  *     true indicates success; false otherwise
 ****************************************************************************************************/
 bool changeKeys(uint8_t oldKeyNum, uint8_t *oldKey, uint8_t *newKeyA, uint8_t *newKeyB, uint8_t *newACB) {
    
    // compute the absolute block number of the sector trailer.
    //  The sector trailer blcok is relative block 3 of the sector for a Classic 1K card
    //uint32_t absoluteBlockNumber = (SECTOR * 4) + 3;
    
    #ifdef TEST
        Serial.print("\nChanging keys for sector ");
        Serial.println(SECTOR);
    #endif

    // we must first authenticate this block -- relative block 3 -- with the old key
   bool success = authenticateBlock(3, oldKeyNum, oldKey);
    if(success == true) {
        #ifdef TEST
            Serial.println("authentication with current key successful.");
        #endif  
        
        // write the new keys and ACB to the sector trailer block
        // first make the new block
        uint8_t newBlock[16];
        createTrailerBlock(newKeyA, newKeyB, newACB, newBlock);
        #ifdef TEST
            Serial.println("New sector trailer block is:");
            nfc.PrintHex(newBlock, 16);
            Serial.println("");
        #endif
        
        //now write it to the sector trailer block on the card        
        uint8_t OK = writeTrailerBlock(SECTOR, newBlock);
        
        if(OK == true) {
            #ifdef TEST
                Serial.println("New sector trailer written successfully!");
            #endif 
            
            return true;
        } else {
             #ifdef TEST
                Serial.println("New sector trailer write failed!");
            #endif 
            
            return false;           
        }
        
    } else {
        #ifdef TEST
            Serial.println("authentication with current key failed.");
        #endif
        
        return false;
    }
    
 } // end of changeKeys()
 

/***************************************************************************************************
 * testCard():  Tests an ISO14443A card to see if it is factory fresh, MN formatted, or other.
 *  The test is performed by authenticating relative block 2 (not otherwise used data block)
 *  on the designated sector (nominally sector 1).  If the block authenticates with default 
 *  key A then the card is declared to be factory fresh.  If the block 
 *  authenticates with MN secret key A then the card is declared to be 
 *  MN formatted.  Otherwise, the card is declared to be "other" and the designed sector is likely 
 *  unusable.
 * 
 * return:  result code, as uint8_t, as follows:
 *  0 means factory fresh card
 *  1 means MN formatted card
 *  2 means neither type (sector most likely unusable)
****************************************************************************************************/
uint8_t testCard() {
    bool success = false;
    
    #ifdef TEST
        Serial.println("Trying to authenticate with default key A ....");
    #endif
    
    success = authenticateBlock(2, 0, DEFAULT_KEY_A);
    if(success == true)   {   // we can assume a factory fresh card
        #ifdef TEST
            Serial.println("default key A authenticated.  Assume factory fresh card ...");
        #endif
        
        return 0;   // code for factory fresh card
    } else {    // not a factory fresh card; reset and test for MN formatted card
        #ifdef TEST
            Serial.println("Not a factory fresh card.  Reset and test for MN format .....");
        #endif
        
        // reset by reading the targe ID again
        nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        success = authenticateBlock(2, 0, MN_SECRET_KEY_A);        
        if(success == true) {   // we can assume an MN formatted card
            #ifdef TEST
                Serial.println("MN secret key A authenticated.  Assume NM formatted card ...");
            #endif
            
            return 1;   // code for MN formatted card
        } else {    // neither test passed - card type is unknown
            #ifdef TEST
                Serial.println("Not an MN formatted card - card type unknown ...");
            #endif
            
            return 2;   // code for unknown card format
        }
    }
}   // end of testCard()


// ------------------ ReadCard ---------------
//
// Called from main loop to see if card is presented and readable.
// 
// When this routine returns COMPLETE_OK the g_cardData should be 
// set, in particular if .clientID is not 0 then the main loop will
// try to checkin this client
//

eRetStatus readTheCard() {
    
  /*
    if (g_cardData.clientID != 0) {
        return COMPLETE_OK;
    } else {
        return IDLE;
    }
*/

    eRetStatus returnStatus = IN_PROCESS;

    //uint8_t success;
    uint8_t dataBlock[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}; //  16 byte buffer to hold a block of data
    //bool flag = false;  // toggle flag for testing purposes

    
    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    Serial.println("waiting for ISO14443A card to be presented to the reader ...");
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Place card on");
    lcd.setCursor(0,1);
    lcd.print("reader");
    
    if(!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // no card presented so we just exit
        return IN_PROCESS;
    }

    // we have a card presented
    g_cardData.clientID = 0;
    g_cardData.UID = "";
    lcd.clear();
  
    #ifdef TEST
        // Display some basic information about the card
        Serial.println("Found an ISO14443A card");
        Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);
        Serial.println("");
        delay(1000);
    #endif

    // test the card to determine its type
    uint8_t cardType = testCard();

    Serial.print("\n\nCard is type ");
    if(cardType == 0) {
         Serial.println("factory fresh\n");
    } else if (cardType == 1) {
         Serial.println("Maker Nexus formatted\n");        
    } else {
        Serial.println("other card format\n");
    }

    if(cardType == 0) { // factory fresh card

        // do nothing factory fresh code
        lcd.setCursor(0,0);
        lcd.print("Card is not MN");
        returnStatus = COMPLETE_FAIL;

    } else  {  // MN formatted card

        // now read the data using MN Key and store in g_cardData
        readBlockData(dataBlock, 0,  0, MN_SECRET_KEY_A);
        Serial.println("The clientID data is:");
        nfc.PrintHex(dataBlock, 16);
        Serial.println("");

        String theClientID = "";
        for (int i=0; i<16; i++) {
            theClientID = theClientID + String( (char) dataBlock[i] ); 
        }
        
        g_cardData.clientID = theClientID.toInt();

        // read UID and store in g_cardData
        readBlockData(dataBlock, 1,  0, MN_SECRET_KEY_A);
        Serial.println("The MN UID data is:");
        nfc.PrintHex(dataBlock, 16); 

        String theUID = "";
        for (int i=15; i>-1; i--) {
            if (dataBlock[i] != 0) {
                theUID = String( (char) dataBlock[i] ) + theUID; 
            } else {
                break; // we reached a null data character;
            }
        }
        
        g_cardData.UID = theUID;
        
        // display the status to the user
        String msg = "";
        if (g_cardData.clientID == 0 ) {
            returnStatus = COMPLETE_FAIL;
            msg = "Card read failed";
            tone(BUZZER_PIN,250,500);
        } else {
            returnStatus = COMPLETE_OK;
            msg = "CID:" + String(g_cardData.clientID);
            tone(BUZZER_PIN,750,50); //good
        }
        lcd.setCursor(0,0);
        lcd.print(msg);
        Serial.println(msg);
        Serial.println("");  

    }
    
    Serial.println("Remove card from reader ...");

    lcd.setCursor(0,1);
    lcd.print("Remove card ...");
    
    while(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // wait for card to be removed
    }
    
    return returnStatus;
    
}





// --------------------- SETUP -------------------
// This routine runs only once upon reset
void setup() {

  // Initialize D0 + D7 pin as output
  // It's important you do this here, inside the setup() function rather than outside it or in the loop function.
    pinMode(led, OUTPUT);
    pinMode(led2, OUTPUT);
    
    pinMode(READY_LED, OUTPUT);
    pinMode(ADMIT_LED, OUTPUT);
    pinMode(REJECT_LED, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    digitalWrite(READY_LED, LOW);
    digitalWrite(ADMIT_LED, LOW);
    digitalWrite(REJECT_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
 
#ifdef LCD_PRESENT
    lcd.begin(16,2);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MN Checkin");
    lcd.setCursor(0,1);
    lcd.print("StartUp");
#endif
    
#ifdef TEST
    delay(5000);    // delay to get putty up
    Serial.println("trying to connect ....");
#endif

#ifdef RFID_READER_PRESENT 
    // ------ RFID SetUp
    pinMode(IRQ_PIN, INPUT);     // IRQ pin from PN532
    //pinMode(RST_PIN, OUTPUT);    // reserved for PN532 RST -- not used at this time
    pinMode(LED_PIN, OUTPUT);   // the D7 LED
    
    nfc.begin(); 
 
    uint32_t versiondata;
    
    do {
        versiondata = nfc.getFirmwareVersion();
        if (!versiondata) {             
            Serial.println("no board");
            delay(1000);
        }
    }  while (!versiondata);
            
    message += "Found chip PN532; firmware version: ";
    message += (versiondata>>16) & 0xFF;
    message += '.';
    message += (versiondata>>8) & 0xFF;
    Serial.println(message);
    
    // now start up the card reader
    nfc.SAMConfig();
    
    //flash the D7 LED twice
    for (int i = 0; i < 2; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN, LOW);
        delay(500);
    } 

#endif

    // ------- CheckIn Setup

    Particle.variable ("ClientID", g_clientInfo.clientID);
    Particle.variable ("RFIDCardKey", g_clientInfo.RFIDCardKey);
    Particle.variable ("ContractStatus",g_clientInfo.contractStatus);
    Particle.variable ("g_Packages",g_packages);
    Particle.variable ("recentErrors",g_recentErrors);
    
    Particle.variable ("JSONParseError", JSONParseError);
      
    Particle.variable ("debug2", debug2);
    Particle.variable ("debug3", debug3);
    Particle.variable ("debug4", debug4);
 
    int success = Particle.function("RFIDCardRead", RFIDCardReadCloud);
    if (success) {}

    success = Particle.function("GetCheckInToken", ezfGetCheckInTokenCloud);
    Particle.subscribe(System.deviceID() + "ezfCheckInToken", ezfReceiveCheckInToken, MY_DEVICES);
  
    success = Particle.function("ClientByMemberNumber", ezfClientByMemberNumber);
    Particle.subscribe(System.deviceID() + "ezfClientByMemberNumber", ezfReceiveClientByMemberNumber, MY_DEVICES);
      
    success = Particle.function("ClientByClientID", ezfClientByClientIDCloud);
    Particle.subscribe(System.deviceID() + "ezfClientByClientID", ezfReceiveClientByClientID, MY_DEVICES);
    
    success = Particle.function("PackagesByClientID",ezfGetPackagesByClientID);
    Particle.subscribe(System.deviceID() + "ezfGetPackagesByClientID",ezfReceivePackagesByClientID);

#ifdef LCD_PRESENT
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MN Checkin");
    lcd.setCursor(0,1);
    lcd.print("Initialized");
#endif

    // Signal ready to go
    tone(BUZZER_PIN,750,50); //good
    delay(100);
    tone(BUZZER_PIN,750,50); //good

}



// This routine gets called repeatedly, like once every 5-15 milliseconds.
// Spark firmware interleaves background CPU activity associated with WiFi + Cloud activity with your code. 
// Make sure none of your code delays or blocks for too long (like more than 5 seconds), or weird things can happen.
void loop() {
    
    enum mlState {mlsIDLE, mlsCARDPRESENT, mlsCARDOK, mlsREQUESTTOKEN, mlsWAITFORTOKEN, mlsASKFORCLIENTINFO, mlsWAITFORCLIENTINFO, mlsCHECKINGIN, mlsERROR};
    
    static mlState mainloopState = mlsIDLE;
    static unsigned long processStartMilliseconds = 0; 
    
    switch (mainloopState) {
    case mlsIDLE: {
        eRetStatus retStatus = readTheCard();
        if (retStatus == COMPLETE_OK) {
            // move to the next step
            mainloopState = mlsREQUESTTOKEN;
            }
        break;
    }
    case mlsREQUESTTOKEN: 
        // request a good token from ezf
        ezfGetCheckInTokenCloud("junk");
        processStartMilliseconds = millis();
        mainloopState = mlsWAITFORTOKEN;
        break;

    case mlsWAITFORTOKEN:
        // waiting for a good token from ezf
        if (g_authTokenCheckIn.token.length() != 0) {
            // we have a token
            mainloopState = mlsASKFORCLIENTINFO;
        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("took too long to get token, checkin aborts");
                processStartMilliseconds = 0;
                mainloopState = mlsIDLE;
            }
        //Otherwise we stay in this state
        }
        break;
    case mlsASKFORCLIENTINFO:  {

        // ask for client details
        ezfClientByClientID(g_cardData.clientID);
        processStartMilliseconds = millis();
        mainloopState = mlsWAITFORCLIENTINFO;
        break;
        
    }
    case mlsWAITFORCLIENTINFO: {
  
        if ( g_clientInfo.isValid ) {
            // got client data, let's move on
            mainloopState = mlsCHECKINGIN;
        } else {
            // timer to limit this state
            if (millis() - processStartMilliseconds > 15000) {
                debugEvent("15 second timer exeeded, checkin aborts");
                processStartMilliseconds = 0;
                mainloopState = mlsIDLE;
            }
        } // Otherwise we stay in this state
        break;
    }
    case mlsCHECKINGIN: {

        // If the client meets all critera, check them in
        bool allowIn = isClientOkToCheckIn();
        if ( !allowIn ) {
            //client account status is bad
            mainloopState = mlsIDLE;
            debugEvent("contract status is not good."); 

            if (g_clientInfo.contractStatus.length() >= 1) {
                debugEvent("status: " + g_clientInfo.contractStatus);
            }  else {
                debugEvent("Status is null");
            }
            
        } else {
        
            debugEvent ("SM: now checkin client");
            // tell EZF to check someone in
            ezfCheckInClient(String(g_clientInfo.clientID));
            tone(BUZZER_PIN,750,50); //good
            delay(100);
            tone(BUZZER_PIN,750,50);
            mainloopState = mlsIDLE;
        
            }
    }
    case mlsERROR:
        break;
    default:
        break;
    } //switch (mainloopState)
    
    
    heartbeatLEDs();

    debugEvent("");  // need this to pump the debug event process

}
