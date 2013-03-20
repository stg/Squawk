// Squawk Soft-Synthesizer Library for Arduino
//
// Davey Taylor 2013
// d.taylor@arduino.cc

#include "Squawk.h"

// Period range, used for clamping
#define PERIOD_MIN 54
#define PERIOD_MAX 856

// Convenience macros
#define LO4(V)    (((V) & 0x0F)    )
#define HI4(V)    (((V) & 0xF0) >> 4)
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define FREQ(PERIOD) (tuning_long / PERIOD)
#define SET_VOL(X) if(ch != 2) { p_osc->vol = (X); } else if((X) == 0) { osc[2].freq = 0; }

// Oscillator memory
typedef struct {
  uint8_t fxp;
  uint8_t offset;
  uint8_t mode;
} osc_t;

// Deconstructed cell
typedef struct {
  uint8_t fxc, fxp, ixp;
} cell_t;

// Effect memory
typedef struct {
  int8_t    volume;
  uint8_t   port_speed;
  uint16_t  port_target;
  bool      glissando;
  osc_t     vibr;
  osc_t     trem;
  uint16_t  period;
  uint8_t   param;
} fxm_t;    

// Locals
static uint8_t  speed;
static uint8_t  tick;
static uint8_t  ix_nextrow;
static uint8_t  ix_nextorder;
static uint8_t  ix_row;
static uint8_t  ix_order;
static uint8_t  row_delay;
static fxm_t    fxm[4];
static cell_t   cel[4];
static bool     deconstruct;
static uint32_t tuning_long;
static uint16_t sample_rate;
static float    tuning = 6.0;

static const uint8_t *p_play;

// Exports
oscillator_t osc[4];

// Imports
extern intptr_t squawk_register;

// ProTracker(+1 octave) period table
static const uint16_t period_tbl[36] PROGMEM = {
//  856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
  428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
  214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
  107, 101,  95,  90,  85,  80,  76,  71,  67,  64,  60,  57,
};

// ProTracker sine table
static const int8_t sine[64] PROGMEM = {
  0x00, 0x0C, 0x18, 0x25, 0x30, 0x3C, 0x47, 0x51, 0x5A, 0x62, 0x6A, 0x70, 0x76, 0x7A, 0x7D, 0x7F,
  0x7F, 0x7F, 0x7D, 0x7A, 0x76, 0x70, 0x6A, 0x62, 0x5A, 0x51, 0x47, 0x3C, 0x30, 0x25, 0x18, 0x0C,
  0x00, 0xF3, 0xE7, 0xDA, 0xCF, 0xC3, 0xB8, 0xAE, 0xA5, 0x9D, 0x95, 0x8F, 0x89, 0x85, 0x82, 0x80,
  0x80, 0x80, 0x82, 0x85, 0x89, 0x8F, 0x95, 0x9D, 0xA5, 0xAE, 0xB8, 0xC3, 0xCF, 0xDA, 0xE7, 0xF3,
};

// Squawk object
SquawkSynth Squawk;

// Look up or generate waveform for ProTracker vibrato/tremolo oscillator
static int8_t do_osc(osc_t *p_osc) {
  int8_t sample = 0;
  int16_t mul;
  switch(p_osc->mode & 0x03) {
    case 0: // Sine
      sample = pgm_read_byte(&sine[(p_osc->offset) & 0x3F]);
      if(sample > 127) sample -= 256;
      break;
    case 1: // Square
      sample = (p_osc->offset & 0x20) ? 127 : -128;
      break;
    case 2: // Saw
      sample = ((p_osc->offset << 2) & 0xFF) - 128;
      break;
    case 3: // Noise (random)
      sample = rand() & 0xFF;
      break;
  }
  mul = sample * LO4(p_osc->fxp);
  p_osc->offset = (p_osc->offset + HI4(p_osc->fxp) - 1) & 0xFF;
  return mul >> 6;
}

// Calculates and returns arpeggio period
// Essentially finds period of current note + halftones
static inline uint16_t arpeggio(uint8_t ch, uint8_t halftones) {
  uint8_t n;
  for(n = 0; n != 47; n++) {
    if(fxm[ch].period >= pgm_read_word(&period_tbl[n])) break;
  }
  return pgm_read_word(&period_tbl[MIN(n + halftones, 47)]);
}

// Calculates and returns glissando period
// Essentially snaps a sliding frequency to the closest note
static inline uint16_t glissando(uint8_t ch) {
  uint8_t n;
  uint16_t period_h, period_l;
  for(n = 0; n != 47; n++) {
    period_l = pgm_read_word(&period_tbl[n]);
    period_h = pgm_read_word(&period_tbl[n + 1]);
    if(fxm[ch].period < period_l && fxm[ch].period >= period_h) {
      if(period_l - fxm[ch].period <= fxm[ch].period - period_h) {
        period_h = period_l;
      }
      break;
    }
  }
  return period_h;
}

// Resets playback
static void playroutine_reset() {
  memset(fxm, 0, sizeof(fxm));
  tick         = 0;
  ix_nextrow   = 0;
  ix_nextorder = 0;
  ix_row       = 0;
  ix_order     = 0;
  row_delay    = 0;
  speed        = 6;
  deconstruct  = true;
}

// Tunes Squawk to a different frequency
void SquawkSynth::tune(float new_tuning) {
  tuning = new_tuning;
  tuning_long = (long)((double)sample_rate * (double)tuning);
}

// Initializes Squawk
// Sets up the selected port, and the sample cruncher ISR
void SquawkSynth::begin(uint16_t hz) {
  word isr_rr;

  sample_rate = hz;
  tuning_long = (long)((double)sample_rate * (double)tuning);

  if(squawk_register == (intptr_t)&OCR0A) {
    // Squawk uses PWM on OCR0A/PD5(ATMega328/168)/PB7(ATMega32U4)
#ifdef  __AVR_ATmega32U4__
    DDRB  |= 0b10000000; // TODO: FAIL on 32U4
#else
    DDRD  |= 0b01000000;
#endif                  
    TCCR0A = 0b10000011; // Fast-PWM 8-bit   
    TCCR0B = 0b00000001; // 62500Hz          
    OCR0A  = 0x7F;
  } else if(squawk_register == (intptr_t)&OCR0B) {
    // Squawk uses PWM on OCR0B/PC5(ATMega328/168)/PD0(ATMega32U4)
#ifdef  __AVR_ATmega32U4__    
    DDRD  |= 0b00000001;
#else
    DDRD  |= 0b00100000;
#endif                   // Set timer mode to
    TCCR0A = 0b00100011; // Fast-PWM 8-bit   
    TCCR0B = 0b00000001; // 62500Hz          
    OCR0B  = 0x7F;
#ifdef OCR2A
  } else if(squawk_register == (intptr_t)&OCR2A) {
    // Squawk uses PWM on OCR2A/PB3
    DDRB  |= 0b00001000; // Set timer mode to
    TCCR2A = 0b10000011; // Fast-PWM 8-bit   
    TCCR2B = 0b00000001; // 62500Hz          
    OCR2A  = 0x7F;
#endif
#ifdef OCR2B
  } else if(squawk_register == (intptr_t)&OCR2B) {
    // Squawk uses PWM on OCR2B/PD3
    DDRD  |= 0b00001000; // Set timer mode to
    TCCR2A = 0b00100011; // Fast-PWM 8-bit   
    TCCR2B = 0b00000001; // 62500Hz          
    OCR2B  = 0x7F;
#endif
#ifdef OCR3AL
  } else if(squawk_register == (intptr_t)&OCR3AL) {
    // Squawk uses PWM on OCR3AL/PC6
    DDRC  |= 0b01000000; // Set timer mode to
    TCCR3A = 0b10000001; // Fast-PWM 8-bit
    TCCR3B = 0b00001001; // 62500Hz
    OCR3AH = 0x00;
    OCR3AL = 0x7F;
#endif
  } else if(squawk_register == (intptr_t)&SPDR) {
    // NOT YET SUPPORTED
    // Squawk uses external DAC via SPI
    // TODO: Configure SPI
    // TODO: Needs SS toggle in sample cruncher
  } else if(squawk_register == (intptr_t)&PORTB) {
    // NOT YET SUPPORTED
    // Squawk uses resistor ladder on PORTB
    // TODO: Needs shift right in sample cruncher
    DDRB   = 0b11111111;
  } else if(squawk_register == (intptr_t)&PORTC) {
    // NOT YET SUPPORTED
    // Squawk uses resistor ladder on PORTC
    // TODO: Needs shift right in sample cruncher
    DDRC   = 0b11111111;
  }

  // Seed LFSR (needed for noise)
  osc[3].freq = 0x2000;

  // Set up ISR to run at sample_rate (may not be exact)
  isr_rr = F_CPU / sample_rate;
  TCCR1A = 0b00000000;     // Set timer mode
  TCCR1B = 0b00001001;
  OCR1AH = isr_rr >> 8;    // Set freq
  OCR1AL = isr_rr & 0xFF;
}

void SquawkSynth::play() {
  TIMSK1 = 1 << OCIE1A;    // Enable interrupt
}

void SquawkSynth::play(const uint8_t *melody) {
  cli();
  p_play = melody;
  playroutine_reset();
  sei();
  play();
}

void SquawkSynth::pause() {
  TIMSK1 = 0;              // Disable interrupt
}

void SquawkSynth::stop() {
  pause();
  playroutine_reset();
}

// Progress module by one tick
void SquawkSynth::advance() {

  // Advance tick
  if(++tick == speed) tick = 0;
    
  // Handle row delay
  if(row_delay) {
    if(tick == 0) row_delay--;
    return;
  }
  
  // Advance playback
  if(tick == 0) {
    if(++ix_row == 64) {
      ix_row = 0;
      if(++ix_order == pgm_read_byte(p_play)) ix_order = 0;
      deconstruct = true;
    }
    // Forced order/row
    if(ix_nextorder != 0xFF) {
      ix_order = ix_nextorder;
      ix_nextorder = 0xFF;
      deconstruct = true;
    }
    if(ix_nextrow != 0xFF) {
      ix_row = ix_nextrow;
      ix_nextrow = 0xFF;
      deconstruct = true;
    }
  }
  
  // Deconstruct cells
  if(deconstruct) {
    const uint8_t *p_data;
    uint8_t data;
    p_data = &p_play[32 + ((pgm_read_byte(&p_play[1 + ix_order]) << 6) + ix_row) * 9];
    data = pgm_read_byte(  p_data); cel[0].fxc  =  data << 0x04;
                                    cel[1].fxc  =  data &  0xF0;
    data = pgm_read_byte(++p_data); cel[0].fxp  =  data;
    data = pgm_read_byte(++p_data); cel[1].fxp  =  data;
    data = pgm_read_byte(++p_data); cel[2].fxc  =  data << 0x04;
                                    cel[3].fxc  =  data &  0xF0;
    data = pgm_read_byte(++p_data); cel[2].fxp  =  data;
    data = pgm_read_byte(++p_data); cel[3].fxp  =  data;
    data = pgm_read_byte(++p_data); cel[0].ixp  =  data;
    data = pgm_read_byte(++p_data); cel[1].ixp  =  data;
    data = pgm_read_byte(++p_data); cel[2].ixp  =  data & 0x3F;
                                    cel[3].ixp  = (data &  0x80) ? 0x00 : 0x3F;
    if(cel[0].fxc == 0xE0) { cel[0].fxc |= cel[0].fxp >> 4; cel[0].fxp &= 0x0F; }
    if(cel[1].fxc == 0xE0) { cel[1].fxc |= cel[1].fxp >> 4; cel[1].fxp &= 0x0F; }
    if(cel[2].fxc == 0xE0) { cel[2].fxc |= cel[2].fxp >> 4; cel[2].fxp &= 0x0F; }
    if(cel[3].fxc == 0xE0) { cel[3].fxc |= cel[3].fxp >> 4; cel[3].fxp &= 0x0F; }
  }
  
  // Quick pointer access
  fxm_t        *p_fxm = fxm;
  oscillator_t *p_osc = osc;
  cell_t       *p_cel = cel;

  // Temps
  uint8_t       ch, fx, fxp;
  bool          pattern_jump = false;
  uint8_t       ix_period;

  for(ch = 0; ch != 4; ch++) {
    uint8_t       temp;
    
    // Deconstruct cell
    fx        = p_cel->fxc;
    fxp       = p_cel->fxp;
    ix_period = p_cel->ixp;

    // General effect parameter memory
    if(fx == 0x10 || fx == 0x20 || fx == 0xE1 || fx == 0xE2 || fx == 0x50 || fx == 0x60 || fx == 0xA0) {
      if(fxp) {
        p_fxm->param = fxp;
      } else {
        fxp = p_fxm->param;
      }
    }

    // If first tick
    if(tick == (fx == 0xED ? fxp : 0)) {
      // Reset oscillators
      if((p_fxm->vibr.mode & 0x4) == 0x0) p_fxm->vibr.offset = 0;
      if((p_fxm->trem.mode & 0x4) == 0x0) p_fxm->trem.offset = 0;

      if(ix_period != 0x3F) {
        // Reset volume
        p_fxm->volume = 0x1F;
        SET_VOL(p_fxm->volume);
        // Cell has note
        if(fx == 0x30 || fx == 0x50) {
          // Tone-portamento effect setup
          p_fxm->port_target = pgm_read_word(&period_tbl[ix_period]);
        } else {
          // Start note
          p_osc->freq = FREQ(pgm_read_word(&period_tbl[ix_period]));
          // Set required effect memory parameters
          p_fxm->period = pgm_read_word(&period_tbl[ix_period]);
        }
      }

      // Effects processed when tick = 0
      switch(fx) {
        case 0x30: // Portamento
          if(fxp) p_fxm->port_speed = fxp;
          break;
        case 0xB0: // Jump to pattern
          ix_nextorder = (fxp >= pgm_read_byte(&p_play[0]) ? 0x00 : fxp);
          ix_nextrow = 0;
          pattern_jump = true;
          break;
        case 0xC0: // Set volume
          p_fxm->volume = MIN(fxp, 0x1F);
          SET_VOL(p_fxm->volume);
          break;
        case 0xD0: // Jump to row
          fxp = HI4(fxp) * 10 + LO4(fxp);
          if(!pattern_jump) ix_nextorder = ((ix_order + 1) >= pgm_read_byte(&p_play[0]) ? 0x00 : ix_order + 1);
          pattern_jump = true;
          ix_nextrow = (fxp > 63 ? 0 : fxp);
          break;
        case 0xF0: // Set speed, CIA (playrate) not supported
          if(fxp <= 0x20) speed = fxp;
          break;
        case 0x40: // Vibrato
          if(fxp) p_fxm->vibr.fxp = fxp;
          break;
        case 0xE1: // Fine slide up
          p_fxm->period = MAX(p_fxm->period - fxp, PERIOD_MIN);
          p_osc->freq = FREQ(p_fxm->period);
          break;
        case 0xE2: // Fine slide down
          p_fxm->period = MIN(p_fxm->period + fxp, PERIOD_MAX);
          p_osc->freq = FREQ(p_fxm->period);
          break;
        case 0xE3: // Glissando control
          p_fxm->glissando = (fxp != 0);
          break;
        case 0xE4: // Set vibrato waveform
          p_fxm->vibr.mode = fxp;
          break;
        case 0xE7: // Set tremolo waveform
          p_fxm->trem.mode = fxp;
          break;
        case 0xEA: // Fine volume slide up
          p_fxm->volume = MIN(p_fxm->volume + fxp, 0x1F);
          SET_VOL(p_fxm->volume);
          break;
        case 0xEB: // Fine volume slide down
          p_fxm->volume = MAX(p_fxm->volume - fxp, 0);
          SET_VOL(p_fxm->volume);
          break;
        case 0xEE: // Delay
          row_delay = fxp;
          break;
      }
      
    } else {
      // Effects processed when tick > 0
      switch(fx) {
        case 0x10: // Slide up
          p_fxm->period = MAX(p_fxm->period - fxp, PERIOD_MIN);
          p_osc->freq = FREQ(p_fxm->period);
          break;
        case 0x20: // Slide down
          p_fxm->period = MIN(p_fxm->period + fxp, PERIOD_MAX);
          p_osc->freq = FREQ(p_fxm->period);
          break;
        case 0xE9: // Retrigger note
          temp = tick; while(temp >= fxp) temp -= fxp;
          if(temp == 0) {
            p_osc->freq = FREQ(pgm_read_word(&period_tbl[ix_period]));
          }
          break;
        case 0xEC: // Note cut
          if(fxp == tick) {
            SET_VOL(0x00);
          }
          break;
        default:   // Multi-effect processing
          // Portamento
          if(fx == 0x30 || fx == 0x50) {
            if(p_fxm->period < p_fxm->port_target) p_fxm->period = MIN(p_fxm->period + p_fxm->port_speed,  p_fxm->port_target);
            else                                   p_fxm->period = MAX(p_fxm->period - p_fxm->port_speed,  p_fxm->port_target);
            if(p_fxm->glissando) p_osc->freq = FREQ(glissando(ch));
            else                 p_osc->freq = FREQ(p_fxm->period);
          }
          // Volume slide
          if(fx == 0x50 || fx == 0x60 || fx == 0xA0) {
            if((fxp & 0xF0) == 0) p_fxm->volume -= (LO4(fxp));
            if((fxp & 0x0F) == 0) p_fxm->volume += (HI4(fxp));
            p_fxm->volume = MAX(MIN(p_fxm->volume, 0x1F), 0);
            SET_VOL(p_fxm->volume);
          }
      }
    }
      
    // Normal play and arpeggio
    if(fx == 0x00) {
      temp = tick; while(temp > 2) temp -= 2;
      if(temp == 0) {
        // Reset
        if(ch != 2 || osc[2].vol) p_osc->freq = FREQ(p_fxm->period);
      } else if(fxp) {
        // Arpeggio
        p_osc->freq = FREQ(arpeggio(ch, (temp == 1 ? HI4(fxp) : LO4(fxp))));
      }
    } else if(fx == 0x40 || fx == 0x60) {
      // Vibrato
      p_osc->freq = FREQ((p_fxm->period + do_osc(&p_fxm->vibr)));
    } else if(fx == 0x70) {
      // Tremolo
      temp = p_fxm->volume + do_osc(&p_fxm->trem);
      SET_VOL(MAX(MIN(temp, 0x1F), 0));
    }
  
    // Next channel
    p_fxm++; p_cel++; p_osc++;
  }
}