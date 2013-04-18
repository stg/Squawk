// A simple data logger for the analog pins
#define LOG_INTERVAL  1000 // mills between entries
#define SENSOR_COUNT     3 // number of analog pins to log
#define ECHO_TO_SERIAL   1 // echo data to serial port
#define WAIT_TO_START    1 // Wait for serial input in setup()
#define SYNC_INTERVAL 1000 // mills between calls to sync()
uint32_t syncTime = 0;     // time of last sync()
uint32_t logTime = 0;      // time data was logged

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
  
#if WAIT_TO_START
  PgmPrintln("Type any character to start");
  while (!Serial.available());
#endif //WAIT_TO_START

  // initialize the SD card
  if (!card.init()) error("card.init");
  
  // initialize a FAT16 volume
  if (!Fat16::init(&card)) error("Fat16::init");
  
  // create a new file
  char name[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    name[6] = i/10 + '0';
    name[7] = i%10 + '0';
    // O_CREAT - create the file if it does not exist
    // O_EXCL - fail if the file exists
    // O_WRITE - open for write only
    if (file.open(name, O_CREAT | O_EXCL | O_WRITE))break;
  }
  if (!file.isOpen()) error ("create");
  PgmPrint("Logging to: ");
  Serial.println(name);

  // write data header
  
  // clear write error
  file.writeError = false;
  file.print("millis");
#if ECHO_TO_SERIAL 
  Serial.print("millis");
#endif //ECHO_TO_SERIAL

#if SENSOR_COUNT > 6
#error SENSOR_COUNT too large
#endif //SENSOR_COUNT

  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    file.print(",sens");
    file.print(i, DEC);    
#if ECHO_TO_SERIAL
    Serial.print(",sens");
    Serial.print(i, DEC);
#endif //ECHO_TO_SERIAL
  }
  file.println();  
#if ECHO_TO_SERIAL
  Serial.println();
#endif  //ECHO_TO_SERIAL

  if (file.writeError || !file.sync()) {
    error("write header");
  }
}

void loop() {   // run over and over again
  uint32_t m = logTime;
  // wait till time is an exact multiple of LOG_INTERVAL
  do {
      logTime = millis();
  } while (m == logTime || logTime % LOG_INTERVAL);
  // log time to file
  file.print(logTime);  
#if ECHO_TO_SERIAL
  Serial.print(logTime);
#endif //ECHO_TO_SERIAL
      
  // add sensor data 
  for (uint8_t ia = 0; ia < SENSOR_COUNT; ia++) {
    uint16_t data = analogRead(ia);
    file.write(',');
    file.print(data);
#if ECHO_TO_SERIAL
    Serial.write(',');
    Serial.print(data);
#endif //ECHO_TO_SERIAL
  }
  file.println();  
#if ECHO_TO_SERIAL
  Serial.println();
#endif //ECHO_TO_SERIAL

  if (file.writeError) error("write data");
  
  //don't sync too often - requires 2048 bytes of I/O to SD card
  if ((millis() - syncTime) <  SYNC_INTERVAL) return;
  syncTime = millis();
  if (!file.sync()) error("sync");
}
