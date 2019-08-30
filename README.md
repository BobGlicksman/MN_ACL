# MN_ACL
## RFID based access control and monitoring system for Maker Nexus
This repository contains hardware, software and test information 
related to the development of an access control system for
Maker Nexus.

Each member is issued an RFID card that has been prepared to identify the member
and to ensure that cards can be revoked and/or replaced.  Membership information relevant
to card issuance and to member check-in is supplied via real-time
query of the Maker Nexus CRM system which uses EZ Facility.  The code to access EZ Facility
via their REST API has been broken out into separate functions so that it can be replaced
by any suitable database.  Administrative utilities are also provided to facilitate recycling
membership RFID cards and identifying the owner of misplaced RFID cards.

This project uses a Particle Argon; however, a Xenon should also work in a Particle Mesh configuration.  
The access control system is based upon Mifare Classic 1K RFID cards that are written to/read from
using a PN532-based MFC/RFID breakout board.  The Adafruit PN532 library is used to supply
the underlying communication support betwen the Particle device and the RFID card. Mifare Classic 1K 
cards have been chosen because they are inexpensive and widely available on the Internet.
These cards are older technology and not very secure.  However, we have deemed them to be secure 
enough for this application.  One sector of the card is chosen for this application and the sector encryption
keys and block access control bits are changed to make the card data secure from casual inspection.  The
remaining sectors of the card can be used for other applications, e.g. vending machine credits or special room
access.


## Folders:
### PN532 tests:  
basic tests of the integration with the breakout board
for Maker Nexus ACL application.  All tests are performed using the
Adafruit PN532 library that has been ported over to Particle. Others have
reported successfully using this library with Particle Photon, Electron,
Argon and Xenon.
### EZF tests: 
basic tests of webhook integration from Particle cloud to
Maker Nexus EZFacility account.  [Nothing here at present].
### Hardware: 
hardware design (schematic and Eagle board file), CAD diagrams, and parts list
are included in this folder.
### checkinfirmware: 
source code for Particle firmware for card reading and check-in/access control
functions.






