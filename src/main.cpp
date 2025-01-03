#include <M5Atom.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

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


// WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// MQTT broker details
const char* switch_id = SWITCH_ID; // Unique switch ID
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char* mqtt_username = MQTT_USERNAME;  // Optional, if required
const char* mqtt_password = MQTT_PASSWORD;  // Optional, if required
static unsigned long lastPublish = 0;

WiFiClient espClient;
PubSubClient client(espClient);


// MQTT topic definitions
String state_topic = "m5/" + String(switch_id) + "/relay/0";
String command_topic = "m5/" + String(switch_id) + "/relay/0/command";

// List of snowflakes to skip as being part of icon
int snowflakeCheckList[] = {5, 6, 7, 10, 11, 12, 13, 15, 16, 17}; // if used instead of empty list then showflakes will not overlay icons; update the list each time icons are modified
//int snowflakeCheckList[] = {}; 
int checkListSize = sizeof(snowflakeCheckList) / sizeof(snowflakeCheckList[0]);

// Declare the updateDisplay function before setup and loop
void updateDisplay();

// Declare the updateSnowflakes function before setup and loop
void updateSnowflakes();


bool isPixelUsedByIcon(int snowflake_idx);



// Function to connect to WiFi
void setup_wifi() {
  delay(10);
  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// function to send out mqtt message with relay state called each time state changes (either as a result of pressing the button, receiving command or periodically)
void publishRelayState(){
  String state = relayState ? "on" : "off";
  client.publish(state_topic.c_str(), state.c_str(), 1);
}

// Callback function to handle received messages
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Check if the received message is "on" or "off" and set relay state
  if (String(topic) == String(command_topic)) {
    if (message == "on") {
      relayState = true;
      digitalWrite(relayPin, HIGH); // Turn relay ON
    } else if (message == "off") {
      relayState = false;
      digitalWrite(relayPin, LOW); // Turn relay OFF
    }
    publishRelayState();
  }
}

// Reconnect to MQTT server
void reconnect() {
  // Loop until we are connected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    //if (client.connect("M5StackClient", mqtt_username, mqtt_password)) {
    if (client.connect(switch_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      
      // Subscribe to the command topic
      client.subscribe(command_topic.c_str()); // Convert String to const char* using c_str()
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}



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


  // Start serial communication
  Serial.begin(115200);
  
  // Connect to WiFi
  setup_wifi();
  
  // Set up MQTT client
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Connect to MQTT server
  reconnect();
  if (client.connected()){
    publishRelayState();
  }
  
  client.setKeepAlive(60);

  // low power mode
  //WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true); 
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
    
    publishRelayState();
  }
  
  
  if (client.connected()){
    client.loop(); // Process incoming messages
  }
  else{
    reconnect();
  }

  // Periodically send the relay state to the MQTT topic
  unsigned long currentMillis = millis();
  if (currentMillis - lastPublish >= 10000) { // Publish every 10 seconds
    lastPublish = currentMillis;
    
    publishRelayState();
  }
  
  delay(50); // Simple debounce delay
  
  updateDisplay();
  
  if (relayState){
    updateSnowflakes();
  }
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

    int hexColor_white = int(r * currentBrightness) << 16 | int(g * currentBrightness) << 8 | int(b * currentBrightness); 

    r = 1;
    g = 0;
    b = 0;
    int hexColor_roof = int(r * currentBrightness) << 16 | int(g * currentBrightness) << 8 | int(b * currentBrightness); 

    hexColor_roof = hexColor_white;

    // house walls
    M5.dis.drawpix(5, hexColor_white);
    M5.dis.drawpix(6, hexColor_white);
    M5.dis.drawpix(7, hexColor_roof);

    M5.dis.drawpix(10, hexColor_white);
    //M5.dis.drawpix(11, hexColor_white);
    //M5.dis.drawpix(12, hexColor_roof);
    M5.dis.drawpix(13, hexColor_roof);

    M5.dis.drawpix(15, hexColor_white);
    M5.dis.drawpix(16, hexColor_white);
    M5.dis.drawpix(17, hexColor_roof);
    

    //M5.dis.drawpix(2, hexColor);
    //M5.dis.drawpix(8, hexColor_red);
    //M5.dis.drawpix(14, hexColor_red);
    //M5.dis.drawpix(18, hexColor_red);
    //M5.dis.drawpix(22, hexColor);


  } else {
    // Display pattern for "OFF" state (Red color for OFF)
    
    int r = 1;
    int g = 0;
    int b = 0;
    int currentBrightness = brightness_low;

    int hexColor = int(r * currentBrightness) << 16 | int(g * currentBrightness) << 8 | int(b * currentBrightness);

    M5.dis.drawpix(0, hexColor); 
    M5.dis.drawpix(4, hexColor); 
    M5.dis.drawpix(20, hexColor); 
    M5.dis.drawpix(24, hexColor); 
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
  for (int i = 0; i < checkListSize; i++) {
    if(snowflakePositions[snowflake_idx] == snowflakeCheckList[i]) {
      return true;
    }
  }
  return false;
}