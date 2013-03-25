#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// ProTracker module header
typedef struct __attribute__((packed)) {
  char    name[20];        // name
  struct __attribute__((packed)) {
    char    name[22];        // name
    uint8_t length_msb;      // length in words
    uint8_t length_lsb;
    uint8_t tuning;          // fine-tune value
    uint8_t volume;          // default volume
    uint8_t loop_offset_msb; // loop point in words
    uint8_t loop_offset_lsb;
    uint8_t loop_len_msb;    // loop length in words
    uint8_t loop_len_lsb;
  } sample[31];            // samples
  uint8_t order_count;     // song length
  uint8_t historical;      // for compatibility (always 0x7F)
  uint8_t order[128];      // pattern order list
  char  ident[4];          // identifier (always "M.K.")
} protracker_head_t;

// Deconstructed cell
typedef struct {
  uint8_t fxc, fxp, ixp;
} cell_t;

void *blob(char *filename, size_t *size) {
  void *data;
  FILE *f;
  f = fopen(filename, "rb");
  if(f) {
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, 0);
    data = malloc(*size);
    if(data) fread(data, 1, *size, f);
    fclose(f);
  }
  return data;
}

void trim(char *str, size_t max) {
  char *last = str;
  while(max--) {
    if(*str != ' ') last = str;
    str++;
  }
  *++last = 0;
}

static const uint16_t period_tbl[84] = {
  3424, 3232, 3048, 2880, 2712, 2560, 2416, 2280, 2152, 2032, 1920, 1814,
  1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016,  960,  907,
   856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480,  453,
   428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240,  226,
   214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120,  113,
   107,  101,   95,   90,   85,   80,   75,   71,   67,   63,   60,   56,
    53,   50,   47,   45,   42,   40,   37,   35,   33,   31,   30,   28,
};

int main(int argc,char**argv) {
  uint8_t *data, *p_data, *p_out;
  FILE *f;
  protracker_head_t head;
  size_t filesize;
  char name[24];
  unsigned int n, z;
  uint8_t patterns;

  // Check parameter count
  if(argc != 3) {
    printf("Useage:\n\t%s [input].mod [output].c\n", argv[0]);
    return 1;
  }

  // Open file
  data = blob(argv[1], &filesize);
  if(!data || filesize == 0) {
    printf("Unable to open input file\n");
    return 1;
  }

  // Nasty count c-array content count
  size_t arrcount = 0;
  p_data = data;
  while(p_data < &data[filesize - 4]) {
    if(*p_data == '0' && *(p_data + 1) == 'x') arrcount++;
    p_data++;
  }

  // Check file size
  if(arrcount < 608) {
      printf("Input file is too small\n");
      free(data);
      return 1;
  } else if(((unsigned int)((arrcount - 32) / 9)) * 9 != arrcount - 32) {
      printf("Input file has incorrect size\n");
      free(data);
      return 1;
  }

  // Memory for data
  uint8_t *tempdata = malloc(arrcount);
  if(!tempdata) {
    printf("Out of memory\n");
    free(data);
    return 1;
  }

  // Convert c-array to blob
  p_data = data;
  p_out = tempdata;
  while(p_data < &data[filesize - 4]) {
    if(*p_data == '0' && *(p_data + 1) == 'x') {
      *(p_data+4) = 0;
      *p_out++ = strtoul(p_data + 2, NULL, 16);
    }
    p_data++;
  }
  free(data);
  data = tempdata;

  // Create header
  memset(&head, 0, sizeof(protracker_head_t));
  memcpy(head.name, "Squawk Melody       ", 20);
  memcpy(head.sample[0].name, "Pulse                 ", 22);
  head.sample[0].length_lsb   = 0x20;
  head.sample[0].volume       = 0x3F;
  head.sample[0].loop_len_lsb = 0x20;
  memcpy(head.sample[1].name, "Square                ", 22);
  head.sample[1].length_lsb   = 0x20;
  head.sample[1].volume       = 0x3F;
  head.sample[1].loop_len_lsb = 0x20;
  memcpy(head.sample[2].name, "Triangle              ", 22);
  head.sample[2].length_lsb   = 0x20;
  head.sample[2].volume       = 0x3F;
  head.sample[2].loop_len_lsb = 0x20;
  memcpy(head.sample[3].name, "Noise                 ", 22);
  head.sample[3].length_msb   = 0x40;
  head.sample[3].volume       = 0x3F;
  head.sample[3].loop_len_msb = 0x40;
  for(n = 4; n < 32; n++) memset(head.sample[n].name, ' ', 22);
  head.order_count = data[0];
  head.historical  = 0x7F;
  memcpy(head.order, &data[1], 31);
  memcpy(head.ident, "M.K.", 4);

  // Time to start writing output
  f = fopen(argv[2], "wb");
  if(!f) {
    printf("Unable to open output file\n");
    free(data);
    return 1;
  }

  // Write header
  fwrite(&head, 1, sizeof(protracker_head_t), f);

  // Write patterns
  p_data = (data + 32);
  for(n = 0; n < ((arrcount - 32) / 9); n++) {
    uint8_t dbyte;
    cell_t cel[4];

    dbyte = *p_data++; cel[0].fxc = dbyte << 0x04; cel[1].fxc = dbyte &  0xF0;
    dbyte = *p_data++; cel[0].fxp = dbyte;
    dbyte = *p_data++; cel[1].fxp = dbyte;
    dbyte = *p_data++; cel[2].fxc = dbyte << 0x04; cel[3].fxc = dbyte >> 0x04;
    dbyte = *p_data++; cel[2].fxp = dbyte;
    dbyte = *p_data++; cel[3].fxp = dbyte;
    dbyte = *p_data++; cel[0].ixp = dbyte;
    dbyte = *p_data++; cel[1].ixp = dbyte;
    dbyte = *p_data++; cel[2].ixp = dbyte;

    if(cel[0].fxc == 0xE0) { cel[0].fxc |= cel[0].fxp >> 4; cel[0].fxp &= 0x0F; }
    if(cel[1].fxc == 0xE0) { cel[1].fxc |= cel[1].fxp >> 4; cel[1].fxp &= 0x0F; }
    if(cel[2].fxc == 0xE0) { cel[2].fxc |= cel[2].fxp >> 4; cel[2].fxp &= 0x0F; }

    cel[3].ixp = ((cel[3].fxp & 0x80) ? 0x00 : 0x7F) | ((cel[3].fxp & 0x40) ? 0x80 : 0x00);
    cel[3].fxp &= 0x3F;
    switch(cel[3].fxc) {
      case 0x02:
      case 0x03: if(cel[3].fxc & 0x01) cel[3].fxp |= 0x40; cel[3].fxp = (cel[3].fxp >> 4) | (cel[3].fxp << 4); cel[3].fxc = 0x70; break;
      case 0x01: if(cel[3].fxp & 0x08) cel[3].fxp = (cel[3].fxp & 0x07) << 4; cel[3].fxc = 0xA0; break;
      case 0x04: cel[3].fxc = 0xC0; break;
      case 0x05: cel[3].fxc = 0xB0; break;
      case 0x06: cel[3].fxc = 0xD0; break;
      case 0x07: cel[3].fxc = 0xF0; break;
      case 0x08: cel[3].fxc = 0xE7; break;
      case 0x09: cel[3].fxc = 0xE9; break;
      case 0x0A: cel[3].fxc = (cel[3].fxp & 0x08) ? 0xEA : 0xEB; cel[3].fxp &= 0x07; break;
      case 0x0B: cel[3].fxc = (cel[3].fxp & 0x10) ? 0xED : 0xEC; cel[3].fxp &= 0x0F; break;
      case 0x0C: cel[3].fxc = 0xEE; break;
    }

    for(z = 0; z < 4; z++) {
      uint16_t period;
      uint8_t  sample;
      uint8_t  effect;
      uint8_t  parameter;
      uint8_t  crunch[4];

      // Convert to ProTracker values
      if((cel[z].fxc >> 4) == 0x0E) {
        parameter = cel[z].fxp | (cel[z].fxc << 4);
      } else {
        parameter = cel[z].fxp;
      }
      effect = cel[z].fxc >> 4;
      sample = (cel[z].ixp & 0x80) ? z + 1 : 0;
      if((cel[z].ixp & 0x7F) == 0x7F) period = 0;
      else period = ((z == 3) ? 113 : period_tbl[cel[z].ixp & 0x7F]);

      // Crunch cell and write
      crunch[0] = sample & 0xF0 | (period >> 8);
      crunch[1] = (period & 0x00FF);
      crunch[2] = (sample << 4) | effect;
      crunch[3] = parameter;
      fwrite(crunch, 4, 1, f);
    }
  }

  // Write samples
  for(n = 0; n < 64; n++) {
    int8_t sample = n < 16 ? 127 : -128;
    fwrite(&sample, 1, 1, f);
  }
  for(n = 0; n < 64; n++) {
    int8_t sample = n < 32 ? 127 : -128;
    fwrite(&sample, 1, 1, f);
  }
  for(n = 0; n < 64; n++) {
    int8_t sample = n < 32 ? -128 + (n << 3) : 127 - ((n - 32) << 3);
    fwrite(&sample, 1, 1, f);
  }
  for(n = 0; n < 32768; n++) {
    int8_t sample = (rand() & 1) ? 127 : -128;
    fwrite(&sample, 1, 1, f);
  }

  fclose(f);
  free(data);
  return 0;
}