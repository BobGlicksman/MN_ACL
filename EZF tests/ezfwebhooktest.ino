//Required to get ArduinoJson to compile
#define ARDUINOJSON_ENABLE_PROGMEM 0
#include <ArduinoJson.h>

// This #include statement was automatically added by the Particle IDE.
#include <ArduinoJson.h>
//https://arduinojson.org/v6/assistant/

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


String JSONParseError = "";
String g_recentErrors = "";
String debug2 = "";
int debug3 = 0;
int debug4 = 0;
int debug5 = 0;


// ------------------   Forward declarations, when needed
//
eRetStatus checkInClientByClientID(int clientID, String cardUID);





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

// -----------------------Check In a Client by MemberNum-------------------
// Called to checkin a client in EZFacility. Will also update our cloud database
//
// This routine should be called from the main look with a clientID of 0
//
// Pass in memberNumber
//    clientID - the key used to lookup a member in EZFacility
//    cardUID - the UID must match that stored in EZFacility for this clientID
//    
// Returns:
//       IN_PROCESS = 1,
//       COMPLETE_OK = 2,
//       COMPLETE_FAIL = 3
//
// Other actions
//    If g_authTokenCheckIn is not valid, will get a valid token
//    g_clientInfo will be valid when this function returns success
//    If fail, then ????? global variable has human readable information about the failure
//    

eRetStatus checkInClientByClientID(int clientID, String cardUID) {
    
    static int state = -1;
    static unsigned long startOfRequest = -1;
    static int clientIDToUse = 0;


    if ( (state == -1) && (clientID == 0) ) {
        // we are not in process and we were not passed a memberNum
        // so quick return
        return IDLE;
    }

    if ( (state > -1) && (clientID > 0) ) {
        // We were in process and called with a member number
        // abort and go idle
        debugEvent ("SM: new checkin request while in process.  member number: " + clientID);
        state = -1;
        return COMPLETE_FAIL;
    
    } 
    
    if (state > -1) {
        if (millis() - startOfRequest > 15000) {
            // this process has taken too long. Abort it. And go to Idle.
            state = -1;
            debugEvent ("SM: error checkInClientByMemberNum took over 15 seconds");
            return COMPLETE_FAIL;
        }
        
    }
    
    if ( (state == -1) && (clientID > 0)) {
        // This is a new request
        state = 1;
        startOfRequest = millis();
        clientIDToUse = clientID; // keep this in a local static variable
       // debugEvent("SM: starting new checkin request with member number: " + clientID);
        
    }
    
//int temp3 = System.freeMemory();
//debugEvent(String(temp3));   
 
// return COMPLETE_OK;   
    
    int var;
    int retCode;
      
    switch (state) {
    case 1: 
        {
       // debugEvent ("SM: request token");
        
        // request a good token
        ezfGetCheckInTokenCloud("junk");
        
        state++;;
        break;
        }
    case 2:
        // waiting for a good token
        if (g_authTokenCheckIn.token.length() == 0) {
            break;
        }
        
       // debugEvent ("SM: got token: " + g_authTokenCheckIn.token);
        
        // ask for client details
        ezfClientByClientID(clientIDToUse);
       
        state++;
        break;
    case 3: {
    
            // waiting for client details
            if ( !g_clientInfo.isValid) {
                break;
            }
      
            debugEvent ( "step 3");
       
            // Test for a good account status
            bool allowIn = false;
            if (g_clientInfo.contractStatus.length() > 0) {
                if (g_clientInfo.contractStatus.indexOf("Active") >= 0) {   
                   allowIn = true; 
                }
            }
            
            // If we have good status, checkin the client
            if ( !allowIn ) {
                //client account status is bad
                state = -1;
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
                
                state++;
            
            }
          
            break;
        }
    case 4:
        // waiting for check in to complete
        debugEvent ("SM: checkin complete");
        debugEvent ("SM: step 4");
        // log the check in to the google database
        state++;
        break;
    case 5:
        // wait for google database to complete
        debugEvent ("SM: step 5");
        // report that all is done
        state++;
        break;
    case 6:
        // we are done
        state = -1;
        break;
    default:
        // We have some error
        debugEvent ("SM: default. state: " + String(state));
        return COMPLETE_FAIL;
        break;
    }
    
    debugEvent("SM: exit with state: " + String(state));
    if (state == -1) {
        return COMPLETE_OK;
    } else {
        return IN_PROCESS;
    } 
    
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
        
        
    }
    
    
    
}



// ------------------ ReadCard ---------------
//
// placeholder routine until integrating Bob's code
//
// When this routine returns COMPLETE_OK the 

eRetStatus readTheCard() {
    
    if (g_cardData.clientID != 0) {
        return COMPLETE_OK;
    } else {
        return IDLE;
    }
    
}

// --------------------- SETUP -------------------
// This routine runs only once upon reset
void setup() {
  // Initialize D0 + D7 pin as output
  // It's important you do this here, inside the setup() function rather than outside it or in the loop function.
    pinMode(led, OUTPUT);
    pinMode(led2, OUTPUT);
    int i = 2;
 

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
 
  
    success = Particle.function("GetCheckInToken", ezfGetCheckInTokenCloud);
    Particle.subscribe(System.deviceID() + "ezfCheckInToken", ezfReceiveCheckInToken, MY_DEVICES);
  
    success = Particle.function("ClientByMemberNumber", ezfClientByMemberNumber);
    Particle.subscribe(System.deviceID() + "ezfClientByMemberNumber", ezfReceiveClientByMemberNumber, MY_DEVICES);
      
    success = Particle.function("ClientByClientID", ezfClientByClientIDCloud);
    Particle.subscribe(System.deviceID() + "ezfClientByClientID", ezfReceiveClientByClientID, MY_DEVICES);
    
    success = Particle.function("PackagesByClientID",ezfGetPackagesByClientID);
    Particle.subscribe(System.deviceID() + "ezfGetPackagesByClientID",ezfReceivePackagesByClientID);



}



// This routine gets called repeatedly, like once every 5-15 milliseconds.
// Spark firmware interleaves background CPU activity associated with WiFi + Cloud activity with your code. 
// Make sure none of your code delays or blocks for too long (like more than 5 seconds), or weird things can happen.
void loop() {
    
    enum mlState {mlsIDLE, mlsCARDPRESENT, mlsCARDOK, mlsCHECKINGIN, mlsERROR};
    
    static mlState mainloopState = mlsIDLE;
    
    switch (mainloopState) {
    case mlsIDLE: {
        
        eRetStatus retStatus = readTheCard();
        if (retStatus == COMPLETE_OK) {
            // move to the next step
            mainloopState = mlsCHECKINGIN;
        }
        break;
    }
    case mlsCHECKINGIN: {
        
        eRetStatus retStatus = checkInClientByClientID(0,""); 
        debugEvent("state machine returned status: " + String(retStatus));
        
        switch (retStatus) {
        case IN_PROCESS:
            break;
        case IDLE: {
            
            eRetStatus retStatus = checkInClientByClientID(g_cardData.clientID, g_cardData.UID); 
            // xxx we should check the return status here to make sure the checkin took off
            g_cardData.clientID = 0;
            g_cardData.UID = "";
            break;
        }
        case COMPLETE_OK:
            mainloopState = mlsIDLE;
            break;
        case COMPLETE_FAIL:
            break;
        } // switch (retStatus)
        
        break;
    }
    case mlsERROR:
        break;
    default:
        break;
    } //switch (mainloopState)
    
    
    heartbeatLEDs();

    debugEvent("");  // need this to pump the debug event process

}

