/*************************************************************************
 * test-PN532-keyChange.ino
 * 
 * This program is a test of changing encryption keys and access control bits
 * on a Mifare Classic 1K card using the Adafruit PN532
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
 * (c) 2019; Team Practical Projects
 * 
 * Author: Bob Glicksman
 * version 2.2; 7/15/19.
 * 
 * v2.2: added LCD display 
 * 
************************************************************************/

//#define TEST     // uncomment for debugging mode

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_PN532.h>

// This #include statement was automatically added by the Particle IDE.
#include <LiquidCrystal.h>

 
#define IRQ_PIN D3
#define RST_PIN D4  // not connected
#define LED_PIN D7

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

 
void setup() {   
    pinMode(IRQ_PIN, INPUT);     // IRQ pin from PN532\
    pinMode(RST_PIN, OUTPUT);    // reserved for PN532 RST -- not used at this time
    pinMode(LED_PIN, OUTPUT);   // the D7 LED
    
    lcd.begin(16,2);
    lcd.clear();
    
    delay(5000);    // delay to get putty up
    Serial.println("trying to connect ....");

    
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
} // end of setup()
 
void loop(void) {
    uint8_t success;
    uint8_t dataBlock[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}; //  16 byte buffer to hold a block of data
    bool flag = false;  // toggle flag for testing purposes

    
    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    Serial.println("waiting for ISO14443A card to be presented to the reader ...");
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Place card on");
    lcd.setCursor(0,1);
    lcd.print("reader");
    
    while(!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // JUST WAIT FOR A CARD
    }
  

    #ifdef TEST
        // Display some basic information about the card
        Serial.println("Found an ISO14443A card");
        Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);
        Serial.println("");
        delay(1000);
    #endif

// START THE TESTS HERE
    
    // test the card to determine its type
    uint8_t cardType = testCard();
    Serial.print("\n\nCard is type ");
    if(cardType == 0) {
         Serial.println("factory fresh\n");
    } else if (cardType == 1) {
         Serial.println("Maker Nexus formatted\n");        
    } else {
        Serial.println("other\n");
    }

    // now reverse the keys and try writing/reading some test data
    if(cardType == 0) { // factory fresh card
        // change the keys to make this an MN card
        bool OK = changeKeys(0, DEFAULT_KEY_A, MN_SECRET_KEY_A, MN_SECRET_KEY_B, MN_SECURE_ACB);
        
        if(OK == true) {
            Serial.println("\nMade fresh card to MN card OK\n");
            
            // now write test data to block 0 and 1 of the MN sector using secret key B
            writeBlockData(TEST_PAT_1, 0,  1, MN_SECRET_KEY_B);
            writeBlockData(TEST_PAT_2, 1,  1, MN_SECRET_KEY_B);
            
            // now read the data back using MN secret key A
            readBlockData(dataBlock, 0,  0, MN_SECRET_KEY_A);
            Serial.println("The new block 0 data is:");
            nfc.PrintHex(dataBlock, 16);
            Serial.println("");
            
            readBlockData(dataBlock, 1,  0, MN_SECRET_KEY_A);
            Serial.println("The new block 1 data is:");
            nfc.PrintHex(dataBlock, 16); 
            
            Serial.println("");
             
            
        } else {
            Serial.println("\nFailed to make fresh card into MN card\n");           
        }
        
    } else  {  // MN formatted card
        // change the keys to make this a factory fresh card
        bool OK = changeKeys(1, MN_SECRET_KEY_B, DEFAULT_KEY_A, DEFAULT_KEY_B, DEFAULT_ACB);
        
        if(OK == true) {
            Serial.println("\nMade MN card into fresh card OK\n");
            
            uint8_t factoryData[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            
            // now write factory data to block 0 and 1 of the MN sector using factory key A
            writeBlockData(factoryData, 0,  0, DEFAULT_KEY_A);
            writeBlockData(factoryData, 1,  0, DEFAULT_KEY_A);
            
            // now read the data back using factory default key A
            readBlockData(dataBlock, 0,  0, DEFAULT_KEY_A);
            Serial.println("The new block 0 data is:");
            nfc.PrintHex(dataBlock, 16);
            Serial.println("");
            
            readBlockData(dataBlock, 1,  0, DEFAULT_KEY_A);
            Serial.println("The new block 1 data is:");
            nfc.PrintHex(dataBlock, 16); 
            
            Serial.println("");
            
            
        } else {
            Serial.println("\nFailed to make fresh card into MN card\n");           
        }
    }
    
    lcd.clear();
    lcd.setCursor(0,0);
    if(cardType == 0) {
       lcd.print("Card is MN");
    } else {
       lcd.print("Card is reset");
    }
    
    Serial.println("Remove card from reader ...");
    lcd.setCursor(0,1);
    lcd.print("Remove card ...");
    
    while(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // wait for card to be removed
    }
    
}   // end of loop()

/*************** FUNCTIONS FOR USE IN REAL APPLICATIONS *******************************/

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
    uint32_t absoluteBlockNumber = (SECTOR * 4) + 3;
    
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
        

