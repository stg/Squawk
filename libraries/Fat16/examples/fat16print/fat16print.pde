/*
 * Print Example
 *
 * This sketch shows how to use the Arduino Print class with Fat16.
 */
#include <Fat16.h>
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
  
  // create a new file
  char name[] = "PRINT00.TXT";
  for (uint8_t i = 0; i < 100; i++) {
    name[5] = i/10 + '0';
    name[6] = i%10 + '0';
    // O_CREAT - create the file if it does not exist
    // O_EXCL - fail if the file exists
    // O_WRITE - open for write
    if (file.open(name, O_CREAT | O_EXCL | O_WRITE)) break;
  }
  if (!file.isOpen()) error ("create");
  PgmPrint("Printing to: ");
  Serial.println(name);
  
  // clear write error
  file.writeError = false;
  
  // print 100 line to file
  for (uint8_t i = 0; i < 100; i++) {
    file.print("line ");
    file.print(i, DEC);
    file.print(" millis = ");
    file.println(millis());
  }
  // force write of all data to the SD card
  if (file.writeError || !file.sync()) error ("print or sync");
  PgmPrint("Done");
}
void loop(void){}
