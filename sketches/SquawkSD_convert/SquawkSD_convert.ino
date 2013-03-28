/* === SQUAWK SD-CARD MOD TO MELODY CONVERSION EXAMPLE === */

#include <Squawk.h>
#include <SD.h>
#include <SquawkSD.h>

/*
This sketch will convert a ProTracker module into a Squawk melody.
The input file *MUST BE* a proper ProTracker module following the
rules described in the README file.

This converter is provided for convenience only, but note that it
does not check for, or report any problems it encounters in the
input file, so you need to make sure the input file is properly
constructed, or it will result in more or less broken output.

If there is a problem with the conversion and you want to find
out why, you need to run the converter in the convert/ folder
on your PC instead, as it provides proper error checking
and reporting.
*/

// Chip select pin for SD card (it's 4 on the ethernet shield)
const int chipSelect = 4;

// File names for conversion
char inputFile[] = "protrack.mod";
char outputFile[] = "melody.sqm";

// Perform conversion
void setup() {
  File module, melody;
  // The default SD card chip select pin must be output
  pinMode(SS, OUTPUT);
  // Initialize the SD card  
  if (SD.begin(chipSelect)) {
    // Open input file
    module = SD.open(inputFile);
    if(module) {
      // If output file exists, remove it first
      if(SD.exists(outputFile)) SD.remove(outputFile);
      // Create and open output file for writing
      melody = SD.open(outputFile, FILE_WRITE);
      // Make sure file was opened successfully
      if(melody) {
        // Convert the file
        SquawkSD.convert(module, melody);
        // Close output file
        melody.close();
      }
      // Close input file
      module.close();
    }
  }
}

void loop() {
  // Do whatever you want
}
