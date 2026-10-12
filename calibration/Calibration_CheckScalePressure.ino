// Blynk 2.0 authentication
#define BLYNK_TEMPLATE_ID "xxxxxxxxxxx"
#define BLYNK_TEMPLATE_NAME "TOKOASIA DEMO"
#define BLYNK_AUTH_TOKEN "xxxxxxxxxxxxxxxxxxxxxxxxxxx33Z"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include <ModbusMaster.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Your WiFi credentials
char ssid[] = "xxxxxxxxx2.4GHz";
char pass[] = "xxxxxxxxx";

// Define the MAX485 control pins
#define MAX485_DE 5
#define MAX485_RE 4

// Initialize I2C LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Instantiate ModbusMaster object
ModbusMaster node;

// Calibration variables
int decimalPlaces = 0;
int measurementUnit = 0;
float currentPressure = 0;
float lowLevelPressure = 0.0;
float highLevelPressure = 0.0;
String tankName = "Tank1";

// Calibration states
enum CalibrationState {
  IDLE,
  MEASURE_LOW,
  MEASURE_HIGH,
  CALIBRATION_COMPLETE
};
CalibrationState calState = IDLE;

// Blynk virtual pins for calibration
#define BLYNK_VIRTUAL_PIN_PRESSURE V0
#define BLYNK_VIRTUAL_PIN_CAL_STATUS V1
#define BLYNK_VIRTUAL_PIN_CAL_CONTROL V2

BlynkTimer timer;

// EEPROM-like storage for calibration data
#include <Preferences.h>
Preferences preferences;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C LCD
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  
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

  // Initialize preferences
  preferences.begin("pressure-cal", false);

  // Connect to WiFi and Blynk
  connectToNetworks();
  
  // Load previous calibration if exists
  loadCalibrationData();
  
  // Setup timer
  timer.setInterval(1000L, updateReadings);
  
  Serial.println("Pressure Sensor Calibration Ready!");
  Serial.println("Commands: 'L'=Set Low, 'H'=Set High, 'R'=Reset, 'S'=Save");
}

void loop() {
  Blynk.run();
  timer.run();
  handleSerialCommands();
  updateLCD();
}

void connectToNetworks() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi:Connecting");
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi:Connected!");
    Serial.println("WiFi connected!");
  }
}

void loadCalibrationData() {
  lowLevelPressure = preferences.getFloat("lowPress", 0.0);
  highLevelPressure = preferences.getFloat("highPress", 0.0);
  tankName = preferences.getString("tankName", "Tank1");
  
  Serial.println("Loaded calibration data:");
  Serial.println("Tank: " + tankName);
  Serial.print("Low Pressure: "); Serial.println(lowLevelPressure);
  Serial.print("High Pressure: "); Serial.println(highLevelPressure);
  
  if (lowLevelPressure > 0 && highLevelPressure > 0) {
    calState = CALIBRATION_COMPLETE;
  }
}

void saveCalibrationData() {
  preferences.putFloat("lowPress", lowLevelPressure);
  preferences.putFloat("highPress", highLevelPressure);
  preferences.putString("tankName", tankName);
  Serial.println("Calibration data saved!");
}

void readSensorConfiguration() {
  // Read decimal places (Register 3)
  if (node.readHoldingRegisters(3, 1) == node.ku8MBSuccess) {
    decimalPlaces = node.getResponseBuffer(0);
    Serial.print("Decimal Places: ");
    Serial.println(decimalPlaces);
  }
  
  // Read unit (Register 2)
  if (node.readHoldingRegisters(2, 1) == node.ku8MBSuccess) {
    measurementUnit = node.getResponseBuffer(0);
    Serial.print("Measurement Unit: ");
    Serial.println(getUnitSymbol(measurementUnit));
  }
}

void readCurrentPressure() {
  if (node.readHoldingRegisters(4, 1) == node.ku8MBSuccess) {
    int16_t rawValue = (int16_t)node.getResponseBuffer(0);
    currentPressure = rawValue / pow(10, decimalPlaces);
    
    Serial.print("Current Pressure: ");
    Serial.print(currentPressure, decimalPlaces);
    Serial.print(" ");
    Serial.println(getUnitSymbol(measurementUnit));
  } else {
    Serial.println("Error reading pressure sensor");
    currentPressure = -1;
  }
}

void updateReadings() {
  readSensorConfiguration();
  readCurrentPressure();
  sendDataToBlynk();
}

void updateLCD() {
  lcd.clear();
  
  // Line 1: Current pressure and calibration status
  lcd.setCursor(0, 0);
  lcd.print("P:");
  if (currentPressure >= 0) {
    lcd.print(currentPressure, decimalPlaces);
    lcd.print(getUnitSymbol(measurementUnit).substring(0, 3));
  } else {
    lcd.print("Error");
  }
  
  // Line 2: Calibration status and values
  lcd.setCursor(0, 1);
  switch (calState) {
    case IDLE:
      lcd.print("Cal:Ready");
      break;
    case MEASURE_LOW:
      lcd.print("Set LOW pressure");
      break;
    case MEASURE_HIGH:
      lcd.print("Set HIGH pressure");
      break;
    case CALIBRATION_COMPLETE:
      lcd.print("L:");
      lcd.print(lowLevelPressure, decimalPlaces);
      lcd.print(" H:");
      lcd.print(highLevelPressure, decimalPlaces);
      break;
  }
}

void sendDataToBlynk() {
  if (currentPressure >= 0) {
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_PRESSURE, currentPressure);
    
    // Send calibration status
    String status = getCalibrationStatus();
    Blynk.virtualWrite(BLYNK_VIRTUAL_PIN_CAL_STATUS, status);
  }
}

String getCalibrationStatus() {
  switch (calState) {
    case IDLE: return "Ready for calibration";
    case MEASURE_LOW: return "Setting Low Pressure";
    case MEASURE_HIGH: return "Setting High Pressure";
    case CALIBRATION_COMPLETE: 
      return "Calibrated: L=" + String(lowLevelPressure, decimalPlaces) + 
             " H=" + String(highLevelPressure, decimalPlaces);
    default: return "Unknown";
  }
}

void handleSerialCommands() {
  if (Serial.available()) {
    char command = Serial.read();
    
    switch (command) {
      case 'L':
      case 'l':
        setLowPressure();
        break;
        
      case 'H':
      case 'h':
        setHighPressure();
        break;
        
      case 'R':
      case 'r':
        resetCalibration();
        break;
        
      case 'S':
      case 's':
        saveCalibrationData();
        break;
        
      case 'P':
      case 'p':
        printCalibrationData();
        break;
        
      case '?':
        printHelp();
        break;
    }
  }
}

void setLowPressure() {
  if (currentPressure >= 0) {
    lowLevelPressure = currentPressure;
    calState = MEASURE_LOW;
    Serial.print("Low pressure set to: ");
    Serial.println(lowLevelPressure, decimalPlaces);
    
    // Ask for high pressure next
    Serial.println("Now set high pressure (command 'H')");
  } else {
    Serial.println("Error: Cannot read current pressure");
  }
}

void setHighPressure() {
  if (currentPressure >= 0) {
    highLevelPressure = currentPressure;
    calState = MEASURE_HIGH;
    Serial.print("High pressure set to: ");
    Serial.println(highLevelPressure, decimalPlaces);
    
    // Check if calibration is complete
    if (lowLevelPressure > 0 && highLevelPressure > 0) {
      calState = CALIBRATION_COMPLETE;
      Serial.println("Calibration complete! Use 'S' to save.");
    }
  } else {
    Serial.println("Error: Cannot read current pressure");
  }
}

void resetCalibration() {
  lowLevelPressure = 0.0;
  highLevelPressure = 0.0;
  calState = IDLE;
  Serial.println("Calibration reset");
}

void printCalibrationData() {
  Serial.println("\n=== CALIBRATION DATA ===");
  Serial.println("Tank: " + tankName);
  Serial.print("Low Pressure: "); 
  Serial.print(lowLevelPressure, decimalPlaces);
  Serial.println(" " + getUnitSymbol(measurementUnit));
  
  Serial.print("High Pressure: "); 
  Serial.print(highLevelPressure, decimalPlaces);
  Serial.println(" " + getUnitSymbol(measurementUnit));
  
  if (lowLevelPressure > 0 && highLevelPressure > 0) {
    float range = highLevelPressure - lowLevelPressure;
    Serial.print("Pressure Range: "); 
    Serial.print(range, decimalPlaces);
    Serial.println(" " + getUnitSymbol(measurementUnit));
  }
  Serial.println("======================");
}

void printHelp() {
  Serial.println("\n=== CALIBRATION COMMANDS ===");
  Serial.println("L - Set current pressure as LOW level");
  Serial.println("H - Set current pressure as HIGH level");
  Serial.println("R - Reset calibration data");
  Serial.println("S - Save calibration to memory");
  Serial.println("P - Print current calibration data");
  Serial.println("? - Show this help menu");
  Serial.println("============================");
}

// Blynk write handler for calibration control
BLYNK_WRITE(BLYNK_VIRTUAL_PIN_CAL_CONTROL) {
  int command = param.asInt();
  
  switch (command) {
    case 1: // Set Low
      setLowPressure();
      break;
    case 2: // Set High
      setHighPressure();
      break;
    case 3: // Reset
      resetCalibration();
      break;
    case 4: // Save
      saveCalibrationData();
      break;
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
