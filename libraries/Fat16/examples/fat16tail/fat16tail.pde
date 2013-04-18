/*
 * This sketch reads and prints the tail of all files
 * created by fat16append.pde, fat16print.pde and fat16write.pde.
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
  PgmPrintln("type any character to start");
  while (!Serial.available());
  Serial.println();
  
  // initialize the SD card
  if (!card.init()) error("card.init");
  
  // initialize a FAT16 volume
  if (!Fat16::init(&card)) error("Fat16::init");
}
uint16_t index = 0;
/*
 * Print tail of Fat16 example files
 */
void loop(void) {
  dir_t dir;
  // read next directory entry
  if (!Fat16::readDir(&dir, &index)) {
    PgmPrintln("End of Directory");
    // halt
    while(1);
  }
  // remember found file and advance index for next dir
  uint16_t found = index++;
  
  // check for file name "APPEND*.TXT", "PRINT*.TXT", or "WRITE*.TXT"
  // first 8 bytes are blank filled name
  // last three bytes are blank filled extension
  if ((strncmp((char *)dir.name, "APPEND", 6) &&
       strncmp((char *)dir.name, "WRITE", 5) &&
       strncmp((char *)dir.name, "PRINT", 5)) ||
       strncmp((char *)&dir.name[8], "TXT", 3)) {
         return;
  }
  // open file by index - easier to use than open by name.
  if (!file.open(found, O_READ)) error("file.open");
  
  // print file name message
  PgmPrint("Tail of: ");
  for(uint8_t i = 0; i < 11; i++){
    if(dir.name[i] == ' ') continue;
    if (i == 8) Serial.write('.');
    Serial.write(dir.name[i]);
  }
  Serial.println();
  
  // position to tail of file
  if (file.fileSize() > 100) {
    if (!file.seekSet(file.fileSize() - 100)) error("file.seekSet");
  }
  int16_t c;
  // find end of line  
  while ((c = file.read()) > 0 && c != '\n');
  
  // print rest of file
  while ((c = file.read()) > 0) Serial.write((char)c);
  file.close();
  Serial.println();
}
