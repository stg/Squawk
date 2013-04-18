/*
 * This sketch reads and prints the file
 * PRINT00.TXT created by fat16print.pde or
 * WRITE00.TXT created by fat16write.pde
 */
#include <Fat16.h>
#include <Fat16util.h> // use functions to print strings from flash memory

SdCard card;
Fat16 file;

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char*  str) {
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
  PgmPrintln("type any character to start");
  while (!Serial.available());
  Serial.println();
  
  // initialize the SD card
  if (!card.init()) error("card.init");
  
  // initialize a FAT16 volume
  if (!Fat16::init(&card)) error("Fat16::init");
  
  // open a file
  if (file.open("PRINT00.TXT", O_READ)) {
    PgmPrintln("Opened PRINT00.TXT");
  } else if (file.open("WRITE00.TXT", O_READ)) {
    PgmPrintln("Opened WRITE00.TXT");    
  } else{
    error("file.open");
  }
  Serial.println();
  
  // copy file to serial port
  int16_t n;
  uint8_t buf[7];// nothing special about 7, just a lucky number.
  while ((n = file.read(buf, sizeof(buf))) > 0) {
    for (uint8_t i = 0; i < n; i++) Serial.write(buf[i]);
  }
  /* easier way
  int16_t c;
  while ((c = file.read()) > 0) Serial.write((char)c);
  */
  PgmPrintln("\nDone");
}

void loop(void) {}
