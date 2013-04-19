/*
 * This sketch tests the dateTimeCallback() function
 * and the timestamp() function.
 */
#include <Fat16.h>
#include <Fat16util.h> // use PgmPrint
/*
 * date/time values for debug
 * normally supplied by a real-time clock or GPS
 */
// date 2009-10-01  1-Oct-09
uint16_t year = 2009;
uint8_t month = 10;
uint8_t day = 1;

// time 20:30:40
uint8_t hour = 20;
uint8_t minute = 30;
uint8_t second = 40;  
/*
 * User provided date time callback function.
 * See Fat16::dateTimeCallback() for usage.
 */
void dateTime(uint16_t* date, uint16_t* time) {
  // User gets date and time from GPS or real-time
  // clock in real callback function
  
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(year, month, day);
  
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(hour, minute, second);
}

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
/*
 * Function to print all timestamps.
 */
void printTimestamps(Fat16& f) {
  dir_t d;
  if (!f.dirEntry(&d)) error("dirEntry");
  
  PgmPrint("Creation: ");
  f.printFatDate(d.creationDate);
  Serial.write(' ');
  f.printFatTime(d.creationTime);
  Serial.println();
  
  PgmPrint("Modify: ");
  f.printFatDate(d.lastWriteDate);
  Serial.write(' ');
  f.printFatTime(d.lastWriteTime);
  Serial.println();  
  
  PgmPrint("Access: ");
  f.printFatDate(d.lastAccessDate);
  Serial.println();
}

void setup(void) {
  Serial.begin(9600);
  Serial.println();
  PgmPrintln("Type any character to start");
  while (!Serial.available());
  
  // initialize the SD card
  if (!card.init()) error("card.init");
  
  // initialize a FAT volume
  if (!Fat16::init(&card)) error("volume.init");
  
  // remove files if they exist
  Fat16::remove("CALLBACK.TXT");
  Fat16::remove("DEFAULT.TXT");
  Fat16::remove("STAMP.TXT");  
  
  // create a new file with default timestamps
  if (!file.open("DEFAULT.TXT", O_CREAT | O_WRITE)) {
    error("open DEFAULT.TXT");
  }
  Serial.println();
  PgmPrintln("Open with default times");
  printTimestamps(file);
  
  // close file
  file.close();
  /*
   * Test the date time callback function.
   *
   * dateTimeCallback() sets the function
   * that is called when a file is created
   * or when a file's directory entry is
   * modified by sync().
   *
   * The callback can be disabled by the call
   * SdFile::dateTimeCallbackCancel()
   */
  // set date time callback function
  Fat16::dateTimeCallback(dateTime);
  
  // create a new file with callback timestamps
  if (!file.open("CALLBACK.TXT", O_CREAT | O_WRITE)) {
    error("open CALLBACK.TXT");
  }  
  Serial.println();
  PgmPrintln("Open with callback times");
  printTimestamps(file);
  
  // change call back date
  day += 1;
  
  // must add two to see change since FAT second field is 5-bits
  second += 2;  
  
  // modify file by writing a byte
  file.write('t');
  
  // force dir update
  file.sync();
  
  Serial.println();
  PgmPrintln("Times after write");
  printTimestamps(file);
  
  // close file
  file.close();
  /*
   * Test timestamp() function
   *
   * Cancel callback so sync will not
   * change access/modify timestamp
   */
  Fat16::dateTimeCallbackCancel();
  
  // create a new file with default timestamps
  if (!file.open("STAMP.TXT", O_CREAT | O_WRITE)) {
    error("open STAMP.TXT");
  }
  // set creation date time
  if (!file.timestamp(T_CREATE, 2009, 11, 10, 1, 2, 3)) {
    error("create time");
  }
  // set write/modification date time
  if (!file.timestamp(T_WRITE, 2009, 11, 11, 4, 5, 6)) {
    error("write time");
  }
  // set access date
  if (!file.timestamp(T_ACCESS, 2009, 11, 12, 7, 8, 9)) {
    error("access time");
  }
  Serial.println();
  PgmPrintln("Times after timestamp() calls");
  printTimestamps(file);
  
  file.close();
  Serial.println();
  PgmPrintln("Done");
}

void loop(void){}
