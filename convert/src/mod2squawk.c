#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#endif

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

void alert(char *str, int argc) {
#ifdef WIN32
  if(argc != 4) {
    MessageBox(NULL, str, "Squawk Converter", MB_OK); 
    return;
  }
#endif
  printf("%s", str);
}

bool confirm(char *str, int argc) {
  char resp;
#ifdef WIN32
  if(argc != 4) {
    return MessageBox(NULL, str, "Squawk Converter", MB_YESNO) == IDYES;
  }
#endif
  printf("%s ", str);
  do {
    resp = toupper(getchar());
  } while(resp != 'N' && resp != 'Y');
  printf("%c\n", resp);
  return resp == 'Y';
}

static void print_use(char**argv) {
  printf("Usage:\n\t%s -? [input].mod [output].c\n", argv[0]);
  printf("-? is either -f for SD card file or -a for a Melody array\n");
  printf("Example\n\t%s -f melody.mod melody.sqm\n", argv[0]);
}

int main(int argc,char**argv) {
  uint8_t *data = NULL;
  FILE *f;
  protracker_head_t *head;
  size_t filesize;
  char name[24];
  unsigned int n, mode = 0;
  uint8_t patterns = 0;
  
#ifndef WIN32
  // Check parameter count
  if(argc != 4) {
    print_use(argv);
    return 1;
  }
#endif

  if(argc > 1) {
    if(strlen(argv[1]) > 1) {
      if(argv[1][0] == '-') {
        if(argv[1][1] == 'f' || argv[1][1] == 'F') {
          mode = 1;
        } else if(argv[1][1] == 'a' || argv[1][1] == 'A') {
          mode = 2;
        }
      }
    }
#ifndef WIN32
    if(mode == 0) {
      print_use(argv);
      return 1;
    }
#endif
  }

  if(argc > 2) {
    // Open file
    data = blob(argv[2], &filesize);
#ifdef WIN32
  } else {
    char szFileName[MAX_PATH];
    OPENFILENAME ofn; 
    szFileName[0] = '\0'; 
    memset(&ofn, 0, sizeof(ofn)); 
    ofn.lStructSize = sizeof(ofn); 
    ofn.hwndOwner = NULL; 
    ofn.lpstrFile = szFileName; 
    ofn.nMaxFile = sizeof(szFileName); 
    ofn.hInstance = NULL; 
    ofn.lpstrFilter = TEXT("ProTracker modules (*.mod)\0*.mod\0All Files (*.*)\0*.*\0\0"); 
    ofn.nFilterIndex = 1;  
    ofn.Flags= OFN_FILEMUSTEXIST;
    int bRes; 
    if(GetOpenFileName(&ofn) == 0) return 1;
    // Open file
    data = blob(szFileName, &filesize);
#endif
  }
  if(!data || filesize == 0) {
    alert("Unable to open input file\n", argc);
    return 1;
  }
  
  // Check file size
  if(filesize < sizeof(protracker_head_t)) {
      alert("Input file is too small\n", argc);
      free(data);
      return 1;
  }

  head = (protracker_head_t*)data;

  // Check identifier
  trim(memcpy(name, head->ident, 4), 4);
  if(strcmp(name, "M.K.") != 0) {
    if(!confirm("Module is not marked \"M.K.\", continue [Y/N]?", argc)) {
      free(data);
      return 1;
    }
  }

  trim(memcpy(name, head->name, 20), 20);
  printf("Processing module: %s\n", name);

  // Find pattern count
  for(n = 0; n < 128; n++) {
    if(head->order[ n ] >= patterns) patterns = head->order[ n ] + 1;
  }
  
  printf("Pattern count: %i\n", patterns);
  printf("Order count: %i\n", head->order_count);

  // Validate pattern count
  if(patterns > 64 || head->order_count > 64) {
    alert("Pattern or order count exceed maximum\n", argc);
    free(data);
    return 1;
  }

  // Check file size again
  if(filesize < sizeof(protracker_head_t) + 1024 * patterns) {
    alert("File size does not match pattern count!\n", argc);
    free(data);
    return 1;
  }

  // Time to start writing output
  if(argc > 3) {
    f = fopen(argv[3], "wb");
#ifdef WIN32
  } else {
    char szFileName[MAX_PATH];
    OPENFILENAME ofn; 
    szFileName[0] = '\0'; 
    memset(&ofn, 0, sizeof(ofn)); 
    ofn.lStructSize = sizeof(ofn); 
    ofn.hwndOwner = NULL; 
    ofn.lpstrFile = szFileName; 
    ofn.nMaxFile = sizeof(szFileName); 
    ofn.hInstance = NULL; 
    ofn.lpstrFilter = TEXT("Squawk module (*.sqm)\0*.sqm\0Squawk c-array (*.txt, *.c, *.cpp)\0*.txt;*.c;*.cpp\0All Files (*.*)\0*.*\0\0");
    ofn.lpstrDefExt = "sqm";
    ofn.nFilterIndex = 1;  
    ofn.Flags= OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN;
    int bRes; 
    if(GetSaveFileName(&ofn) == 0) return 1;
    // Open file
    f = fopen(szFileName, "wb");

    if(mode == 0) {
      mode = 1;
      char *p_ext = strrchr(szFileName, '.');
      if(p_ext) {
        if(!(strcmp(p_ext, ".h") && strcmp(p_ext, ".c") && strcmp(p_ext, ".cpp") && strcmp(p_ext, ".txt"))) {
          mode = 2;
        }
      }
    }

#endif
  }
  if(!f) {
    alert("Unable to open output file\n", argc);
    free(data);
    return 1;
  }

  unsigned int ptn, row, chn;
  uint8_t *cell = data + sizeof(protracker_head_t);

  uint8_t fxc[4], fxp[4], note[4], sample[4];
  uint16_t period;
  uint8_t temp, hout;
  
  static unsigned int cols = 0;

#define HOUT(C) if(mode == 2 ) { if(!cols) fprintf(f, "\n "); fprintf(f, " 0x%02X,", C); if(++cols == 16) cols = 0; } else { hout = (C); fwrite(&hout, 1, 1, f); }

  if(mode == 2) {
    // Squawk melody
    fprintf(f, "Melody InsertTitleHere[] = {");
    HOUT( 'A' );
  } else {
    HOUT( 'S' ); // ID
    HOUT( 'Q' );
    HOUT( 'M' );
    HOUT( '1' );
    HOUT( 0 ); // Size of meta data
    HOUT( 0 );
  }
  
  // Write order list
  HOUT( head->order_count );
  for(n = 0; n < head->order_count; n++) {
    HOUT( head->order[n] );
  }

  // Write patterns
  for(ptn = 0; ptn < patterns; ptn++) {
    for(row = 0; row < 64; row++) {
      for(chn = 0; chn < 4; chn++) {
        
        // Basic extraction
        temp         = *cell++;              // sample.msb and period.msb
        period       = (temp & 0x0F) << 8;
        sample[chn]  = temp & 0xF0;
        period      |= *cell++;              // period.lsb
        temp         = *cell++;              // sample.lsb and effect
        sample[chn] |= temp >> 4;
        fxc[chn]     = (temp & 0x0F) << 4;
        fxp[chn]     = *cell++;              // parameters
        if(fxc[chn] == 0xE0) {
          fxc[chn]    |= fxp[chn] >> 4;      // extended parameters
          fxp[chn]    &= 0x0F;
        }
        
        #define DIF(A, B) ((A) > (B) ? ((int32_t)(A) - (int32_t)(B)) : ((int32_t)(B) - (int32_t)(A)))
        
        // Find closest matching period
        if(period == 0) {
          note[chn] = 0x7F;
        } else {
          int16_t best = DIF(period, period_tbl[0]);
          note[chn] = 0;
          for(n = 0; n < sizeof(period_tbl) / sizeof(uint16_t); n++) {
            if(DIF(period, period_tbl[n]) < best) {
              note[chn] = n;
              best = DIF(period, period_tbl[n]);
            }
          }
        }
        
        // Crunch volume/decimal commands
        if(fxc[chn] == 0x50 || fxc[chn] == 0x60 || fxc[chn] == 0xA0) {
          fxp[chn] = (fxp[chn] >> 1) & 0x77;
        } else if(fxc[chn] == 0x70) {
          fxp[chn] = (fxp[chn] & 0xF0) | ((fxp[chn] & 0x0F) >> 1);
        } else if(fxc[chn] == 0xC0 || fxc[chn] == 0xEA || fxc[chn] == 0xEB) {
          fxp[chn] >>= 1;
        } else if(fxc[chn] == 0xD0) {
          fxp[chn] = ((fxp[chn] >> 4) * 10) | (fxp[chn] & 0x0F);
        }

        // Check for unsupported commands
        if(fxc[chn] == 0xF0) {
          if(fxp[chn] > 0x20) {
            printf("[%02X][%02X][%01X] Set tempo not supported\n", ptn, row, chn);
          }
        } else if(fxc[chn] == 0xEF) {
          printf("[%02X][%02X][%01X] Funk-it not supported\n", ptn, row, chn);
        } else if(fxc[chn] == 0x80) {
          printf("[%02X][%02X][%01X] Panning not supported\n", ptn, row, chn);
        } else if(fxc[chn] == 0xE5) {
          printf("[%02X][%02X][%01X] Fine-tune is wonky by design\n", ptn, row, chn);
        } else if(fxc[chn] == 0xE6) {
          printf("[%02X][%02X][%01X] Advanced looping not supported\n", ptn, row, chn);
        } else if(fxc[chn] == 0xE8) {
          printf("[%02X][%02X][%01X] Panning not supported\n", ptn, row, chn);
        } else if(fxc[chn] == 0x90) {
          printf("[%02X][%02X][%01X] Sample offset not supported\n", ptn, row, chn);
        }

        // Re-nibblify - it's a word!
        if(chn != 3) {
          if((fxc[chn] & 0xF0) == 0xE0) {
            fxp[chn] |= fxc[chn] << 4;
          }
          fxc[chn] >>= 4;       
        }

      }

      // Ghetto crunch the last channel to save a byte
      switch(fxc[3]) {
        case 0x50: case 0x60: case 0xA0:
          fxc[3] = 0x1;
          if((fxp[3] >> 4) == (fxp[3] & 0x0F)) {
            if((fxp[3] && 0x0F) != 0) {
              printf("[%02X][%02X][%01X] Slide parameter combination not supported on ch 4\n", ptn, row, chn);
            }
          }           
          if((fxp[3] >> 4) >= (fxp[3] & 0x0F)) {
            fxp[3] = 0x08 + ((fxp[3] >> 4) - (fxp[3] & 0x0F));
          } else {
            fxp[3] = ((fxp[3] & 0x0F) - (fxp[3] >> 4));
          }
          break;
        case 0x70:
          fxc[3] = (fxp[3] & 0x4) ? 0x3 : 0x2;
          fxp[3] = (fxp[3] >> 4) | ((fxp[3] & 0x03) << 4);
          break;
        case 0xC0:
          fxc[3] = 0x4;
          fxp[3] &= 0x1F;
          break;
        case 0xB0:
          fxc[3] = 0x5;
          fxp[3] &= 0x1F;
          break;
        case 0xD0:
          fxc[3] = 0x6;
          if(fxp[3] > 63) fxp[3] = 0;
          break;
        case 0xF0:
          if(fxp[3] > 0x20) {
            fxc[3] = 0x0;
            fxp[3] = 0x00;
          } else {
            fxc[3] = 0x7;             
          }
          break;
        case 0xE7:
          fxc[3] = 0x8;
          break;
        case 0xE9:
          fxc[3] = 0x9;
          break;
        case 0xEA:
          fxc[3] = 0xA;
          fxp[3] |= 0x08;
          break;
        case 0xEB:
          fxc[3] = 0xA;
          break;
        case 0xEC:
          fxc[3] = 0xB;
          break;
        case 0xED:
          fxc[3] = 0xB;
          fxp[3] |= 0x10;
          break;
        case 0xEE:
          fxc[3] = 0xC;
          break;
        default:
          fxc[3] = 0;
          fxp[3] = 0;
      }
      if(note[3] != 0x7F) fxp[3] |= 0x80;
      if(sample[3]) fxp[3] |= 0x40;

      // Write out
      HOUT( (fxc[0]) | fxc[1] << 4 );
      HOUT( fxp[0] );
      HOUT( fxp[1] );
      HOUT( (fxc[2]) | fxc[3] << 4 );
      HOUT( fxp[2] );
      HOUT( fxp[3] );
      HOUT( note[0] | (sample[0] == 0 ? 0x00 : 0x80) );
      HOUT( note[1] | (sample[1] == 0 ? 0x00 : 0x80) );
      HOUT( note[2] | (sample[2] == 0 ? 0x00 : 0x80) );
    }
  }
  if(mode == 2) {
    // Squawk melody end
    fprintf(f, "\n};\n"); 
  }

  fclose(f);
  free(data);

#ifdef WIN32
  if(argc != 4) {
    printf("Conversion successful!\n");
    printf("Press any key to quit");
    getch();
  }
#endif  

  return 0;
}