// 1. Mandatory Blynk Template Credentials
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxx"
#define BLYNK_TEMPLATE_NAME "CableFaultLocator"
#define BLYNK_AUTH_TOKEN "QeILhZucLpZcZ5byWD8mxgMq8XXXvYbb"

// 2. Hardware Architecture Library Initializations
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>              // Native ESP32 Wi-Fi communication stack 
#include <BlynkSimpleEsp32.h>  // Blynk Cloud telemetry bridge library for ESP32

// Initialize physical I2C screen. Standard hardware modules use 0x27 or 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Configurations mapped to ESP32 (30-Pin) layout
const int sensorPin = 36;  // Routes to 'SVP' on your board
const int buzzerPin = 14;  // Routes to 'P14' on your board

// Wireless Network Settings
char ssid[] = "Shreya";     // Your smartphone hotspot name
char pass[] = "shreya123";        // Your Wi-Fi network password

// Operational System Memory Storage Flags
int lastSentDistance = 0; 

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  
  Wire.begin(21, 22); 
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" ESP32 SYSTEM   ");
  lcd.setCursor(0, 1);
  lcd.print("CONNECTING WIFI.");
  
  // Manually start the Wi-Fi connection first
  WiFi.begin(ssid, pass);
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WIFI CONNECTED! ");
  lcd.setCursor(0, 1);
  lcd.print("STARTING BLYNK..");
  delay(1000);

  // Initialize Blynk with a timeout config to prevent hard-freezing
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect(3000); // Attempt connection to cloud for 3 seconds max
  
  lcd.clear();
  Serial.println("--- SETUP COMPLETE ---");
}

void loop() {
  Blynk.run(); // Keeps the wireless internet connection and app sync alive
  
  int adcReading = analogRead(sensorPin);
  lcd.setCursor(0, 0);
  
  /* 
   * NOTE: The ESP32 utilizes a highly precise 12-bit ADC (0 to 4095 steps)
   * compared to the Arduino's old 10-bit converter (0 to 1023). 
   * The threshold ranges below are mathematically scaled up x4 to match the 12-bit step values.
   */
  if (adcReading > 3800) {
    digitalWrite(buzzerPin, LOW);
    lcd.print("STATUS: SECURE  ");
    lcd.setCursor(0, 1);
    lcd.print("LINE IS CLEAR   ");
    
    if (lastSentDistance != 0) {
      Blynk.virtualWrite(V1, "Line Status: Fully Operational"); 
      lastSentDistance = 0;
    }
  } 
  else {
    digitalWrite(buzzerPin, HIGH); // Sound local alarm
    lcd.print("STATUS: FAULT!! ");
    lcd.setCursor(0, 1);
    
    int detectedKM = 0;
    
    // Process 12-bit scaled resistor thresholds
    if (adcReading >= 1900 && adcReading <= 2200)       detectedKM = 1; // Expected ~2048
    else if (adcReading >= 2600 && adcReading <= 2900)  detectedKM = 2; // Expected ~2730
    else if (adcReading >= 2950 && adcReading <= 3200)  detectedKM = 3; // Expected ~3072
    else if (adcReading >= 3210 && adcReading <= 3450)  detectedKM = 4; // Expected ~3276
    
    if (detectedKM != 0) {
      lcd.print("DISTANCE: " + String(detectedKM) + " KM  ");
      
      // Anti-Flooding Filter: Only push notification if it is a NEW fault entry
      if (lastSentDistance != detectedKM) {
        String alertPayload = "CRITICAL FAULT DETECTED AT " + String(detectedKM) + " KM";
        
        Blynk.virtualWrite(V1, alertPayload); 
        Blynk.logEvent("cable_fault_alert", alertPayload); 
        
        Serial.println("📡 ESP32 Telemetry Transmitted: [ " + alertPayload + " ]");
        lastSentDistance = detectedKM;
      }
    } else {
      lcd.print("UNKNOWN LOCATION");
    }
  }
  delay(100); 
}
