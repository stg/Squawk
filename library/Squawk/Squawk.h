// Squawk Soft-Synthesizer Library for Arduino
//
// Davey Taylor 2013
// d.taylor@arduino.cc

#ifndef _SQUAWK_H_
#define _SQUAWK_H_
#include <stddef.h>
#include <inttypes.h>
#include "Arduino.h"

#define Melody const uint8_t PROGMEM

class SquawkSynth {

public:
  SquawkSynth() {};

  // Initialize Squawk to generate samples at sample_rate Hz
  void begin(uint16_t sample_rate);

  // Load and play specified melody
  // melody needs to point to PROGMEM data
  void play(const uint8_t *melody);
  
  // Play the currently loaded melody
  void play();
  
  // Pause playback
  void pause();
  
  // Stop playback (essentially pause and restart)
  void stop();
  
  // Advance playback
  void advance();
  
  // Tune Squawk to a different frequency
  // tuning default is 6.0
  void tune(float tuning);
};

extern SquawkSynth Squawk;

#ifdef __AVR_ATmega32U4__
// Supported configurations for ATmega32U4
#define SQUAWK_PWM_PIN5  OCR3AL
#define SQUAWK_PWM_PIN11 OCR0A
#define SQUAWK_PWM_PIN3  OCR0B
/*
// NOT SUPPORTED YET
#define SQUAWK_PWM_PIN6  OCR4D
#define SQUAWK_PWM_PIN9  OCR4B
#define SQUAWK_PWM_PIN10 OCR4B
*/
#endif

#ifdef __AVR_ATmega168__
// Supported configurations for ATmega168
#define SQUAWK_PWM_PIN6  OCR0A
#define SQUAWK_PWM_PIN5  OCR0B
#define SQUAWK_PWM_PIN11 OCR2A
#define SQUAWK_PWM_PIN3  OCR2B
#endif

#ifdef __AVR_ATmega328P__
// Supported configurations for ATmega328P
#define SQUAWK_PWM_PIN6  OCR0A
#define SQUAWK_PWM_PIN5  OCR0B
#define SQUAWK_PWM_PIN11 OCR2A
#define SQUAWK_PWM_PIN3  OCR2B
#endif

/*
// NOT SUPPORTED YET
#define SQUAWK_SPI SPDR
#define SQUAWK_RLD_PORTB PORTB
#define SQUAWK_RLD_PORTC PORTC
*/

// oscillator structure
typedef struct {
  uint8_t  vol;
  uint16_t freq;
  uint16_t phase;
} Oscillator;

// oscillator memory
extern Oscillator osc[4];
// channel 0 is pulse wave
//    waveform has 25% duty
// channel 1 is pulse wave
//    waveform has 50% duty = square wave
// channel 2 is triangle wave
//    .vol is disregarded, always plays at full volume
// channel 3 is noise
//    .freq & .phase are used as a LFSR, should not be changed

// LFSR: Linear feedback shift register, a method of producing
// a pseudo-random bit sequence. Used to generate nasty noise.

// SAMPLE CRUNCHER
// generates samples and updates oscillators
// uses 3+112=115 cycles / 31.625% CPU @ 44kHz on 16MHz
#define SQUAWK_CONSTRUCT_ISR(TARGET_REGISTER) \
intptr_t squawk_register = (intptr_t)&TARGET_REGISTER; \
ISR(TIMER1_COMPA_vect, ISR_NAKED) { \
  asm volatile( \
    "push r18                                         " "\n\t" \
    "push r17                                         " "\n\t" \
    "push r16                                         " "\n\t" \
    "push r8                                          " "\n\t" \
    "push r5                                          " "\n\t" \
    "push r0                                          " "\n\t" \
    "in   r0,                    __SREG__             " "\n\t" \
    "lds  r18,                   osc+0*%[mul]+%[fre]  " "\n\t" \
    "lds  r8,                    osc+0*%[mul]+%[pha]  " "\n\t" \
    "add  r8,                    r18                  " "\n\t" \
    "sts  osc+0*%[mul]+%[pha],   r8                   " "\n\t" \
    "lds  r18,                   osc+0*%[mul]+%[fre]+1" "\n\t" \
    "lds  r5,                    osc+0*%[mul]+%[pha]+1" "\n\t" \
    "adc  r5,                    r18                  " "\n\t" \
    "sts  osc+0*%[mul]+%[pha]+1, r5                   " "\n\t" \
    "mov  r17,                   r5                   " "\n\t" \
    "lsl  r17                                         " "\n\t" \
    "and  r17,                   r5                   " "\n\t" \
    "lds  r16,                   osc+0*%[mul]+%[vol]  " "\n\t" \
    "sbrc r17,                   7                    " "\n\t" \
    "neg  r16                                         " "\n\t" \
    "lds  r18,                   osc+1*%[mul]+%[fre]  " "\n\t" \
    "lds  r8,                    osc+1*%[mul]+%[pha]  " "\n\t" \
    "add  r8,                    r18                  " "\n\t" \
    "sts  osc+1*%[mul]+%[pha],   r8                   " "\n\t" \
    "lds  r18,                   osc+1*%[mul]+%[fre]+1" "\n\t" \
    "lds  r5,                    osc+1*%[mul]+%[pha]+1" "\n\t" \
    "adc  r5,                    r18                  " "\n\t" \
    "sts  osc+1*%[mul]+%[pha]+1, r5                   " "\n\t" \
    "lds  r17,                   osc+1*%[mul]+%[vol]  " "\n\t" \
    "sbrc r5,                    7                    " "\n\t" \
    "neg  r17                                         " "\n\t" \
    "add  r16,                   r17                  " "\n\t" \
    "lds  r18,                   osc+2*%[mul]+%[fre]  " "\n\t" \
    "lds  r8,                    osc+2*%[mul]+%[pha]  " "\n\t" \
    "add  r8,                    r18                  " "\n\t" \
    "sts  osc+2*%[mul]+%[pha],   r8                   " "\n\t" \
    "lds  r18,                   osc+2*%[mul]+%[fre]+1" "\n\t" \
    "lds  r5,                    osc+2*%[mul]+%[pha]+1" "\n\t" \
    "adc  r5,                    r18                  " "\n\t" \
    "sts  osc+2*%[mul]+%[pha]+1, r5                   " "\n\t" \
    "mov  r17,                   r5                   " "\n\t" \
    "sbrc r17,                   7                    " "\n\t" \
    "com  r17                                         " "\n\t" \
    "lsr  r17                                         " "\n\t" \
    "add  r16,                   r17                  " "\n\t" \
    "ldi  r17,                   1                    " "\n\t" \
    "lds  r8,                    osc+3*%[mul]+%[fre]  " "\n\t" \
    "lds  r5,                    osc+3*%[mul]+%[fre]+1" "\n\t" \
    "add  r8,                    r8                   " "\n\t" \
    "adc  r5,                    r5                   " "\n\t" \
    "sbrc r5,                    7                    " "\n\t" \
    "eor  r8,                    r17                  " "\n\t" \
    "sbrc r5,                    6                    " "\n\t" \
    "eor  r8,                    r17                  " "\n\t" \
    "sts  osc+3*%[mul]+%[fre],   r8                   " "\n\t" \
    "sts  osc+3*%[mul]+%[fre]+1, r5                   " "\n\t" \
    "lds  r17,                   osc+3*%[mul]+%[vol]  " "\n\t" \
    "sbrc r5,                    7                    " "\n\t" \
    "neg  r17                                         " "\n\t" \
    "add  r16,                   r17                  " "\n\t" \
    "subi r16,                   160                  " "\n\t" \
    "sts  %[reg],                r16                  " "\n\t" \
    "out  __SREG__,              r0                   " "\n\t" \
    "pop  r0                                          " "\n\t" \
    "pop  r5                                          " "\n\t" \
    "pop  r8                                          " "\n\t" \
    "pop  r16                                         " "\n\t" \
    "pop  r17                                         " "\n\t" \
    "pop  r18                                         " "\n\t" \
    "reti                                             " "\n\t" \
    : \
    : [reg] "M" _SFR_MEM_ADDR(TARGET_REGISTER), \
      [mul] "M" (sizeof(Oscillator)), \
      [pha] "M" (offsetof(Oscillator, phase)), \
      [fre] "M" (offsetof(Oscillator, freq)), \
      [vol] "M" (offsetof(Oscillator, vol)) \
  ); \
}

#endif