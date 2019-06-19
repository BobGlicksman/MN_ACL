# MN_ACL
RFID based access control and monitoring system for Maker Nexus
##This repository contains testing information, code and results for
tests related to the development of an access control system for
Maker Nexus.
##These tests use a Particle Photon.  The Photon is connected to an
Adafruit PN532 RFID breakout board (https://www.adafruit.com/product/364)
using an I2C interface, as follows:
###Photon D0 --> breakout SDA with 5.6K pullup to Photon 3.3 volts
###Photon D1 --> breakout SCL with 5.6K pullup to Photon 3.3 volts
###Photon D3 --> breakout IRQ (no pullup)
###Photon D4 --> unconnected; reserved for RST
###Breakout board jumpers for I2C:  SEL 1 is OFF; SEL 0 is ON
###Breakout board +5 volts is connected to Photon VIN
###Breakout board GND and both Photon GNDs are all connected
##Folders:
###PN532 tests:  basic tests of the integration with the breakout board
for Maker Nexus ACL application.  All tests are performed using the
Adafruit PN532 libaray that has been ported over to Particle. Others have
reported successfully using this library with Particle Photon, Electron,
Argon and Xenon.
###EZF tests: basic tests of webhook integration from Particle cloud to
Maker Nexus EZFacility account.  [Noting here at present].






