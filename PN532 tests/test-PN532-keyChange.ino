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
 * The test will construct the desired trailer block contents but will
 * wwrite it to block 0 relative to the sector and read it back to make sure that
 * it is constructed properly.
 * 
 * Adapted from code at:
 *   https://github.com/adafruit/Adafruit-PN532/blob/master/examples
 * By: Bob Glicksman
 * (c) 2019; Team Practical Projects
 * 
 * version 1.0; 6/23/19
 * 
************************************************************************/

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_PN532.h>
 
#define IRQ_PIN D3
#define RST_PIN D4  // not connected

// Sector to use for testing
const int SECTOR = 2;	// Sector 0 is manufacturer data and sector 1 is real MN data

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


// instantiate in I2C mode   
Adafruit_PN532 nfc(IRQ_PIN, RST_PIN); 

 
void setup() {   
    pinMode(D3, INPUT);     // IRQ pin from PN532\
    pinMode(D4, OUTPUT);    // reserved for PN532 RST -- not used at this time
    
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
            
    Serial.print("Found chip PN5");
    Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. ");
    Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata>>8) & 0xFF, DEC);
    
    // configure board to read RFID tags
    nfc.SAMConfig();
    Serial.println("Waiting for an ISO14443A Card ...");
} 
 
void loop(void) {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
    if (success) {
        // Display some basic information about the card
        Serial.println("Found an ISO14443A card");
        Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);
        Serial.println("");
    
        if (uidLength == 4)
        {
        // We probably have a Mifare Classic card ... 
        Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
	  
        // Now we need to try to authenticate it for read/write access
        // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
        Serial.println("Trying to authenticate block 0 relative with default KEYA value");
	  
	    // compute blocks to test with based on block number relative to the sector
	    int block0 = SECTOR * 4;
	    int block1 = block0 +1;
    	int sectorTrailer = block0 + 3;

	    // authenticate relative block 0 using the default key A	
        success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, block0, 0, DEFAULT_KEY_A);
	  
        if (success) {
            Serial.print("Sector ");
            Serial.print(SECTOR);
            Serial.println(" has been authenticated");
        
            // assemble the 16 bytes of data to write
            uint8_t data[16];
            for (int i = 0; i < 6; i++) {	// key A in bytes 0 .. 5
            	data[i] = DEFAULT_KEY_A[i];
            }
            for (int i = 0; i < 4; i++) {	// ACB in bytes 6, 7, 8, 9
	            data[i + 6] = DEFAULT_ACB[i];
            }
            for (int i = 0; i < 6; i++) {	// key B in bytes 10 .. 15
	            data[i + 10] = DEFAULT_KEY_B[i];
            }
        
            // write the data to block 0 relative to SECTOR
	        success = nfc.mifareclassic_WriteDataBlock (block0, data);

            // Try to read the contents of block 0 relative to sector
            success = nfc.mifareclassic_ReadDataBlock(block0, data);
		
            if (success) {
                // Data seems to have been read ... spit it out
                Serial.println("Reading Block 0 relative to SECTOR:");
                nfc.PrintHexChar(data, 16);
                Serial.println("");
            
            // Wait a bit before reading the card again
            delay(1000);
            }
            // rewite block with some other data
            
            // assemble the 16 bytes of data to write
            for (int i = 0; i < 6; i++) {	// key A in bytes 0 .. 5
            	data[i] = MN_SECRET_KEY_A[i];
            }
            for (int i = 0; i < 4; i++) {	// ACB in bytes 6, 7, 8, 9
	            data[i + 6] = MN_SECURE_ACB[i];
            }
            for (int i = 0; i < 6; i++) {	// key B in bytes 10 .. 15
	            data[i + 10] = MN_SECRET_KEY_B[i];
            }
        
            // write the data to block 0 relative to SECTOR
	        success = nfc.mifareclassic_WriteDataBlock (block0, data);

            // Try to read the contents of block 0 relative to sector
            success = nfc.mifareclassic_ReadDataBlock(block0, data);
		
            if (success) {
                // Data seems to have been read ... spit it out
                Serial.println("Reading Block 0 relative to SECTOR:");
                nfc.PrintHexChar(data, 16);
                Serial.println("");
		  
                // Wait a bit before reading the card again
                delay(1000);
            }
            else
            {
                Serial.println("Ooops ... unable to read the requested block.  Try another key?");
            }
        }
        else
        {
            Serial.println("Ooops ... authentication failed: Try another key?");
        }
    }
    
    if (uidLength == 7)
    {
        // We probably have a Mifare Ultralight card ...
        Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");
	  
        // Try to read the first general-purpose user page (#4)
        Serial.println("Reading page 4");
        uint8_t data[32];
        success = nfc.mifareultralight_ReadPage (4, data);
        if (success) {
            // Data seems to have been read ... spit it out
            nfc.PrintHexChar(data, 4);
            Serial.println("");
		
            // Wait a bit before reading the card again
            delay(1000);
        }
        else
        {
            Serial.println("Ooops ... unable to read the requested page!?");
        }
    }
  }
}