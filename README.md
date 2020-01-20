# MN_ACL
## RFID based access control and monitoring system for Maker Nexus.
This repository contains hardware, software and test information 
related to the development of an access control system for
Maker Nexus.

Each member is issued an RFID card that has been prepared to identify the member
and to ensure that cards can be revoked and/or replaced.  Membership information relevant
to card issuance and to member check-in and permission to access certain equipment
is supplied via real-time
query of the Maker Nexus CRM system which uses EZ Facility.  The code to access EZ Facility
via their REST API has been broken out into separate functions so that it can be replaced
by any suitable database.  Administrative utilities are also provided to facilitate recycling
membership RFID cards and identifying the owner of misplaced RFID cards.

A "Facility Database", separate from EZ Facility, is included in this project.  EZ Facility is the
authoritative source for all membership status, payment and training information needed for access
control decisions.  The Facility Database provides membership access records for both a real-time 
status display and for off-line reporting on facility and equipment utilization.

This project uses a Particle Argon; however, a Xenon should also work in a Particle Mesh configuration.  
The access control system is based upon Mifare Classic 1K RFID cards that are written to/read from
using a PN532-based MFC/RFID breakout board.  The Adafruit PN532 library is used to supply
the underlying communication support between the Particle device and the RFID card. Mifare Classic 1K 
cards have been chosen because they are inexpensive and widely available on the Internet.
These cards are older technology and not very secure.  However, we have deemed them to be secure 
enough for this application.  One sector of the card is chosen for this application and the sector encryption
keys and block access control bits are changed to make the card data secure from casual inspection and
from cloning.  The
remaining sectors of the card can be used for other applications, e.g. vending machine credits or special room
access.

A detailed technical overview of this project is provided in Documents/RFID_ACS_Overview_Document.pdf.  
We strongly encourage you to read through his document before deciding how this project might be
applied to your unique access control and monitoring situation.


## Files and Folders:
README.md:  This document.
Repository_Overview.pdf:  An overview of the structure and contents of this repository.
Terms_Of_Use_License_and_Disclaimer.pdf:  You MUST READ this document and AGREE TO 
its terms and conditions before you are authorized to use the material in this repository. This
project is released under and open source, non-commercial license.
### Documents (folder):
Project documentation, including the RFID_ACS_Overview_Document and build, install and
usage documents.
### Hardware (folder): 
Design information (CAD files) and documentation for all of the hardware used
in this project, including electronic design, printed circuit board design, and
mechanical/packaging design and build information.
### Software (folder):
Design information, documentation, source and executable code for all of the software
in this project, including Particle (Argon) software, webhooks, PHP files, database
schema and stored procedures, Android app source and executable files, and web/html
files.
### Test and Development (folder):
Legacy test code and test results for integrating RFID cards and EZ Facility API calls with 
Particle devices.  The information in this folder is not necessary to build or install
this project.  However, it may be useful for developing extent ions to the RFID and/or EZ Facility
capabilities of this project.







