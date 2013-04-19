/*
 * Copy Example - only runs on chips with 2K or more RAM
 *
 * This sketch copies the file APPEND.TXT, created by the
 * fat16append.pde example, to the file ACOPY.TXT.
 */
#include <Fat16.h>
#include <Fat16util.h> // use functions to print strings from flash memory

// large buffer to test for bugs. 512 bytes runs much faster.
char buf[600];

SdCard card;
Fat16 from; // read file
Fat16 copy; // write file

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
  
  PgmPrint("FreeRam: ");
  Serial.println(FreeRam());
  
  // initialize the SD card
  if (!card.init()) error("card.init");
  
  // initialize a FAT16 volume
  if (!Fat16::init(&card)) error("Fat16::init");
  
  strcpy_P(buf, PSTR("APPEND.TXT"));
  // open for read
  if (!from.open(buf, O_READ)) {
    PgmPrint("Can't open "); 
    Serial.println(buf);
    PgmPrintln("Run the append example to create the file.");
    error("from.open");
  }
  strcpy_P(buf, PSTR("ACOPY.TXT"));
  // create if needed, truncate to zero length, open for write
  if (!copy.open(buf, O_CREAT | O_TRUNC | O_WRITE)) {
    error("copy.open"); 
  }
  // count for printing periods
  uint16_t p = 0;
  int16_t n;  
  while ((n = from.read(buf, sizeof(buf))) > 0) {
    if (copy.write(buf, n) != n) error("write");
    // print progress periods
    if (!(p++ % 25)) Serial.write('.');
    if (!(p % 500)) Serial.println();
  }
  Serial.println();
  if (n != 0) error ("read");
  // force write of directory entry and last data
  if (!copy.close()) error("close copy");
  PgmPrintln("Copy done.");
}

void loop(void) {}
