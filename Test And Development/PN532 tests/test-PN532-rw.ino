/*************************************************************************
 * test-PN532-rw.ino
 * 
 * This program is a basic test of integrating the Adafruit PN532
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
 * The program initially waits 5 seconds so that a tty port can be connected for
 * serial console communication.  A "trying to connect ..." message is then
 * printed to the serial port to show that things are ready.  Next, the Photon
 * communicates with the breakout board to read out and print the PN532 firmware
 * version data to show that the Photon is talking to the breakout board.  
 * 
 * After an RFID card is presented to the breakout board, the UID is read out
 * in loop() and the UID is printed (HEX value) and tested to see if it is a Classic
 * card, in which case the UID is decoded and the data is printed.
 * 
 * The program next reads out block 0 of sector 1 (block 4) and displays the data
 * from the card.  Uncommenting 2 lines of code allows some hardcoded data to be written
 * to this block and then the new data can be read back and verified.
 * 
 * Adapted from code at:
 *   https://github.com/adafruit/Adafruit-PN532/blob/master/examples
 * By: Bob Glicksman
 * (c) 2019; Team Practical Projects
 * 
 * version 1.0; 6/20/19
 * 
************************************************************************/

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_PN532.h>
 
#define IRQ_PIN D3
#define RST_PIN D4  // not connected

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
      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	  
	  // Start with block 4 (the first block of sector 1) since sector 0
	  // contains the manufacturer data and it's probably better just
	  // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);
	  
      if (success)
      {
        Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
        uint8_t data[16];
		
        // If you want to write something to block 4 to test with, uncomment
		// the following line and this text should be read back in a minute
       // memcpy(data, (const uint8_t[]){ 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0 }, sizeof data);
        //success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(4, data);
		
        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 4:");
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
      if (success)
      {
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