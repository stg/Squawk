/*
 * Remove Example
 *
 * This sketch shows how to use remove() to delete
 * the file created by the fat16append.pde example.
 */
#include "Fat16.h"
#include <Fat16util.h> // use functions to print strings from flash memory

SdCard card;
Fat16 file;

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char* str) {
  PgmPrint("error: ");
  SerialPrintln_P(str);
  if (card.errorCode) {
    PgmPrint("SD error: ");
    Serial.println(card.errorCode, HEX);
  }
  while(1);
}

void setup(void) {
  Serial.begin(9600);
  Serial.println();
  PgmPrintln("Type any character to start");
  while (!Serial.available());
  
  // initialize the SD card
  if (!card.init()) error("card.init");
  
  // initialize a FAT16 volume
  if (!Fat16::init(&card)) error("Fat16::init");
  
  char name[] = "APPEND.TXT";
  if (!file.open(name, O_WRITE)) {
    PgmPrint("Can't open "); 
    Serial.println(name);
    PgmPrintln("Run the append example to create the file.");
    error("open");
  }
  if (!file.remove()) error("file.remove");
  Serial.print(name);
  PgmPrintln(" deleted.");
}

void loop(void) {}
