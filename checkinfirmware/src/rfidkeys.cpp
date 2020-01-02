//
//  this file holds the secret keys you use to encrypt and read your RFID cards
//
//  DO NOT CHECK IN THIS FILE TO ANY PUBLIC REPOSITORY WITH YOUR KEYS IN IT
//
//  Replace the text blocks below with the six byte values you use for your keys.
//  Of the form: [160,141,167,49,73,21]
//
#include "rfidkeys.h"
#include "application.h"

// keys used to write and read a card
String RFIDKeysJSON = "{\"WriteKey\":[91,127,197,134,179,14],\"ReadKey\":[29,97,96,199,174,204],\"WriteKeyOld\":[160,161,162,163,164,165],\"ReadKeyOld\":[176,177,178,179,180,181] }";
