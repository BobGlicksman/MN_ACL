// This file holds rfid utilities that are used by modules in the RFID project
// of Maker Nexus
//
// (CC BY-NC-SA 3.0 US) 2019 Maker Nexus   https://creativecommons.org/licenses/by-nc-sa/3.0/us/
//

#include "application.h"
#include "mnutils.h"

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_PN532.h>


// Sector to use for testing
const int SECTOR = 3;	// Sector 0 is manufacturer data and sector 1 is real MN data

// Encryption keys for testing
uint8_t DEFAULT_KEY_A[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t DEFAULT_KEY_B[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t TESTKEYA[6] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5 };
uint8_t TESTKEYB[6] = { 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5 };
uint8_t g_secretKeyA[6] = {0,0,0,0,0,0}; 
uint8_t g_secretKeyB[6] = {0,0,0,0,0,0};  
uint8_t g_secretKeyA_OLD[6] = {0,0,0,0,0,0}; 
uint8_t g_secretKeyB_OLD[6] = {0,0,0,0,0,0};  
bool g_secretKeysValid = false;

struct struct_cardData {
    int clientID = 0;
    String UID = "";
    String cardStatus = "";
    bool isValid = false;
} g_cardData;


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


/**************************
 * mnrfidInit();
 * called once by programs using this module
 * 
 */
void mnrfidInit() {

    writeToLCD("Initializing","RFID Reader");
    // ------ RFID SetUp
    pinMode(IRQ_PIN, INPUT);     // IRQ pin from PN532
    //pinMode(RST_PIN, OUTPUT);    // reserved for PN532 RST -- not used at this time
    
    nfc.begin(); 

    uint32_t versiondata = 0;
    
    do {
        versiondata = nfc.getFirmwareVersion();
        if (!versiondata) {             
            Serial.println("no board");
            writeToLCD("Setup error","NFC board error");
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
}

// ---------------- clearCardData -----------
void clearCardData() {
    g_cardData.isValid = false;
    g_cardData.clientID = 0;
    g_cardData.UID = "";
    g_cardData.cardStatus = "";
}


// xxx now that keys come from an include, this could be simpler; no need for JSON
// xxx and should be "setRFIDKeys"
// ------------- GET RFID KEYS -----------------
// Get RFID Keys
void responseRFIDKeys(const char *event, const char *data) {

    g_secretKeysValid = false;
    //const int capacity = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(1) + 10;
    StaticJsonDocument<800> docJSON;

    // will it parse?
    DeserializationError err = deserializeJson(docJSON, data ); // XXX
    JSONParseError =  err.c_str();
    if (!err) {
        //We have valid JSON, get the key
        JsonArray WriteKey = docJSON["WriteKey"];
        JsonArray ReadKey = docJSON["ReadKey"];
        JsonArray WriteKeyOld = docJSON["WriteKeyOld"];
        JsonArray ReadKeyOld = docJSON["ReadKeyOld"];
        for (int i=0;i<6;i++) {
            g_secretKeyA[i] = WriteKey[i];
            g_secretKeyB[i] = ReadKey[i];
            g_secretKeyA_OLD[i] = WriteKeyOld[i];
            g_secretKeyB_OLD[i] = ReadKeyOld[i];
            g_secretKeysValid = true;
        }
            
        debugEvent("key parsed");
        buzzerGoodBeep();
       
    } else {
        writeToLCD("JSON KEY error",JSONParseError);
        buzzerBadBeep();
        delay(5000);
    }
    
}

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
 *  Before calling this routine you must have true returned from
 *     nfc.readPassiveTargetID
 * 
 * return:  result code, as uint8_t, as follows:
 *  0 means factory fresh card
 *  1 means MN formatted card
 *  2 means neither type (sector most likely unusable)
 *  3 means card formatted with previous MN keys
****************************************************************************************************/
uint8_t testCard() {
    bool success = false;
    
    #ifdef TEST
        Serial.println("Trying to authenticate with default key A ....");
    #endif
    
    // xxx Would be better to test g_secretKeyA first, since that is the most common case
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
        success = authenticateBlock(2, 0, g_secretKeyA);        
        if(success == true) {   // we can assume an MN formatted card
            #ifdef TEST
                Serial.println("MN secret key A authenticated.  Assume NM formatted card ...");
            #endif
            
            return 1;   // code for MN formatted card

        } else { // not factory fresh or current MN keys 
            // reset by reading the target ID again
            nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
            success = authenticateBlock(2, 0, g_secretKeyA_OLD);        
            if(success == true) {   // we can assume an MN formatted card
                #ifdef TEST
                    Serial.println("MN secret key A old authenticated.  Assume old NM formatted card ...");
                #endif
            
                return 3;   // code for MN formatted card

            } else { // no test passed - card type is unknown
                #ifdef TEST
                    Serial.println("Not an MN formatted card - card type unknown ...");
                #endif
            
                return 2;   // code for unknown card format
            }
        }
    }
}   // end of testCard()


// ------------------ ReadCard ---------------
//
// Called from main loop to see if card is presented and readable.
//
// Params
//      msg1 and msg2 are displayed on the LCD while waiting for the
//          user to present their card
// 
// When this routine returns COMPLETE_OK the g_cardData should be 
// set, in particular if .clientID is not 0 then the main loop will
// assume g_cardData is good and try to checkin this client
//
enumRetStatus readTheCard(String msg1, String msg2) {

    enumRetStatus returnStatus = IN_PROCESS;

    //uint8_t success;
    uint8_t dataBlock0[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}; //  16 byte buffer to hold a block of data
    uint8_t dataBlock1[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}; //  16 byte buffer to hold a block of data

    //bool flag = false;  // toggle flag for testing purposes

    
    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    Serial.println("waiting for ISO14443A card to be presented to the reader ...");
    
    //writeToLCD("Place card on","reader");
    writeToLCD(msg1,msg2);

    if(!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // no card presented so we just exit
        return IN_PROCESS;
    }

    // we have a card presented
    digitalWrite(READY_LED,LOW);
    clearCardData();
    //writeToLCD("Reading Card","   Wait   ");
  
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

    switch (cardType) {
    case 0:
        // do nothing factory fresh card
        g_cardData.cardStatus = "Factory Fresh Card";
        g_cardData.isValid = true;
        returnStatus = COMPLETE_FAIL;
        Serial.println("\n\nCard is type factory fresh\n");
        logToDB("CardTypeFactoryFresh","",0,"");
        writeToLCD("Card is not MN","(fresh format)");
        buzzerBadBeep();
        delay(1000);
        break;
    
    case 1: {
        // MN formatted card
        #ifdef TEST
            Serial.println("\n\nCard is type Maker Nexus formatted\n");
        #endif
        
        String theClientID = "";
        String theUID = "";

        // now read the data using MN Key
        bool result0 = readBlockData(dataBlock0, 0,  0, g_secretKeyA);
        bool result1 = readBlockData(dataBlock1, 1,  0, g_secretKeyA);
        Serial.println("Remove card from reader ...");
        writeToLCD(" ","Remove card ...");

        #ifdef TEST
            Serial.println("The clientID data is:");
            nfc.PrintHex(dataBlock0, 16);
            Serial.println("");
            Serial.println("The MN UID data is:");
            nfc.PrintHex(dataBlock1, 16);
        #endif

        // Did we get a good read from each block?
        if (result0 && result1) {
            // good results, move data into g_cardData

            // Copy out block 0
            for (int i=0; i<16; i++) {
                theClientID = theClientID + String( (char) dataBlock0[i] ); 
            }

            // Copy out block 1    
            for (int i=0; i<16; i++) {
                if(dataBlock1[i] !=0) {
                    theUID = theUID + String( (char) dataBlock1[i] ); 
                } else {
                    break; // reached a null character
                }
            }

            g_cardData.clientID = theClientID.toInt();
            g_cardData.UID = theUID;
            g_cardData.cardStatus = "MN Format Card";
            g_cardData.isValid = true;
        }

        // display the status to the user
        String msg = "";
        String msg2 = "";

        if (!result0) {
            // error reading block 0 data 
            returnStatus = COMPLETE_FAIL;
            g_cardData.cardStatus = "Block 0 read failure";
            msg = "Card read failed";
            msg2 = "try again: 0";
            
        } else if (!result1) {
            // error reading block 1 data 
            returnStatus = COMPLETE_FAIL;
            g_cardData.cardStatus = "Block 1 read failure";
            msg = "Card read failed";
            msg2 = "try again :1";

        } else if (g_cardData.clientID == 0 ) {
            // got a bad clientID
            returnStatus = COMPLETE_FAIL;
            g_cardData.cardStatus = "clientID read as 0 - or not numeric";
            msg = "Card read failed";

        } else {
            returnStatus = COMPLETE_OK;
            msg = "";
            msg2 = "";
        }
        
        // If msg, then something went wrong
        if (msg.length() != 0) {
            writeToLCD(msg, msg2);
            buzzerBadBeep();
            delay(1000);
        } else {
            buzzerGoodBeep();
        }

        Serial.println(msg);
        Serial.println("");  
        break;
        }

    case 3:
        // old format of MN card
        g_cardData.cardStatus = "MN Old Format Card";
        g_cardData.isValid = true;
        returnStatus = COMPLETE_FAIL;
        Serial.println("\n\nCard is type Maker Nexus old formatted\n");
        logToDB("CardTypeOldMN","",0,"");
        writeToLCD("Card is old MN"," ");
        buzzerBadBeep();
        delay(1000);
        break;

    default:
        g_cardData.cardStatus = "Cannot Read Card";
        g_cardData.isValid = true;
        returnStatus = COMPLETE_FAIL;
        Serial.println("\n\nCard is type other card format\n");
        logToDB("CardTypeUnknown","",0,"");
        writeToLCD("Card is not MN","(unknown format)");
        buzzerBadBeep();
        delay(1000);
        break;
    }

    
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // card is on the reader, put up a message
        Serial.println("Remove card from reader ...");
        writeToLCD(" ","Remove card ...");
    }
    
    while(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // wait for card to be removed
    }
    
    return returnStatus;
    
}

// ------- resetCardToFreshNow ----
//
// If the card can be read with the current or old MN card, reset it to factory fresh format
//
void resetCardToFreshNow() {

    Serial.println("waiting for ISO14443A card to be presented to the reader ...");
    
    writeToLCD("Place card on","reader to RESET");

    unsigned long processStartMilliseconds = millis();
    while(!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // JUST WAIT FOR A CARD
        if (millis() - processStartMilliseconds > 15000) {
            writeToLCD ("Tired of waiting", " ");
            delay(500);
            buzzerGoodBeep();
            g_adminCommand = acIDLE;
            g_adminCommandData = "";
            return;
        }
    }
    
    // test the card to determine its type
    uint8_t cardType = testCard();
    debugEvent("card is type " + cardType);
    Serial.print("\n\nCard is type ");

    uint8_t thisCardWriteKey[6];
    switch (cardType) {
    case 0:
        Serial.println("already factory fresh\n");
        writeToLCD ("Already reset","remove card");
        delay (1000);
        g_adminCommand = acIDLE;
        g_adminCommandData = "";
        return;
        break;

    case 1:
        //mn format 
        for (int i=0; i<6; i++){
            thisCardWriteKey[i] = g_secretKeyB[i];
        }
        break;

    case 3:
        // mn old format
        for (int i=0; i<6; i++){
            thisCardWriteKey[i] = g_secretKeyB_OLD[i];
        }
        break;

    case 2:
    default:
        // unknown format         
        Serial.println("unknown format\n");
        writeToLCD ("Unknown format","can not reset");
        delay(1000);
        g_adminCommand = acIDLE;
        g_adminCommandData = "";
        return;
        break;       
    }

    // Get here to reset the card

    // change the keys to make this an MN card
    bool cardIsReady = changeKeys(1, thisCardWriteKey, DEFAULT_KEY_A, DEFAULT_KEY_B, DEFAULT_ACB);
    if (cardIsReady) {
        Serial.println("\nMade fresh card to MN card OK\n");
        writeToLCD("Card is fresh","now");
        buzzerGoodBeeps2();
    } else {
        writeToLCD("Failed change", "to MN type");
        buzzerBadBeep();
    }
    delay(1000);
    clearClientInfo();
    clearCardData();
    writeToLCD("Card Done","remove card");

    while(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // wait for card to be removed
    }

    g_adminCommand = acIDLE;
    g_adminCommandData = "";
    return;

}

// -------- burnCardNow ---------
// Called to create a new card in MN format.
//
// Params: 
//    clientID - As found in EZFacility for the user
//    cardUID - Ad found in the custom field in EZFacility for this client
//
void burnCardNow(int clientID, String cardUID) {
    // burn baby burn!
    // All this needs to happen in 10 seconds or the cloud function will time out
    unsigned long processStartMilliseconds = 0;
    
    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    Serial.println("waiting for ISO14443A card to be presented to the reader ...");
    
    writeToLCD("Place card on","reader to burn");
    
    processStartMilliseconds = millis();
    while(!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // JUST WAIT FOR A CARD
        if (millis() - processStartMilliseconds > 15000) {
            writeToLCD ("Tired of waiting", " ");
            delay(500);
            g_adminCommand = acIDLE;
            g_adminCommandData = "";
            return; // xxx
        }
    }
    
    // test the card to determine its type
    uint8_t cardType = testCard();
    debugEvent("card is type " + cardType);
    Serial.print("\n\nCard is type ");
    if(cardType == 0) {
         Serial.println("factory fresh\n");
        // change the keys to make this an MN card
        bool cardIsReady = changeKeys(0, DEFAULT_KEY_A, g_secretKeyA, g_secretKeyB, MN_SECURE_ACB);
        if (cardIsReady) {
            Serial.println("\nMade fresh card to MN card OK\n");
        } else {
            writeToLCD("Could not change", "to MN type");
            g_adminCommand = acIDLE;
            g_adminCommandData = "";
            return;
        }
    } else if (cardType == 1) {  // xxx should we have enum
        Serial.println("Maker Nexus formatted\n");        
    } else {
        Serial.println("other\n");
        writeToLCD("Unable to","use this card");
        buzzerBadBeep();
        g_adminCommand = acIDLE;
        g_adminCommandData = "";
        return;
    }

    // we now have an MN card 
    writeToLCD("Going to","burn card"); //xxx
    //delay(2000);
            
    // now write data to block 0 and 1 of the MN sector using secret key B
    uint8_t clientIDChar[16];
    for (int i=0; i<16; i++){
        clientIDChar[i] = String(clientID).c_str()[i];
    }

    uint8_t cardUIDChar[16];
    int temp = cardUID.length();
    for (int i=0; i<16; i++){
        if (i < temp){
            cardUIDChar[i] = cardUID.c_str()[i];
        } else {
            cardUIDChar[i] = 0;
        }
    }


    writeBlockData(clientIDChar, 0,  1, g_secretKeyB );
    writeBlockData(cardUIDChar, 1,  1, g_secretKeyB);
    /*
    uint8_t dataBlock[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}; //  16 byte buffer to hold a block of data
    // now read the data back using MN secret key A
    readBlockData(dataBlock, 0,  0, g_secretKeyA);
    Serial.println("The new block 0 data is:");
    nfc.PrintHex(dataBlock, 16);
    Serial.println("");
    
    readBlockData(dataBlock, 1,  0, g_secretKeyA);
    Serial.println("The new block 1 data is:");
    nfc.PrintHex(dataBlock, 16); 
    
    Serial.println("");
    */
    clearClientInfo();
    clearCardData();
    Serial.println("Remove card from reader ...");
    writeToLCD("Card Done","remove card");
    buzzerGoodBeeps2();

    while(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // wait for card to be removed
    }

    g_adminCommand = acIDLE;
    g_adminCommandData = "";

}