/*
 * Append Example
 *
 * This sketch shows how to use open for append and the Arduino Print class
 * with Fat16.
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
  
  char name[] = "APPEND.TXT";
  PgmPrint("Appending to: ");
  Serial.println(name);
  
  // clear write error
  file.writeError = false;
  
  for (uint8_t i = 0; i < 100; i++) {
    // O_CREAT - create the file if it does not exist
    // O_APPEND - seek to the end of the file prior to each write
    // O_WRITE - open for write
    if (!file.open(name, O_CREAT | O_APPEND | O_WRITE)) error("open");
    // print 100 lines to file
    for (uint8_t j = 0; j < 100; j++) {
      file.print("line ");
      file.print(j, DEC);
      file.print(" of pass ");
      file.print(i, DEC);
      file.print(" millis = ");
      file.println(millis());
      if (file.writeError) error("write");
    }
    if (!file.close()) error("close");
    if (i > 0 && i%25 == 0)Serial.println();
    Serial.write('.');
  }
  Serial.println();
  Serial.println("Done");
}
void loop(void){}
