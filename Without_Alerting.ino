#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <HX711.h>

// Pin Definitions
#define HX711_DOUT_PIN 25
#define HX711_SCK_PIN 26
#define VIBRATION_SENSOR_PIN 34
#define TILT_SENSOR_PIN 14
#define DHT_PIN 27
#define DHT_TYPE DHT11

// OLED Display Configuration - Adjusted for 128x32 display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32  // Changed to 32 pixels height
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address (typical: 0x3C or 0x3D)

// Initialize Objects
HX711 scale;
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variables for sensor readings
float weight = 0.0;
float temperature = 0.0;
float humidity = 0.0;
int vibration = 0;
bool tiltState = false;

// Variables for display page control
int displayPage = 0;
unsigned long lastPageChange = 0;
const int PAGE_CHANGE_INTERVAL = 3000; // 3 seconds per page

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 IoT Project Starting...");
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Initializing...");
  display.display();
  delay(2000);
  
  // Initialize HX711 scale
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  if (scale.wait_ready_timeout(1000)) {
    scale.set_scale(2280.f); // This value needs calibration for your specific load cell
    scale.tare();
    Serial.println("HX711 initialized");
  } else {
    Serial.println("HX711 not found");
  }
  
  // Initialize DHT sensor
  dht.begin();
  
  // Setup input pins
  pinMode(VIBRATION_SENSOR_PIN, INPUT);
  pinMode(TILT_SENSOR_PIN, INPUT_PULLUP);
  
  // Show startup message
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System Ready!");
  display.display();
  delay(1000);
}

void loop() {
  // Read all sensor values
  readSensors();
  
  // Check if it's time to change the display page
  if (millis() - lastPageChange > PAGE_CHANGE_INTERVAL) {
    displayPage = (displayPage + 1) % 3; // Cycle through 3 display pages
    lastPageChange = millis();
  }
  
  // Display sensor values on the OLED
  updateDisplay();
  
  // Print values to Serial for debugging
  printSerialData();
  
  // Wait before next reading
  delay(500);
}

void readSensors() {
  // Read HX711 (Load Cell)
  if (scale.wait_ready_timeout(200)) {
    weight = scale.get_units(5); // Average of 5 readings for stability
  }
  
  // Read DHT11 (Temperature & Humidity)
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  // Read Vibration Sensor (analog)
  vibration = analogRead(VIBRATION_SENSOR_PIN);
  
  // Read Tilt Sensor (digital)
  tiltState = !digitalRead(TILT_SENSOR_PIN); // Assuming LOW when tilted
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  
  // Split data across multiple pages due to limited screen height
  switch(displayPage) {
    case 0: // Page 1: Weight and Temperature
      display.setCursor(0, 0);
      display.print("Weight: ");
      display.print(weight, 1);
      display.println(" g");
      
      display.setCursor(0, 16);
      display.print("Temp: ");
      display.print(temperature, 1);
      display.println(" C");
      break;
      
    case 1: // Page 2: Humidity and Vibration
      display.setCursor(0, 0);
      display.print("Humidity: ");
      display.print(humidity, 0);
      display.println("%");
      
      display.setCursor(0, 16);
      display.print("Vibration: ");
      display.println(vibration);
      break;
      
    case 2: // Page 3: Tilt state and system status
      display.setCursor(0, 0);
      display.print("Tilt: ");
      display.println(tiltState ? "ACTIVE" : "NORMAL");
      
      display.setCursor(0, 16);
      display.print("System: ONLINE");
      
      // Add small page indicator
      display.setCursor(120, 24);
      display.print(displayPage+1);
      break;
  }
  
  // Small page indicator
  display.setCursor(120, 0);
  display.print(displayPage+1);
  display.print("/3");
  
  display.display();
}

void printSerialData() {
  Serial.println("------ SENSOR READINGS ------");
  Serial.print("Weight: ");
  Serial.print(weight);
  Serial.println(" g");
  
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  
  Serial.print("Vibration: ");
  Serial.println(vibration);
  
  Serial.print("Tilt State: ");
  Serial.println(tiltState ? "ACTIVE" : "NORMAL");
  Serial.println("----------------------------");
}