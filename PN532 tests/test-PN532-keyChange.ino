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
 * This first test will not change encrytion keys nor access control bits.
 * The test will just try to authenticate blocks 0 and 1 relative to the designated
 * sector using both the default keys and the MN secret keys.
 * 
 * Adapted from code at:
 *   https://github.com/adafruit/Adafruit-PN532/blob/master/examples
 * By: Bob Glicksman
 * (c) 2019; Team Practical Projects
 * 
 * version 1.3; 6/26/19.
 * 
************************************************************************/

#define DEBUG   // uncomment for debugging mode

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_PN532.h>
 
#define IRQ_PIN D3
#define RST_PIN D4  // not connected
#define LED_PIN D7

// Sector to use for testing
const int SECTOR = 4;	// Sector 0 is manufacturer data and sector 1 is real MN data

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

 
void setup() {   
    pinMode(IRQ_PIN, INPUT);     // IRQ pin from PN532\
    pinMode(RST_PIN, OUTPUT);    // reserved for PN532 RST -- not used at this time
    pinMode(LED_PIN, OUTPUT);   // the D7 LED
    
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
    
/*    Serial.print("Found chip PN5");
    Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. ");
    Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata>>8) & 0xFF, DEC);
*/    
    // configure board to read RFID tags
    nfc.SAMConfig();
    Serial.println("Waiting for an ISO14443A Card ...");
    
    
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

    
    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate

    Serial.println("waiting for ISO14443A card to be presented to the reader ...");
    while(!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        // JUST WAIT FOR A CARD
    }
  

    #ifdef DEBUG
        // Display some basic information about the card
        Serial.println("Found an ISO14443A card");
        Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);
        Serial.println("");
        delay(1000);
    #endif

// START THE TESTS HERE
/*    
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
*/
uint8_t cardType = 0;   // for debugging
    // read the block 0 (relative) data using key A according to the card type
    if(cardType == 0) { // factory fresh card -- use the default key A to read
        #ifdef DEBUG
            Serial.println("Reading data using default key A");
        #endif
        bool OK =  readBlockData(dataBlock, 0, 0, DEFAULT_KEY_A);
        if(OK == true) {    // successful read
            #ifdef DEBUG
                Serial.println("data read OK.");
            #endif
            nfc.PrintHexChar(dataBlock, 16);    // print the data
        } else {
            Serial.println("data read failed! ..");
        }
        
    } else if(cardType == 1) {  // MN formatted card -- use the MN secret key A to read
        #ifdef DEBUG
            Serial.println("Reading data using MN secret key A");
        #endif
        bool OK =  readBlockData(dataBlock, 0, 0, MN_SECRET_KEY_A);
        if(OK == true) {    // successful read
            #ifdef DEBUG
                Serial.println("data read OK.");
            #endif
            nfc.PrintHexChar(dataBlock, 16);    // print the data
        } else {
            Serial.println("data read failed! ..");
        }
    
    } else {    // not a recognized card type - cannot read data
        Serial.println("card is not a recognized type.");
    }
   
    delay(10000); // delay 10 seconds to record the data
    
}   // end of loop()

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
    // check that the relative block number is 0, 1 or 2 only
    if(blockNum > 2) {
        #ifdef DEBUG
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
        #ifdef DEBUG
            Serial.println("bad key number.  Must be 0 or 1");
            delay(1000);
        #endif
        return 0;   // return with error indication
    }
    
    // key is valid
    #ifdef DEBUG
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
    
    #ifdef DEBUG
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
 *  to trying to read data.
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
    
    // compute the absolute block number:  (SECTOR * 4) + relative block number
    uint32_t absoluteBlockNumber = (SECTOR * 4) + blockNum;
    
    // first need to authenticate the block with a key
    authOK = authenticateBlock(blockNum, keyNum, key);
     
    if (authOK == true) {  // Block is authenticated so we can read the data
        readOK = nfc.mifareclassic_ReadDataBlock(absoluteBlockNumber, data);
        
        if(readOK == true) {
            #ifdef DEBUG
                Serial.println("\nData read OK\n");
                delay(1000);
            #endif 
            return true;   // successful read
        } else {
            #ifdef DEBUG
                Serial.println("\nData read failed!\n");
                delay(1000);
            #endif 
            return false;   // failed  read            
        }
        
    } else {
        #ifdef DEBUG
            Serial.println("\nBlock authentication failed!\n");
            delay(1000);
        #endif
        return false;
    }

 }  // end of readBlockData()

/***************************************************************************************************
 * testCard():  Tests an ISO14443A card to see if it is factory fresh, MN formatted, or other.
 *  The test is performed by authenticating relative block 0 on the designated sector (nominally
 *  sector 1).  If the block authenticates with both default key A and default key B then the card
 *  is declared to be factory fresh.  If the block authenticates with both MN secret key A and
 *  MN secret key B, then the card is declared to be MN formatted.  Otherwise, the card is 
 *  declared to be "other" and the designed sector is likely unusable.
 * 
 * return:  result code, as uint8_t, as follows:
 *  0 means factory fresh card
 *  1 means MN formatted card
 *  2 means neither type (sector most likely unusable)
****************************************************************************************************/
uint8_t testCard() {
    bool dKeyA = false;
    bool dKeyB = false;
    bool MNkeyA = false;
    bool MNkeyB = false;
    
    #ifdef DEBUG
        Serial.println("Trying to authenticate with default key A ....");
    #endif
    dKeyA = authenticateBlock(0, 0, DEFAULT_KEY_A);
        
    #ifdef DEBUG
        Serial.println("Trying to authenticate with default key B ....");
    #endif
    dKeyB = authenticateBlock(0, 1, DEFAULT_KEY_B);

    #ifdef DEBUG    
        Serial.println("Trying to authenticate with MN secret key A ....");
    #endif
    MNkeyA = authenticateBlock(1, 0, MN_SECRET_KEY_A);
        
    #ifdef DEBUG
        Serial.println("Trying to authenticate with MN secret key B ....");
    #endif
    MNkeyB = authenticateBlock(0, 1, MN_SECRET_KEY_B);

    #ifdef DEBUG
        if(dKeyA == true) {
            Serial.println("default key A authenticated ...");
        } else {
            Serial.println("default key A failed ...");
        }
        
        if(dKeyB == true) {
            Serial.println("default key B authenticated ...");
        } else {
            Serial.println("default key B failed ...");
        }
        
        if(MNkeyA == true) {
            Serial.println("MN secret key A authenticated ...");
        } else {
            Serial.println("MN secret key A failed ...");
        }
        
        if(MNkeyB == true) {
            Serial.println("MN secret key B authenticated ...");
        } else {
            Serial.println("MN secret key B failed ...");
        }
        Serial.println("");
        delay(1000);
    #endif
    
    if( (dKeyA == true) && (dKeyB == true) ) {
        return 0;   // card is factory fresh
    }
    
    if( (MNkeyA == true) && (MNkeyB == true) ) {
        return 1;   // card is MN formatted
    }
    
    return 2;   // card is other
}   // end of cardTest()

