// This file holds common unilities that are used by modules in the RFID project
// of Maker Nexus
//
// (CC BY-NC-SA 3.0 US) 2019 Maker Nexus   https://creativecommons.org/licenses/by-nc-sa/3.0/us/
//

#include "mnutils.h"

enumRetStatus  eRetStatus;

enumAdminCommand g_adminCommand = acIDLE;
String g_adminCommandData = "";

enumDeviceConfigType  eDeviceConfigType;

struct_clientInfo g_clientInfo;
structEEPROMdata EEPROMdata;

String JSONParseError = "";


// instatiate the LCD
LiquidCrystal lcd(A0, A1, A2, A3, D5, D6);

void buzzerBadBeep() {
    tone(BUZZER_PIN,250,500);
}

void buzzerGoodBeep(){
    tone(BUZZER_PIN,750,50); //good
}

void buzzerGoodBeeps2(){
    tone(BUZZER_PIN,750,50); //good
    delay(100);
    tone(BUZZER_PIN,750,50);
}

// writeToLCD
// pass in "","" to clear screen 
// pass in "" for one line to leave it unchanged
void writeToLCD(String line1, String line2) {
#ifdef LCD_PRESENT
    const char* BLANKLINE = "                ";
    if ((line1.length() == 0) & (line2.length() ==0)) {
        lcd.clear();
    } else {
        if (line1.length() > 0){
            lcd.setCursor(0,0);
            lcd.print(BLANKLINE);
            lcd.setCursor(0,0);
            lcd.print(line1.substring(0,15));                        
        }
        if (line2.length() > 0){
            lcd.setCursor(0,1);
            lcd.print(BLANKLINE);
            lcd.setCursor(0,1);
            lcd.print(line2.substring(0,15));
        }
    }
    


#endif
}


//-------------- Particle Publish Routines --------------
// These routines are used to space out publish events to avoid being throttled.
//

int particlePublish (String eventName, String data) {
    
    static unsigned long lastSentTime = 0;
    
    if (millis() - lastSentTime > 1000 ){
        // only publish once a second
        
        Particle.publish(eventName, data, PRIVATE);
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

// writes message to a webhook that will send it on to a cloud database
// These have to be throttled to less than one per second on each device
// Parameters:
//    logEvent - a short reason for logging ("checkin","reboot","error", etc)
//    logData - optional freeform text up to 250 characters
//    clientID - optional if this event was for a particular client 
void logToDB(String logEvent, String logData, int clientID, String clientFirstName){
    const size_t capacity = JSON_OBJECT_SIZE(10);
    DynamicJsonDocument doc(capacity);

    String idea2 = Time.format(Time.now(), "%F %T");
    doc["dateEventLocal"] = idea2.c_str();
    doc["deviceFunction"] =  deviceTypeToString(EEPROMdata.deviceType).c_str();
    doc["clientID"] = clientID;
    doc["firstName"] = clientFirstName.c_str();
    doc["logEvent"] = logEvent.c_str();
    doc["logData"] = logData.c_str();

    char JSON[2000];
    serializeJson(doc,JSON );
    String publishvalue = String(JSON);

    Particle.publish("RFIDLogging",publishvalue,PRIVATE);

    return;

}

// Write to the EEPROM 
//
// 
void EEPROMWrite () {
    int addr = 0;
    EEPROMdata.MN_Identifier = MN_EPROM_ID;
    EEPROM.put(addr, EEPROMdata);
}

// Read from EEPROM
//
//
void EEPROMRead() {
    int addr = 0;
    EEPROM.get(addr,EEPROMdata);
    if (EEPROMdata.MN_Identifier != MN_EPROM_ID) {
        EEPROMdata.deviceType = UNDEFINED_DEVICE;
    }
}

// Convert eDeviceConfigType to human readable for LCD
//
String deviceTypeToString(enumDeviceConfigType deviceType){
    String msg = "";
    switch (deviceType) {
    case UNDEFINED_DEVICE:
        msg = "UNDEFINED_DEVICE";
        break;
    case CHECK_IN_DEVICE:
        msg = "Check In";
        break;
    case ADMIN_DEVICE:
        msg = "Admin";
        break;
    case BOSSLASER:
        msg = "BossLaser";
        break;
    case WOODSHOP:
        msg = "Woodshop";
        break;
    default:
        msg = "ERR: UNKNOWN";
        break;
    }
    return msg;
} 


void clearClientInfo() {
    
    g_clientInfo.isValid = false;
    g_clientInfo.lastName = "";
    g_clientInfo.firstName = "";
    g_clientInfo.clientID = 0;
    g_clientInfo.RFIDCardKey = "";
    g_clientInfo.contractStatus = "";
    g_clientInfo.memberNumber = "";
    g_clientInfo.amountDue = 0;
    
}