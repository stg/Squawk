/*
 * This sketch is a simple write/read benchmark.
 */
#include <Fat16.h>
#include <Fat16util.h>

#define FILE_SIZE_MB 5
#define FILE_SIZE (1000000UL*FILE_SIZE_MB)
#define BUF_SIZE 100

uint8_t buf[BUF_SIZE];

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

void setup() {
  Serial.begin(9600);
  PgmPrintln("Type any character to start");
  while (!Serial.available());
  
  PgmPrint("Free RAM: ");
  Serial.println(FreeRam());  
 
  if (!card.init()) error("card.init failed!");
  if (!Fat16::init(&card)) error("Fat16::init failed!");
  
  // open or create file - truncate existing file.
  if (!file.open("BENCH.DAT", O_CREAT | O_TRUNC | O_RDWR)) {
    error("open");
  }
  
  // fill buf with known data
  for (uint16_t i = 0; i < (BUF_SIZE-2); i++) {
    buf[i] = 'A' + (i % 26);
  }
  buf[BUF_SIZE-2] = '\r';
  buf[BUF_SIZE-1] = '\n';
  
   PgmPrint("File size ");
  Serial.print(FILE_SIZE_MB);
  PgmPrintln(" MB");
  PgmPrintln("Starting write test.  Please wait up to three minutes");
  
  // do write test
  uint32_t n =FILE_SIZE/sizeof(buf);
  uint32_t t = millis();
  for (uint32_t i = 0; i < n; i++) {
    if (file.write(buf, sizeof(buf)) != sizeof(buf)) {
      error("write");
    }
  }
  t = millis() - t;
  file.sync();
  double r = (double)file.fileSize()/t;
  PgmPrint("Write ");
  Serial.print(r);
  PgmPrintln(" KB/sec");
  Serial.println();
  PgmPrintln("Starting read test.  Please wait up to a minute");
  
  // do read test
  file.rewind();
  t = millis();
   for (uint32_t i = 0; i < n; i++) {
    if (file.read(buf, sizeof(buf)) != sizeof(buf)) {
      error("read");
    }
  }
  t = millis() - t;
  r = (double)file.fileSize()/t;
  PgmPrint("Read ");
  Serial.print(r);
  PgmPrintln(" KB/sec");
  PgmPrintln("Done");
}

void loop() { }
