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
 * version 1.2; 6/25/19.  Eliminate UID length tests and messages
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
    uint8_t dataBlock[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //  16 byte buffer to hold a block of data

    
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

    // start the tests here
    Serial.println("Trying to authenticate with default key A ....");
    // try to authenticate blocks 0 and 1 (relative to SECTOR) with default key A
    authenticateBlock(0, 0, DEFAULT_KEY_A);
    authenticateBlock(1, 0, DEFAULT_KEY_A); 
        
    Serial.println("Trying to authenticate with default key B ....");
    // try to authenticate blocks 0 and 1 (relative to SECTOR) with default key B
    authenticateBlock(0, 1, DEFAULT_KEY_B);
    authenticateBlock(1, 1, DEFAULT_KEY_B);
        
    Serial.println("Trying to authenticate with MN secret key A ....");
    // try to authenticate blocks 0 and 1 (relative to SECTOR) with MN secret key A
    authenticateBlock(0, 0, MN_SECRET_KEY_A);
    authenticateBlock(1, 0, MN_SECRET_KEY_A);
        
    Serial.println("Trying to authenticate with MN secret key B ....");
    // try to authenticate blocks 0 and 1 (relative to SECTOR) with MN secret key B
    authenticateBlock(0, 1, MN_SECRET_KEY_B);
    authenticateBlock(1, 1, MN_SECRET_KEY_B);
        
    // conclude tests - terminate loop here
    Serial.println("..... end of test run ....\n\n");
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