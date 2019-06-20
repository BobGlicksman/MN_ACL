/*************************************************************************
 * test-PN532.ino
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
 *  The breakout board jumpers are set for I2C as follows:  SEL 1 is OFF and SEL 0 is ON
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
 * Version 1.1: added a 1 second delay to the end of loop() to allow the serial buffer to flush out.
 * 
 * Adapted from code at:
 *  https://classes.engineering.wustl.edu/ese205/core/index.php?title=Particle_photon_setup_with_PN532_NFC_board
 * By: Bob Glicksman
 * (c) 2019; Team Practical Projects
 * 
 * version 1.0; 6/19/19
 * version 1.1; 6/20/19
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
 
void loop() {
    uint8_t success = 0;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };    // Buffer to store the returned UID
    uint8_t uidLength = 0;  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength); 
 
    if (success) {
        // Display some basic information about the card
        Serial.println("Found an ISO14443A card");
        Serial.print("  UID Length: ");
        Serial.print(uidLength, DEC);
        Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);
        
        if (uidLength == 4) {
            uint32_t cardid = uid[0];
            cardid <<= 8;
            cardid |= uid[1];
            cardid <<= 8;
            cardid |= uid[2];
            cardid <<= 8;
            cardid |= uid[3];
            Serial.print("Mifare Classic card #");
            Serial.println(cardid);
        }
        
        Serial.println("");
    }
    delay(1000); // wait for serial buffer to flush out
}

