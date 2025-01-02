#include <M5Atom.h>

const int relayPin = 26; // Relay connected to PH2 pin
bool relayState = false; // Default mode is off
int brightness_low = 32;
int brightness_high = 48;
int brightness_snowflake = 200;

const int cycle_duration = 2000; // Full dimming cycle duration (in milliseconds)
const int total_pixels = 25; // Total number of pixels (5x5 matrix)
int snowflakePositions[total_pixels]; // Array to store random positions for snowflakes
unsigned long lastUpdate = 0; // Store the time of the last update
unsigned long startTime = 0; // Start time for dimming cycle
int currentSnowflake = -1; // Current snowflake index
int currentBrightness = 0; // Current brightness level of the snowflake

// List of snowflakes to skip as being part of icon
//int snowflakeCheckList[] = {5, 10, 15, 6, 7, 16, 17, 13, 0, 4, 20, 24}; 
int snowflakeCheckList[] = {}; 
int checkListSize = sizeof(snowflakeCheckList) / sizeof(snowflakeCheckList[0]);

// Declare the updateDisplay function before setup and loop
void updateDisplay();

// Declare the updateSnowflakes function before setup and loop
void updateSnowflakes();


bool isPixelUsedByIcon(int snowflake_idx);


void setup() {
  M5.begin(true, false, true); // Initialize serial, I2C, and display
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Ensure relay starts off
  M5.dis.clear(); // Clear the 5x5 LED matrix
  M5.dis.setBrightness(200);
  updateDisplay(); // Display initial state

  randomSeed(analogRead(0)); // Seed the random number generator

  // Generate random positions for the snowflakes
  for (int i = 0; i < total_pixels; i++) {
    snowflakePositions[i] = i; // Assign pixel indices (0 to 24)
  }

  // Shuffle snowflake positions for randomness
  for (int i = 0; i < total_pixels; i++) {
    int j = random(total_pixels);
    int temp = snowflakePositions[i];
    snowflakePositions[i] = snowflakePositions[j];
    snowflakePositions[j] = temp;
  }

}

void loop() {
  M5.update(); // Update the M5Atom library to check for button press

  if (M5.Btn.wasPressed()) {
    // Toggle relay state
    relayState = !relayState;
    
    if (relayState) {
      digitalWrite(relayPin, HIGH); // Turn relay ON
      //M5.dis.setBrightness(brightness_high);
    } else {
      digitalWrite(relayPin, LOW); // Turn relay OFF
      //M5.dis.setBrightness(brightness_low);
    }
    updateDisplay(); // Update display based on new state
  }
  delay(100); // Simple debounce delay
  
  updateDisplay();
  updateSnowflakes();
  
}

void updateDisplay() {
  // Clear the 5x5 matrix display
  M5.dis.clear();

  // Show the relay state on the 5x5 LED matrix
  if (relayState) {
    int r = 1;
    int g = 1;
    int b = 1;
    int currentBrightness = brightness_high;

    int hexColor = int(r * currentBrightness) << 16 | int(g * currentBrightness) << 8 | int(b * currentBrightness); 

    // Display pattern for "ON" state (Green color for ON)
    M5.dis.drawpix(5, hexColor);
    M5.dis.drawpix(10, hexColor);
    M5.dis.drawpix(15, hexColor);

    M5.dis.drawpix(6, hexColor);
    M5.dis.drawpix(7, hexColor);

    M5.dis.drawpix(16, hexColor);
    M5.dis.drawpix(17, hexColor);
    
    M5.dis.drawpix(13, hexColor);

  } else {
    // Display pattern for "OFF" state (Red color for OFF)
    
    int r = 1;
    int g = 0;
    int b = 0;
    int currentBrightness = brightness_low;

    int hexColor = int(r * currentBrightness) << 16 | int(g * currentBrightness) << 8 | int(b * currentBrightness);

    M5.dis.drawpix(0, hexColor); // Red
    M5.dis.drawpix(4, hexColor); // Red
    M5.dis.drawpix(20, hexColor); // Red
    M5.dis.drawpix(24, hexColor); // Red
  }
}

void updateSnowflakes() {
  unsigned long currentTime = millis();
  
  if (currentSnowflake >= 0) {
    // Calculate the brightness based on the cycle, making the fade in and out effect`  `
    int elapsedTime = currentTime - startTime;
    float brightnessFactor = (sin((elapsedTime / (float)cycle_duration) * TWO_PI / 2.0)); // Sine wave for fade in and out
    currentBrightness = int(brightnessFactor * brightness_snowflake); // Update brightness based on the sine wave

    // Convert current brightness to hex RGB value
    int hexColor = currentBrightness << 16 | currentBrightness << 8 | currentBrightness; // Same R, G, B value for grayscale
    
    // Update the brightness of the current snowflake
    M5.dis.drawpix(snowflakePositions[currentSnowflake], hexColor); // Set pixel with hex color
    
    // Check if the dimming cycle is complete
    if (elapsedTime >= cycle_duration) {
      // Move to the next snowflake
      M5.dis.drawpix(snowflakePositions[currentSnowflake], 0x000000);  // make sure last snowflake pixel is turned off
      currentSnowflake++;
      if (currentSnowflake >= total_pixels) {
              currentSnowflake = 0; // Start over if all positions are used
            } 
      bool isPixelUsed = isPixelUsedByIcon(currentSnowflake);
      while(isPixelUsed){
        currentSnowflake++;
        if (currentSnowflake >= total_pixels) {
              currentSnowflake = 0; // Start over if all positions are used
        }
        isPixelUsed = isPixelUsedByIcon(currentSnowflake);
      }

      startTime = currentTime; // Reset the start time for the next snowflake
      //M5.dis.clear(); // Clear the screen
    }
  
  }

  // If no snowflake is currently active, start the first snowflake
  if (currentSnowflake == -1) {
    currentSnowflake = 0;
    startTime = currentTime; // Start the cycle for the first snowflake
    //M5.dis.clear(); // Clear the display
  }
  
}

bool isPixelUsedByIcon(int snowflake_idx){
  bool result = false;
  for (int i = 0; i < checkListSize; i++) {
    if(snowflakePositions[snowflake_idx] == snowflakeCheckList[i]) {
      result = true;
    }
  }
  return result;
}