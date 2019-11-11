//
//  this file holds the secret keys you use to encrypt and read your RFID cards
//
//  DO NOT CHECK IN THIS FILE TO ANY PUBLIC REPOSITORY WITH YOUR KEYS IN IT
//
//  Replace the text blocks below with the six byte values you use for your keys.
//  Of the form: [160,141,167,49,73,21]
//
#include "application.h"

String RFIDKeysJSON = "{\"WriteKey\":[100,101,102,103,104,105],\"ReadKey\":[200,201,202,203,205,205] }";