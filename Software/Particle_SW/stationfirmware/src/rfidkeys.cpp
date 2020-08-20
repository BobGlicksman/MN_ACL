//
//  this file holds the secret keys you use to encrypt and read your RFID cards
//
//  DO NOT CHECK IN THIS FILE TO ANY PUBLIC REPOSITORY WITH YOUR KEYS IN IT
//
//  Replace the text blocks below with the six byte values you use for your keys.
//  Of the form: [160,141,167,49,73,21]
//
#include "mnutils.h"

#ifndef MN_PRODUCTION_COMPILE  //this covers the entire file. variable defined in mnutils.h
// NOT production, so compile this

#include "application.h"

String RFIDKeysJSON = "{\"WriteKey\":[100,101,102,103,104,105],\"ReadKey\":[200,201,202,203,205,205] }";

// secret used to validate checkin event
int checkinEventSecret = 12345;   // Make this unique and secret to your installation

#endif