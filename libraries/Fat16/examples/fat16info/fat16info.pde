/*
 * This sketch attempts to initialize a SD card and analyze its format.
 */
#include <Fat16.h>
#include <Fat16util.h> // use functions to print strings from flash memory

SdCard card;
Fat16 file;

void sdError(void) {
  if (card.errorCode) {
    PgmPrintln("SD error");
    PgmPrint("errorCode: ");
    Serial.println(card.errorCode, HEX);
    PgmPrint("errorData: ");
    Serial.println(card.errorData, HEX); 
  } 
  return;
}

uint8_t cidDmp(void) {
  cid_t cid;
  if (!card.readCID(&cid)) {
    PgmPrintln("readCID failed");
    sdError();
    while(1);
  }
  PgmPrint("\nManufacturer ID: ");
  Serial.println(cid.mid, HEX);
  PgmPrint("OEM ID: ");
  Serial.write(cid.oid[0]);
  Serial.write(cid.oid[1]);
  Serial.println();
  PgmPrint("Product: ");
  for (uint8_t i = 0; i < 5; i++) {
    Serial.write(cid.pnm[i]);
  }
  PgmPrint("\nVersion: ");
  Serial.print(cid.prv_n, DEC);
  Serial.write('.');
  Serial.println(cid.prv_m, DEC);
  PgmPrint("Serial number: ");
  Serial.println(cid.psn);
  PgmPrint("Manufacturing date: ");
  Serial.print(cid.mdt_month, DEC);
  Serial.write('/');
  Serial.println(2000 + cid.mdt_year_low + (cid.mdt_year_high <<4));
  Serial.println();
  return 1;
}
/*
 * print partition info
 */
uint8_t partitionInfo(void) {
  mbr_t* mbr = (mbr_t*)Fat16::dbgCacheBlock(0);
  if (mbr == 0) {
    PgmPrintln("partitionInfo: cacheBlock(0) failed");
    return 0;
  }
  uint8_t good = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if ((mbr->part[i].boot & 0X7F) != 0 ||
        (mbr->part[i].firstSector == 0 && mbr->part[i].totalSectors != 0)) {
          // not a valid partition table
          return 0;
    }
    if (mbr->part[i].firstSector != 0 && mbr->part[i].totalSectors != 0) {
      //might be ok
      good = 1;
    }
  }
  if (!good) return 0;
  PgmPrintln("Partition  Table:");
  PgmPrintln("part,boot,type,start,size");
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print(i + 1, DEC);
    Serial.write(',');
    Serial.print(mbr->part[i].boot, HEX);
    Serial.write(',');
    Serial.print(mbr->part[i].type, HEX);
    Serial.write(',');
    Serial.print(mbr->part[i].firstSector);
    Serial.write(',');
    Serial.println(mbr->part[i].totalSectors);
  }
  Serial.println();
  return 1;
}
/*
 * print info for FAT volume
 */
uint8_t volumeInfo(uint8_t part) {
  uint8_t volumeType = 0;
  if (part > 4) {
    PgmPrintln("volumeInfo: invalid partition number");
    return 0;
  }
  mbr_t* mbr = (mbr_t*)Fat16::dbgCacheBlock(0);
  if (mbr == 0) {
    PgmPrintln("volumeInfo: cacheBlock(0) failed");
    return 0;
  }
  bpb_t* bpb = &(((fbs_t*)mbr)->bpb);
  if (part != 0) {
    if ((mbr->part[part-1].boot & 0X7F) !=0  ||
      mbr->part[part-1].totalSectors < 100 ||
      mbr->part[part-1].firstSector == 0) {
        //not a valid partition
        return 0;
     }
     fbs_t* fbs = (fbs_t*)Fat16::dbgCacheBlock(mbr->part[part-1].firstSector);
     if (fbs == 0) {
       PgmPrintln("volumeInfo cacheBlock(fat boot sector) failed");
       return 0;
     }
     if (fbs->bootSectorSig0 != 0X55 || fbs->bootSectorSig1 != 0XAA) {
       PgmPrintln("Bad FAT boot sector signature for partition: ");
       Serial.println(part, DEC);
       return 0;
     } 
     bpb = &fbs->bpb;
  }
  if (bpb->bytesPerSector != 512 ||
      bpb->fatCount == 0 ||
      bpb->reservedSectorCount == 0 ||
      bpb->sectorsPerCluster == 0 ||
      (bpb->sectorsPerCluster & (bpb->sectorsPerCluster - 1)) != 0) {
        // not valid FAT volume
        return 0;
  }
  uint32_t totalBlocks = bpb->totalSectors16 ? bpb->totalSectors16 : bpb->totalSectors32;
  uint32_t fatSize = bpb->sectorsPerFat16;
  if (fatSize == 0) {
    // first entry of FAT32 structure is FatSize32
    // don't ask why this works
    fatSize = *(&bpb->totalSectors32 + 1);
  }
  uint16_t rootDirSectors = (32*bpb->rootDirEntryCount + 511)/512;
  uint32_t firstDataSector = bpb->reservedSectorCount + 
                                 bpb->fatCount*fatSize + rootDirSectors;
  uint32_t dataBlockCount = totalBlocks - firstDataSector;
  uint32_t clusterCount = dataBlockCount/bpb->sectorsPerCluster;
  if (part) {
    PgmPrint("FAT Volume info for partition: ");
    Serial.println(part, DEC);
  } else {
    PgmPrintln("FAT Volume is super floppy format");
  }
  if (clusterCount < 4085) {
    volumeType = 12;
  } else if (clusterCount < 65525) {
    volumeType = 16;
  } else {
    volumeType = 32;
  }
  PgmPrint("Volume is FAT");
  Serial.println(volumeType, DEC);
  PgmPrint("clusterSize: ");
  Serial.println(bpb->sectorsPerCluster, DEC);
  PgmPrint("clusterCount: ");
  Serial.println(clusterCount);  
  PgmPrint("fatCount: ");
  Serial.println(bpb->fatCount, DEC);
  PgmPrint("fatSize: ");
  Serial.println(fatSize);
  PgmPrint("totalBlocks: ");
  Serial.println(totalBlocks);
  Serial.println();
  return volumeType;
}
void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println();
  PgmPrint("Fat16 version: ");
  Serial.println(FAT16_VERSION);
  PgmPrint("FreeRam: ");
  Serial.println(FreeRam(), DEC);
}

void loop() {
  while (Serial.read() >= 0);
  PgmPrintln("\ntype any character to start");
  while (Serial.read() < 0);
  int8_t part = -1;
  uint32_t t0 = millis();
  if (!card.init()) {
    PgmPrintln("card.init failed");
    sdError();
    return;
  }
  uint32_t d = millis()- t0;
  PgmPrint("\ninit time: ");
  Serial.println(d);
  cidDmp();
  uint32_t size = card.cardSize();
  PgmPrint("Card Size(blocks): ");
  Serial.println(size);
  if (size < 1000) {
    PgmPrint("Quitting, bad size");
    return;
  }
  Serial.println();
  Fat16::dbgSetDev(&card);
  if (partitionInfo()) {
    for (uint8_t i = 1; i < 5; i++) {
      if (volumeInfo(i) == 16) part = i;
    }
  } else {
    PgmPrintln("Card is not MBR trying super floppy");
    if (volumeInfo(0) == 16) part = 0;
  }
  if (part >= 0) {
    if (!Fat16::init(&card, part)) {
      PgmPrintln("Can't init volume");
      return;
    }
    uint16_t index = 0;
    dir_t d;
    PgmPrintln("Root Directory:");
    PgmPrintln("name     ext att size");
    uint8_t nd = 0;
    //list up to 20 files
    while ((Fat16::readDir(&d, &index)) && nd++ < 20) {
      for (uint8_t i = 0; i < 11; i++) {
        if (i == 8)Serial.write(' ');
        Serial.write(d.name[i]);
      }
      Serial.write(' ');
      Serial.print(d.attributes, HEX);
      Serial.write(' ');
      Serial.println(d.fileSize);
      index++;
    }
    PgmPrintln("\nDone");
  } else {
    PgmPrintln("FAT16 volume not found");
  }
}
