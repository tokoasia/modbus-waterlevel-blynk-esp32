// Blynk 2.0 authentication
#define BLYNK_TEMPLATE_ID "TMPL6ZbF9qA5N"
#define BLYNK_TEMPLATE_NAME "TOKOASIA DEMO"
#define BLYNK_AUTH_TOKEN "Nk3DHUe50EIQpywlZmCZ2uVwsZs4m33Z"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include <ModbusMaster.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Your WiFi credentials
char ssid[] = "Suharno_2.4GHz";
char pass[] = "90262719";

// Define the MAX485 control pins
#define MAX485_DE 5
#define MAX485_RE 4

// Initialize I2C LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Instantiate ModbusMaster object
ModbusMaster node;

// Calibration variables - CHANGE THESE VALUES!
int emptyTankMM = 12;      // Set this to your empty tank reading
int fullTankMM = 143;      // Set this to your full tank reading

// Sensor variables
int decimalPlaces = 0;
int measurementUnit = 0;
int currentLevelMM = 0;
float currentPercentage = 0;

// Blynk virtual pins
#define BLYNK_VIRTUAL_PIN_LEVEL V0
#define BLYNK_VIRTUAL_PIN_PERCENTAGE V1
#define BLYNK_VIRTUAL_PIN_CAL_STATUS V2

BlynkTimer timer;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C LCD
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Calibration Mode");
  
  // Initialize Serial2 for Modbus
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // Pin modes for MAX485
  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE, OUTPUT);
  digitalWrite(MAX485_DE, LOW);
  digitalWrite(MAX485_RE, LOW);
  
  // Initialize Modbus
  node.begin(1, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // Connect to WiFi and Blynk
  connectToWiFi();
  
  // Setup timer
  timer.setInterval(2000L, sendDataToBlynk);
  
  Serial.println("=== PRESSURE SENSOR CALIBRATION ===");
  Serial.println("Current Calibration Values:");
  Serial.print("Empty Tank: "); Serial.print(emptyTankMM); Serial.println(" mm");
  Serial.print("Full Tank: "); Serial.print(fullTankMM); Serial.println(" mm");
  Serial.println("Commands: L=Set Low, H=Set High, R=Reset, P=Print");
  Serial.println("===================================");
}

void loop() {
  Blynk.run();
  timer.run();
  
  readSensorData();
  updateLCD();
  handleSerialCommands();
  
  delay(1000);
}

void connectToWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi:Connecting");
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi:Connected!");
    Serial.println("WiFi connected!");
    delay(1000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi:Failed!");
    Serial.println("WiFi connection failed - continuing offline");
    delay(1000);
  }
}

void readSensorData() {
  // Read sensor configuration
  if (node.readHoldingRegisters(3, 1) == node.ku8MBSuccess) {
    decimalPlaces = node.getResponseBuffer(0);
  }
  
  if (node.readHoldingRegisters(2, 1) == node.ku8MBSuccess) {
    measurementUnit = node.getResponseBuffer(0);
  }
  
  // Read current level
  if (node.readHoldingRegisters(4, 1) == node.ku8MBSuccess) {
    currentLevelMM = (int16_t)node.getResponseBuffer(0);
    currentPercentage = calculatePercentage(currentLevelMM);
    
    Serial.print("Level: ");
    Serial.print(currentLevelMM);
    Serial.print(" mm | ");
    Serial.print(currentPercentage, 1);
    Serial.println("%");
  } else {
    Serial.println("Error reading sensor");
    currentLevelMM = -1;
    currentPercentage = -1;
  }
}

float calculatePercentage(int levelMM) {
  if (fullTankMM <= emptyTankMM) return 0; // Prevent division by zero
  
  float percentage = ((levelMM - emptyTankMM) / (float)(fullTankMM - emptyTankMM)) * 100.0;
  
  // Constrain to 0-100%
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  
  return percentage;
}

void updateLCD() {
  lcd.clear();
  
  // Line 1: Current reading
  lcd.setCursor(0, 0);
  if (currentLevelMM >= 0) {
    lcd.print("L:");
    lcd.print(currentLevelMM);
    lcd.print("mm ");
    
    if (Blynk.connected()) {
      lcd.print("B:OK");
    } else {
      lcd.print("B:OFF");
    }
  } else {
    lcd.print("Sensor Error!");
  }
  
  // Line 2: Calibration status
  lcd.setCursor(0, 1);
  lcd.print("Cal:");
  lcd.print(emptyTankMM);
  lcd.print("-");
  lcd.print(fullTankMM);
  lcd.print("mm");
}

void handleSerialCommands() {
  if (Serial.available()) {
    char command = Serial.read();
    
    switch (command) {
      case 'L':
      case 'l':
        setLowLevel();
        break;
        
      case 'H':
      case 'h':
        setHighLevel();
        break;
        
      case 'R':
      case 'r':
        resetCalibration();
        break;
        
      case 'P':
      case 'p':
        printCalibration();
        break;
        
      case '?':
        printHelp();
        break;
    }
  }
}

void setLowLevel() {
  if (currentLevelMM >= 0) {
    emptyTankMM = currentLevelMM;
    Serial.print("✓ Empty level set to: ");
    Serial.print(emptyTankMM);
    Serial.println(" mm");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Empty Set:");
    lcd.print(emptyTankMM);
    lcd.print("mm");
    delay(2000);
  } else {
    Serial.println("✗ Error: Cannot read sensor");
  }
}

void setHighLevel() {
  if (currentLevelMM >= 0) {
    fullTankMM = currentLevelMM;
    Serial.print("✓ Full level set to: ");
    Serial.print(fullTankMM);
    Serial.println(" mm");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Full Set:");
    lcd.print(fullTankMM);
    lcd.print("mm");
    delay(2000);
  } else {
    Serial.println("✗ Error: Cannot read sensor");
  }
}

void resetCalibration() {
  emptyTankMM = 0;
  fullTankMM = 100;
  Serial.println("✓ Calibration reset to defaults");
}

void printCalibration() {
  Serial.println("\n=== CURRENT CALIBRATION ===");
  Serial.print("Empty Tank: "); Serial.print(emptyTankMM); Serial.println(" mm");
  Serial.print("Full Tank: "); Serial.print(fullTankMM); Serial.println(" mm");
  Serial.print("Range: "); Serial.print(fullTankMM - emptyTankMM); Serial.println(" mm");
  Serial.println("===========================");
}

void printHelp() {
  Serial.println("\n=== CALIBRATION COMMANDS ===");
  Serial.println("L - Set current reading as EMPTY tank level");
  Serial.println("H - Set current reading as FULL tank level");
  Serial.println("R - Reset calibration to defaults");
  Serial.println("P - Print current calibration values");
  Serial.println("? - Show this help menu");
  Serial.println("============================");
}

void sendDataToBlynk() {
  if (currentLevelMM >= 0 && currentPercentage >= 0) {
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_LEVEL, currentLevelMM);
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_PERCENTAGE, currentPercentage);
    
    String status = "Cal: " + String(emptyTankMM) + "-" + String(fullTankMM) + "mm";
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_CAL_STATUS, status);
  }
}

String getUnitSymbol(int unit) {
  switch(unit) {
    case 0: return "Mpa";
    case 1: return "Kpa";
    case 2: return "Day";
    case 3: return "Bar";
    case 4: return "mb";
    case 5: return "kg/cm";
    case 6: return "PSI";
    case 7: return "m";
    case 8: return "mm";
    default: return "??";
  }
}

void preTransmission() {
  digitalWrite(MAX485_DE, HIGH);
  digitalWrite(MAX485_RE, HIGH);
}

void postTransmission() {
  digitalWrite(MAX485_DE, LOW);
  digitalWrite(MAX485_RE, LOW);
}
