// This file holds rfid utilities that are used by modules in the RFID project
// of Maker Nexus
//
// (CC BY-NC-SA 3.0 US) 2019 Maker Nexus   https://creativecommons.org/licenses/by-nc-sa/3.0/us/
//
#ifndef MNRFIDUTILS_H
#define MNRFIDUTILS_H

#include "application.h"

// xxx these should be set via a function and removed from global scope
extern uint8_t g_secretKeyA[6]; 
extern uint8_t g_secretKeyB[6];  
extern uint8_t g_secretKeyA_OLD[6]; 
extern uint8_t g_secretKeyB_OLD[6];  
extern bool g_secretKeysValid;

extern struct struct_cardData {
    int clientID;
    String UID;
    String cardStatus;
    bool isValid;
} g_cardData;

/**************************
 * mnrfidInit();
 * called once by programs using this module
 * 
 */
void mnrfidInit();

// ---------------- clearCardData -----------
void clearCardData();

// ------------- GET RFID KEYS -----------------
// Get RFID Keys
void responseRFIDKeys(const char *event, const char *data);


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
void createTrailerBlock(uint8_t *keya, uint8_t *keyb, uint8_t *acb, uint8_t *data);

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
uint8_t writeTrailerBlock(int sector, uint8_t *data);


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
uint8_t authenticateBlock(int blockNum, uint8_t keyNum, uint8_t *key);
 


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
 bool readBlockData(uint8_t *data, int blockNum,  uint8_t keyNum, uint8_t *key);
 

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
 bool writeBlockData(uint8_t *data, int blockNum,  uint8_t keyNum, uint8_t *key);

 



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
 bool changeKeys(uint8_t oldKeyNum, uint8_t *oldKey, uint8_t *newKeyA, uint8_t *newKeyB, uint8_t *newACB);


 
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
 *  3 means card formatted with previous MN keys
****************************************************************************************************/
uint8_t testCard();    


// ------------------ ReadCard ---------------
//
// Called from main loop to see if card is presented and readable.
// 
// When this routine returns COMPLETE_OK the g_cardData should be 
// set, in particular if .clientID is not 0 then the main loop will
// try to checkin this client
//

enumRetStatus readTheCard(String msg1, String msg2);



// If the card can be read with the current or old MN card, reset it to factory fresh format
void resetCardToFreshNow();



void burnCardNow(int clientID, String cardUID);



#endif