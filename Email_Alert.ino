#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <HX711.h>
#include <WiFi.h>

#include <ESP_Mail_Client.h>

// Blynk Configuration - REPLACE THESE WITH YOUR ACTUAL VALUES
#define BLYNK_TEMPLATE_ID "TMPL3ylYV6pV1"
#define BLYNK_TEMPLATE_NAME "Structure Durability"
#define BLYNK_AUTH_TOKEN "aFsdcFj7BcBqDDdp7-Of5vBQPaLmTVJn"
#include <BlynkSimpleEsp32.h>

// WiFi credentials - UPDATE THESE
#define WIFI_SSID "Ayoo"
#define WIFI_PASSWORD "shri123er"

// Virtual Pins for Blynk
#define VPIN_WEIGHT V0
#define VPIN_TEMPERATURE V1
#define VPIN_HUMIDITY V2
#define VPIN_VIBRATION_STATUS V3
#define VPIN_VIBRATION_COUNT V4
#define VPIN_TILT_STATUS V5
#define VPIN_TILT_ANGLE V6
#define VPIN_TILT_DURATION V7
#define VPIN_SYSTEM_STATUS V8
#define VPIN_NOTIFICATION_LOG V9
#define VPIN_RESET_VIBRATION V10
#define VPIN_CALIBRATE_SCALE V11
#define VPIN_THRESHOLD_VIBRATION V12
#define VPIN_THRESHOLD_TILT V13

// Forward declarations
void IRAM_ATTR vibrationISR();

// Pin Definitions
#define HX711_DOUT_PIN 25
#define HX711_SCK_PIN 26
#define VIBRATION_DIGITAL_PIN 34  // SW420 digital output
#define TILT_SENSOR_PIN 14        // SW18010P tilt sensor
#define TILT_ANALOG_PIN 32        // Analog pin for tilt sensor
#define DHT_PIN 27
#define DHT_TYPE DHT11

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Initialize Objects
HX711 scale;
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
BlynkTimer timer;

// Variables for sensor readings
float weight = 0.0;
float temperature = 0.0;
float humidity = 0.0;
bool vibrationDetected = false;
int vibrationValue = 0;
unsigned long vibrationCount = 0;
bool tiltState = false;
unsigned long lastTiltChange = 0;
int tiltAnalogValue = 0;
int tiltAngle = 0;

// Variables for tilt sensor debouncing
bool currentTiltReading = false;
bool lastTiltReading = false;
unsigned long lastTiltDebounceTime = 0;

// Variables for display page control
int displayPage = 0;
unsigned long lastPageChange = 0;
const int PAGE_CHANGE_INTERVAL = 3000;

// Timing control for sensor readings
unsigned long lastSensorReadTime = 0;
const int SENSOR_READ_INTERVAL = 2000; // Read sensors every 2 seconds

// Debounce settings
const unsigned long DEBOUNCE_DELAY = 50;

// Email notification settings - UPDATE THESE WITH YOUR EMAIL CREDENTIALS
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "hegtest1vina@gmail.com"
#define AUTHOR_PASSWORD "hisw avgx djxk eaxv"
#define RECIPIENT_EMAIL "hegdevinayak887@gmail.com"

// Threshold settings for notifications (can be adjusted via Blynk)
int VIBRATION_THRESHOLD = 1000;
int TILT_ANGLE_THRESHOLD = 2;
int TILT_DURATION_THRESHOLD = 1;
#define EMAIL_COOLDOWN 300000     // 5 minutes between emails

// Session and message objects for email
SMTPSession smtp;
SMTP_Message message;

// Email notification variables
unsigned long lastVibrationNotification = 0;
unsigned long lastTiltNotification = 0;
bool vibrationAlertSent = false;
bool tiltAlertSent = false;
bool emailConfigured = true;

// Blynk variables
String systemStatus = "Initializing...";
String lastNotification = "System Started";

// Blynk Virtual Pin Handlers

// Reset vibration counter
BLYNK_WRITE(VPIN_RESET_VIBRATION) {
  if (param.asInt() == 1) {
    vibrationCount = 0;
    vibrationAlertSent = false;
    Blynk.virtualWrite(VPIN_VIBRATION_COUNT, 0);
    Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, "Vibration counter reset");
    Serial.println("Vibration counter reset via Blynk");
  }
}

// Scale calibration trigger
BLYNK_WRITE(VPIN_CALIBRATE_SCALE) {
  if (param.asInt() == 1) {
    calibrateScale();
    Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, "Scale calibration initiated");
  }
}

// Vibration threshold adjustment
BLYNK_WRITE(VPIN_THRESHOLD_VIBRATION) {
  VIBRATION_THRESHOLD = param.asInt();
  Serial.print("Vibration threshold updated to: ");
  Serial.println(VIBRATION_THRESHOLD);
  String msg = "Vibration threshold set to " + String(VIBRATION_THRESHOLD);
  Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, msg);
}

// Tilt threshold adjustment
BLYNK_WRITE(VPIN_THRESHOLD_TILT) {
  TILT_ANGLE_THRESHOLD = param.asInt();
  Serial.print("Tilt threshold updated to: ");
  Serial.println(TILT_ANGLE_THRESHOLD);
  String msg = "Tilt threshold set to " + String(TILT_ANGLE_THRESHOLD) + "°";
  Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, msg);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Structural Durability Project with Blynk Starting...");
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Display allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Initializing...");
  display.display();
  delay(2000);
  
  // Initialize HX711 scale
  initializeScale();
  
  // Initialize DHT sensor
  dht.begin();
  
  // Setup input pins
  pinMode(VIBRATION_DIGITAL_PIN, INPUT);
  pinMode(TILT_SENSOR_PIN, INPUT_PULLUP);
  pinMode(TILT_ANALOG_PIN, INPUT);
  
  // Attach interrupt for vibration sensor
  attachInterrupt(digitalPinToInterrupt(VIBRATION_DIGITAL_PIN), vibrationISR, FALLING);
  
  // Connect to WiFi and Blynk
  setupWiFiAndBlynk();
  
  // Initialize Email settings
  setupEmail();
  
  // Setup Blynk timers
  timer.setInterval(2000L, sendSensorDataToBlynk); // Send data every 2 seconds
  timer.setInterval(5000L, updateSystemStatus);     // Update status every 5 seconds
  
  // Initialize timing variables
  lastTiltChange = millis();
  lastTiltDebounceTime = millis();
  lastPageChange = millis();
  lastSensorReadTime = millis();
  
  // Initialize Blynk widgets
  initializeBlynkWidgets();
  
  systemStatus = "System Ready";
  Serial.println("Setup complete!");
}

void initializeScale() {
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  if (scale.wait_ready_timeout(1000)) {
    Serial.println("HX711 found. Performing initial calibration...");
    calibrateScale();
  } else {
    Serial.println("HX711 not found");
    systemStatus = "ERROR: HX711 not found";
  }
}

void calibrateScale() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Scale Calibration");
  display.println("Remove all weight");
  display.display();
  delay(3000);
  
  scale.tare();
  Serial.println("Scale tared");
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Add known weight");
  display.println("(100g)");
  display.println("Wait 5 seconds...");
  display.display();
  delay(5000);
  
  long reading = scale.get_value(10);
  float knownWeight = 100.0;
  float calibrationFactor = reading / knownWeight;
  scale.set_scale(calibrationFactor);
  
  Serial.print("Calibration complete. Factor: ");
  Serial.println(calibrationFactor);
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibration done!");
  display.display();
  delay(2000);
}

void setupWiFiAndBlynk() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Connected!");
    display.println("Connecting Blynk...");
    display.display();
    
    // Connect to Blynk
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect();
    
    if (Blynk.connected()) {
      Serial.println("Blynk Connected!");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Blynk Connected!");
      display.display();
      delay(2000);
    } else {
      Serial.println("Blynk connection failed");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Blynk Failed!");
      display.display();
      delay(2000);
    }
  } else {
    Serial.println("WiFi connection failed");
    systemStatus = "WiFi Connection Failed";
  }
}

void initializeBlynkWidgets() {
  // Set initial values for Blynk widgets
  Blynk.virtualWrite(VPIN_WEIGHT, 0);
  Blynk.virtualWrite(VPIN_TEMPERATURE, 0);
  Blynk.virtualWrite(VPIN_HUMIDITY, 0);
  Blynk.virtualWrite(VPIN_VIBRATION_STATUS, 0);
  Blynk.virtualWrite(VPIN_VIBRATION_COUNT, 0);
  Blynk.virtualWrite(VPIN_TILT_STATUS, 0);
  Blynk.virtualWrite(VPIN_TILT_ANGLE, 0);
  Blynk.virtualWrite(VPIN_TILT_DURATION, 0);
  Blynk.virtualWrite(VPIN_SYSTEM_STATUS, systemStatus);
  Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, "System initialized");
  Blynk.virtualWrite(VPIN_THRESHOLD_VIBRATION, VIBRATION_THRESHOLD);
  Blynk.virtualWrite(VPIN_THRESHOLD_TILT, TILT_ANGLE_THRESHOLD);
}

void setupEmail() {
  smtp.debug(1);
  smtp.callback(smtpCallback);
  
  if (strlen(AUTHOR_EMAIL) == 0 || strlen(AUTHOR_PASSWORD) == 0 || strlen(RECIPIENT_EMAIL) == 0) {
    Serial.println("WARNING: Email credentials not configured!");
    emailConfigured = false;
    systemStatus = "Email not configured";
  } else {
    Serial.println("Email configuration loaded");
    emailConfigured = true;
  }
}

void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
  if (status.success()) {
    Serial.println("Email sent successfully");
    lastNotification = "Email sent successfully";
  } else {
    Serial.println("Failed to send email");
    lastNotification = "Email failed to send";
  }
  Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, lastNotification);
}

void sendEmailAlert(const char* subject, const char* content) {
  if (!emailConfigured) {
    Serial.println("Email not configured");
    lastNotification = "Email not configured";
    Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, lastNotification);
    return;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    lastNotification = "WiFi disconnected - Email failed";
    Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, lastNotification);
    return;
  }
  
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";
  
  message.sender.name = "Structural Monitor";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = subject;
  message.addRecipient("User", RECIPIENT_EMAIL);
  message.text.content = content;
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_high;
  
  if (!smtp.connect(&session)) {
    Serial.println("Error connecting to SMTP server");
    lastNotification = "SMTP connection failed";
    Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, lastNotification);
    return;
  }
  
  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Error sending Email");
    lastNotification = "Email sending failed";
  } else {
    Serial.println("Email sent successfully");
    lastNotification = "Alert email sent";
    // Send Blynk notification as well
    Blynk.logEvent("structural_alert", String(subject) + ": " + String(content));
  }
  Blynk.virtualWrite(VPIN_NOTIFICATION_LOG, lastNotification);
}

void IRAM_ATTR vibrationISR() {
  vibrationCount++;
  vibrationDetected = true;
}

void loop() {
  Blynk.run();
  timer.run();
  
  // Read sensors at controlled intervals
  if (millis() - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    readSensors();
    checkAlertConditions();
    printSerialData();
    lastSensorReadTime = millis();
  }
  
  // Update display
  if (millis() - lastPageChange > PAGE_CHANGE_INTERVAL) {
    displayPage = (displayPage + 1) % 3;
    lastPageChange = millis();
  }
  updateDisplay();
  
  // Auto-reset vibration detected flag
  static unsigned long lastVibrationReset = 0;
  if (vibrationDetected && (millis() - lastVibrationReset > 5000)) {
    vibrationDetected = false;
    lastVibrationReset = millis();
  }
  
  delay(50);
}

void readSensors() {
  // Read HX711 (Load Cell)
  if (scale.wait_ready_timeout(200)) {
    weight = scale.get_units(5);
  }
  
  // Read DHT11 (Temperature & Humidity)
  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature();
  
  if (!isnan(newHumidity)) humidity = newHumidity;
  if (!isnan(newTemperature)) temperature = newTemperature;
  
  // Read SW420 Vibration Sensor
  if (digitalRead(VIBRATION_DIGITAL_PIN) == LOW) {
    vibrationDetected = true;
  }
  
  // Read Analog Tilt Sensor
  tiltAnalogValue = analogRead(TILT_ANALOG_PIN);
  tiltAngle = map(tiltAnalogValue, 0, 4095, 0, 180);
  
  // Read digital tilt sensor with debounce
  bool currentTiltReading = digitalRead(TILT_SENSOR_PIN) == LOW;
  
  if (currentTiltReading != lastTiltReading) {
    lastTiltDebounceTime = millis();
  }
  
  if ((millis() - lastTiltDebounceTime) > DEBOUNCE_DELAY) {
    if (currentTiltReading != tiltState) {
      tiltState = currentTiltReading;
      if (tiltState) {
        lastTiltChange = millis();
      }
    }
  }
  
  lastTiltReading = currentTiltReading;
}

void sendSensorDataToBlynk() {
  if (Blynk.connected()) {
    Blynk.virtualWrite(VPIN_WEIGHT, weight);
    Blynk.virtualWrite(VPIN_TEMPERATURE, temperature);
    Blynk.virtualWrite(VPIN_HUMIDITY, humidity);
    Blynk.virtualWrite(VPIN_VIBRATION_STATUS, vibrationDetected ? 1 : 0);
    Blynk.virtualWrite(VPIN_VIBRATION_COUNT, vibrationCount);
    Blynk.virtualWrite(VPIN_TILT_STATUS, tiltState ? 1 : 0);
    Blynk.virtualWrite(VPIN_TILT_ANGLE, tiltAngle);
    
    if (tiltState) {
      unsigned long tiltDuration = (millis() - lastTiltChange) / 1000;
      Blynk.virtualWrite(VPIN_TILT_DURATION, tiltDuration);
    } else {
      Blynk.virtualWrite(VPIN_TILT_DURATION, 0);
    }
  }
}

void updateSystemStatus() {
  if (Blynk.connected()) {
    if (WiFi.status() == WL_CONNECTED) {
      systemStatus = "All systems operational";
    } else {
      systemStatus = "WiFi connection lost";
    }
    Blynk.virtualWrite(VPIN_SYSTEM_STATUS, systemStatus);
  }
}

void checkAlertConditions() {
  // Check vibration threshold
  if (vibrationCount >= VIBRATION_THRESHOLD && 
      !vibrationAlertSent && 
      (millis() - lastVibrationNotification > EMAIL_COOLDOWN)) {
    
    char subject[64];
    char content[256];
    
    snprintf(subject, sizeof(subject), "ALERT: High Vibration Detected");
    snprintf(content, sizeof(content), 
             "Vibration threshold exceeded!\n\n"
             "Current vibration count: %lu\n"
             "Threshold: %d\n"
             "Temperature: %.1f C\n"
             "Humidity: %.1f %%\n"
             "Weight: %.1f g\n\n"
             "Check your Blynk app for real-time monitoring.",
             vibrationCount, VIBRATION_THRESHOLD, temperature, humidity, weight);
    
    sendEmailAlert(subject, content);
    lastVibrationNotification = millis();
    vibrationAlertSent = true;
    
    // Send Blynk notification
    Blynk.logEvent("vibration_alert", "High vibration detected! Count: " + String(vibrationCount));
  }
  
  if (vibrationCount < VIBRATION_THRESHOLD) {
    vibrationAlertSent = false;
  }
  
  // Check tilt threshold
  unsigned long tiltDuration = tiltState ? (millis() - lastTiltChange) / 1000 : 0;
  
  if (tiltState && 
      tiltAngle >= TILT_ANGLE_THRESHOLD && 
      tiltDuration >= TILT_DURATION_THRESHOLD &&
      !tiltAlertSent && 
      (millis() - lastTiltNotification > EMAIL_COOLDOWN)) {
    
    char subject[64];
    char content[256];
    
    snprintf(subject, sizeof(subject), "ALERT: Critical Tilt Detected");
    snprintf(content, sizeof(content), 
             "Tilt threshold exceeded!\n\n"
             "Current tilt angle: %d degrees\n"
             "Angle threshold: %d degrees\n"
             "Tilt duration: %lu seconds\n"
             "Temperature: %.1f C\n"
             "Humidity: %.1f %%\n\n"
             "Check your Blynk app for real-time monitoring.",
             tiltAngle, TILT_ANGLE_THRESHOLD, tiltDuration, temperature, humidity);
    
    sendEmailAlert(subject, content);
    lastTiltNotification = millis();
    tiltAlertSent = true;
    
    // Send Blynk notification
    Blynk.logEvent("tilt_alert", "Critical tilt detected! Angle: " + String(tiltAngle) + "°");
  }
  
  if (!tiltState || tiltAngle < TILT_ANGLE_THRESHOLD) {
    tiltAlertSent = false;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  
  switch(displayPage) {
    case 0: // Weight and Temperature
      display.setCursor(0, 0);
      display.print("Weight: ");
      display.print(weight, 1);
      display.println(" g");
      
      display.setCursor(0, 12);
      display.print("Temp: ");
      display.print(temperature, 1);
      display.println(" C");
      
      display.setCursor(0, 24);
      display.print("Humidity: ");
      display.print(humidity, 0);
      display.println("%");
      break;
      
    case 1: // Vibration
      display.setCursor(0, 0);
      display.print("Vibration: ");
      display.println(vibrationDetected ? "DETECTED!" : "Normal");
      
      display.setCursor(0, 12);
      display.print("Count: ");
      display.println(vibrationCount);
      
      display.setCursor(0, 24);
      display.print("Blynk: ");
      display.println(Blynk.connected() ? "Connected" : "Offline");
      break;
      
    case 2: // Tilt
      display.setCursor(0, 0);
      display.print("Tilt: ");
      display.println(tiltState ? "ACTIVE" : "NORMAL");
      
      if (tiltState) {
        display.setCursor(0, 12);
        display.print("Duration: ");
        display.print((millis() - lastTiltChange) / 1000);
        display.println("s");
      }
      
      display.setCursor(0, 24);
      display.print("Angle: ");
      display.print(tiltAngle);
      display.println(" deg");
      break;
  }
  
  display.display();
}

void printSerialData() {
  Serial.println("------ SENSOR READINGS ------");
  Serial.print("Weight: "); Serial.print(weight); Serial.println(" g");
  Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("Vibration: "); Serial.println(vibrationDetected ? "DETECTED!" : "Normal");
  Serial.print("Vibration Count: "); Serial.println(vibrationCount);
  Serial.print("Tilt State: "); Serial.println(tiltState ? "ACTIVE" : "NORMAL");
  Serial.print("Tilt Angle: "); Serial.print(); Serial.println(" degrees");
  Serial.print("Blynk Status: "); Serial.println(Blynk.connected() ? "Connected" : "Disconnected");
  Serial.println("----------------------------");
}