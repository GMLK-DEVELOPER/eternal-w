#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <FastLED.h>

// Прототипы функций для RGB ленты
void setRGBColor(CRGB color);
void setRGBColor(int r, int g, int b);
void enableRGB(bool enable);
void setRGBBrightness(int brightness);
void updateRainbowEffect();

// WiFi settings
const char* ssid = "DESKTOP";
const char* password = "8q93D1-8";

// Telegram Bot Token
#define BOT_TOKEN "7176506200:AAEwqZXr8QnvElwO-9JjzeiU9iE1OTf9j4I"

// Chat ID (leave empty to respond to all)
#define CHAT_ID ""

// Servo pin on ESP32
#define SERVO_PIN 23

// I2C pins (standard for ESP32)
#define SDA_PIN 21
#define SCL_PIN 22

// LCD display settings
// 0x27 is standard address for most I2C LCD modules, but might be 0x3F or different
// If not working with 0x27, try 0x3F
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

// Conversion: 1 command unit = 180 physical degrees
#define DEGREES_MULTIPLIER 180

// RGB LED Strip settings
#define LED_DATA_PIN 5   // DO - Data pin
#define LED_CLOCK_PIN 4  // BO - Clock pin (если это часы)
#define NUM_LEDS 30      // Количество светодиодов в ленте
#define LED_TYPE WS2812B // Тип ленты (может быть другим, например WS2811, APA102)
#define COLOR_ORDER GRB  // Порядок цветов

// Check messages interval (1 second)
const unsigned long BOT_MTBS = 1000;
unsigned long bot_lasttime;

// Variables for servo control
int servoStopValue = 90;      // Value to stop servo
int cwSpeed = 60;             // Speed for clockwise rotation (adjust as needed)
int ccwSpeed = 120;           // Speed for counter-clockwise rotation (adjust as needed)

// Variables for movement control
int commandUnits = 0;         // Current position in command units (1 unit = 180 degrees)
int targetUnits = 0;          // Target position in command units
bool isMoving = false;        // Is servo currently moving
unsigned long moveStartTime = 0;    // When movement started
unsigned long lastUnitChange = 0;   // Last time unit was changed
int unitsToMove = 0;          // Total units to move
int unitsMoved = 0;           // Units moved so far
int secondsPerUnit = 1;       // Seconds per unit (default 1 unit per second)

// Variables for RGB LED control
int ledBrightness = 50;        // Brightness (0-255)
CRGB currentColor = CRGB::Blue; // Current color
bool rgbEnabled = false;       // RGB strip state
bool rainbowMode = false;      // Rainbow animation mode
unsigned long lastLedUpdate = 0; // Last time LEDs were updated
const int ledUpdateInterval = 50; // LED update interval (ms)

// Variables for LCD update
unsigned long lastLcdUpdate = 0;
const int lcdUpdateInterval = 500; // LCD screen update interval (ms)

// User input state management
bool waitingForCustomPosition = false;
bool waitingForCustomRotation = false;
bool waitingForRGBColor = false;
String lastChatId = "";

// Telegram bot objects
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Servo object
Servo myServo;

// LCD display object
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// Массив светодиодов
CRGB leds[NUM_LEDS];

// Объявление прототипов функций
void initLCD();
void updateLCDMovement();
void updateLCD();
void startMovement(int targetPosition, int speedSec = 1);
void stopMovement();
void updateMovement();
void setRGBColor(CRGB color);
void setRGBColor(int r, int g, int b);
void enableRGB(bool enable);
void setRGBBrightness(int brightness);
void updateRainbowEffect();
void sendMainKeyboard(String chat_id);
void sendCustomPositionKeyboard(String chat_id);
void sendCustomRotationKeyboard(String chat_id);
void sendRGBColorKeyboard(String chat_id);
void sendRGBBrightnessKeyboard(String chat_id);
void handleNewMessages(int numNewMessages);

// RGB LED functions
void setRGBColor(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  currentColor = color;
  rainbowMode = false;
}

void setRGBColor(int r, int g, int b) {
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  FastLED.show();
  currentColor = CRGB(r, g, b);
  rainbowMode = false;
}

void enableRGB(bool enable) {
  rgbEnabled = enable;
  if (enable) {
    setRGBColor(currentColor);
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  }
}

void setRGBBrightness(int brightness) {
  ledBrightness = constrain(brightness, 0, 255);
  FastLED.setBrightness(ledBrightness);
  FastLED.show();
}

void updateRainbowEffect() {
  if (rainbowMode && rgbEnabled) {
    // Используем встроенный эффект радуги из FastLED
    fill_rainbow(leds, NUM_LEDS, millis() / 50, 7);
    FastLED.show();
  }
}

// Initialize LCD display
void initLCD() {
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin(LCD_COLUMNS, LCD_ROWS);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
}

// Forward declarations of functions
void setRGBColor(CRGB color);
void setRGBColor(int r, int g, int b);
void enableRGB(bool enable);
void setRGBBrightness(int brightness);
void updateRainbowEffect();
void updateRainbowEffect();
void updateLCDMovement();
void updateLCD();
void startMovement(int targetPosition, int speedSec = 1);
void stopMovement();
void updateMovement();
void sendMainKeyboard(String chat_id);
void sendCustomPositionKeyboard(String chat_id);
void sendCustomRotationKeyboard(String chat_id);
void sendRGBColorKeyboard(String chat_id);
void sendRGBBrightnessKeyboard(String chat_id);
void handleNewMessages(int numNewMessages);

// Variables for servo control
int servoStopValue = 90;      // Value to stop servo
int cwSpeed = 60;             // Speed for clockwise rotation (adjust as needed)
int ccwSpeed = 120;           // Speed for counter-clockwise rotation (adjust as needed)

// Variables for movement control
int commandUnits = 0;         // Current position in command units (1 unit = 180 degrees)
int targetUnits = 0;          // Target position in command units
bool isMoving = false;        // Is servo currently moving
unsigned long moveStartTime = 0;    // When movement started
unsigned long lastUnitChange = 0;   // Last time unit was changed
int unitsToMove = 0;          // Total units to move
int unitsMoved = 0;           // Units moved so far
int secondsPerUnit = 1;       // Seconds per unit (default 1 unit per second)

// Variables for RGB LED control
int ledBrightness = 50;        // Brightness (0-255)
CRGB currentColor = CRGB::Blue; // Current color
bool rgbEnabled = false;       // RGB strip state
bool rainbowMode = false;      // Rainbow animation mode
unsigned long lastLedUpdate = 0; // Last time LEDs were updated
const int ledUpdateInterval = 50; // LED update interval (ms)

// Variables for LCD update
unsigned long lastLcdUpdate = 0;
const int lcdUpdateInterval = 500; // LCD screen update interval (ms)

// User input state management
bool waitingForCustomPosition = false;
bool waitingForCustomRotation = false;
bool waitingForRGBColor = false;
String lastChatId = "";

// Telegram bot objects
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Servo object
Servo myServo;

// LCD display object
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// RGB LED functions
void setRGBColor(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  currentColor = color;
  rainbowMode = false;
}

void setRGBColor(int r, int g, int b) {
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  FastLED.show();
  currentColor = CRGB(r, g, b);
  rainbowMode = false;
}

void enableRGB(bool enable) {
  rgbEnabled = enable;
  if (enable) {
    setRGBColor(currentColor);
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  }
}

void setRGBBrightness(int brightness) {
  ledBrightness = constrain(brightness, 0, 255);
  FastLED.setBrightness(ledBrightness);
  FastLED.show();
}

void updateRainbowEffect() {
  if (rainbowMode && rgbEnabled) {
    // Используем встроенный эффект радуги из FastLED
    fill_rainbow(leds, NUM_LEDS, millis() / 50, 7);
    FastLED.show();
  }
}

// Update LCD with movement information
void updateLCDMovement() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Unit: ");
  lcd.print(commandUnits);
  lcd.print("/");
  lcd.print(targetUnits);
  
  lcd.setCursor(0, 1);
  lcd.print("Moved: ");
  lcd.print(unitsMoved);
  lcd.print("/");
  lcd.print(unitsToMove);
}

// Update LCD with standard information
void updateLCD() {
  unsigned long currentMillis = millis();
  
  if (!isMoving && currentMillis - lastLcdUpdate >= lcdUpdateInterval) {
    lastLcdUpdate = currentMillis;
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Current: ");
    lcd.print(commandUnits);
    lcd.print(" unit");
    
    lcd.setCursor(0, 1);
    if (WiFi.status() == WL_CONNECTED) {
      lcd.print("IP: ");
      lcd.print(WiFi.localIP().toString());
    } else {
      lcd.print("WiFi disconnected");
    }
  }
}

// Start movement from current unit to target unit (1 unit per second)
// 1 unit = 180 physical degrees
void startMovement(int targetPosition, int speedSec = 1) {
  // Set target position in command units
  targetUnits = targetPosition;
  secondsPerUnit = speedSec;
  
  // Calculate direction and distance
  int clockwiseDist, counterClockwiseDist;
  
  // In a continuous system, we just calculate direct distance
  clockwiseDist = targetUnits - commandUnits;
  counterClockwiseDist = -clockwiseDist; // Opposite direction
  
  // Choose shorter direction
  if (abs(clockwiseDist) <= abs(counterClockwiseDist)) {
    // Move clockwise
    unitsToMove = abs(clockwiseDist);
    if (clockwiseDist > 0) {
      myServo.write(cwSpeed); // Clockwise rotation
    } else if (clockwiseDist < 0) {
      myServo.write(ccwSpeed); // Counter-clockwise rotation
    } else {
      myServo.write(servoStopValue); // Already at target
    }
  } else {
    // Move counter-clockwise
    unitsToMove = abs(counterClockwiseDist);
    if (counterClockwiseDist > 0) {
      myServo.write(cwSpeed); // Clockwise rotation
    } else if (counterClockwiseDist < 0) {
      myServo.write(ccwSpeed); // Counter-clockwise rotation
    } else {
      myServo.write(servoStopValue); // Already at target
    }
  }
  
  // If already at target position
  if (unitsToMove == 0) {
    myServo.write(servoStopValue); // Stop servo
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Already at");
    lcd.setCursor(0, 1);
    lcd.print("Target position");
    return;
  }
  
  isMoving = true;
  moveStartTime = millis();
  lastUnitChange = moveStartTime;
  unitsMoved = 0;
  
  // Update LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moving to: ");
  lcd.print(targetUnits);
  lcd.setCursor(0, 1);
  lcd.print("From: ");
  lcd.print(commandUnits);
  
  Serial.println("Moving from " + String(commandUnits) + " to " + String(targetUnits) + 
                 " (" + String(unitsToMove) + " units)");
}

// Stop movement
void stopMovement() {
  myServo.write(servoStopValue); // Stop servo
  isMoving = false;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Movement stopped");
  lcd.setCursor(0, 1);
  lcd.print("Unit: ");
  lcd.print(commandUnits);
  
  Serial.println("Movement stopped at unit " + String(commandUnits));
}

// Update movement (advancing 1 unit per second)
void updateMovement() {
  if (isMoving) {
    unsigned long currentMillis = millis();
    unsigned long elapsedTime = (currentMillis - lastUnitChange);
    
    // Check if we've moved enough (1 unit per second or as configured)
    if (elapsedTime >= (secondsPerUnit * 1000)) {
      lastUnitChange = currentMillis;
      
      // Update current position
      if (myServo.read() < servoStopValue) {
        // Moving clockwise
        commandUnits++;
      } else if (myServo.read() > servoStopValue) {
        // Moving counter-clockwise
        commandUnits--;
      }
      
      unitsMoved++;
      
      // Update LCD every second
      updateLCDMovement();
      
      // Check if we've reached target
      if (unitsMoved >= unitsToMove) {
        myServo.write(servoStopValue); // Stop servo
        isMoving = false;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Target reached!");
        lcd.setCursor(0, 1);
        lcd.print("Unit: ");
        lcd.print(commandUnits);
        
        Serial.println("Target position reached: " + String(commandUnits));
      }
    }
  }
}

// Send custom keyboard markup with main controls
void sendMainKeyboard(String chat_id) {
  String message = "Servo Control System\n";
  message += "Current position: " + String(commandUnits) + " units\n";
  message += "Speed: " + String(secondsPerUnit) + " sec/unit\n";
  message += "Status: " + String(isMoving ? "Moving" : "Stopped") + "\n";
  message += "RGB: " + String(rgbEnabled ? "ON" : "OFF");
  
  // Define the keyboard layout for main controls
  String keyboard = "[[\"⬅️ CCW 1 unit\", \"⏹️ STOP\", \"CW 1 unit ➡️\"], ";
  keyboard += "[\"⬅️ CCW 5 units\", \"Custom Move\", \"CW 5 units ➡️\"], ";
  keyboard += "[\"Position 0\", \"Custom Rotate\", \"Position 10\"], ";
  keyboard += "[\"Speed +\", \"Status\", \"Speed -\"], ";
  keyboard += "[\"RGB ON\", \"RGB OFF\", \"RGB Color\"], ";
  keyboard += "[\"RGB Rainbow\", \"RGB Brightness\", \"System Info\"]]";
  
  bot.sendMessageWithReplyKeyboard(chat_id, message, "", keyboard, true);
}

// Send keyboard for awaiting custom position input
void sendCustomPositionKeyboard(String chat_id) {
  String message = "Please enter the position value (in units) you want to move to.\n";
  message += "Current position: " + String(commandUnits) + " units\n";
  message += "Type a number and send it.";
  
  // Define the keyboard layout for cancellation
  String keyboard = "[[\"Cancel Custom Move\"]]";
  
  bot.sendMessageWithReplyKeyboard(chat_id, message, "", keyboard, true);
}

// Send keyboard for awaiting custom rotation input
void sendCustomRotationKeyboard(String chat_id) {
  String message = "Please enter how many units to rotate.\n";
  message += "Current position: " + String(commandUnits) + " units\n";
  message += "Positive numbers: clockwise\n";
  message += "Negative numbers: counter-clockwise\n";
  message += "Type a number and send it.";
  
  // Define the keyboard layout for cancellation
  String keyboard = "[[\"Cancel Custom Rotate\"]]";
  
  bot.sendMessageWithReplyKeyboard(chat_id, message, "", keyboard, true);
}

// Send keyboard for RGB color selection
void sendRGBColorKeyboard(String chat_id) {
  String message = "Select a color for the RGB LED strip:";
  
  // Define the keyboard layout for color selection
  String keyboard = "[[\"🔴 Red\", \"🟢 Green\", \"🔵 Blue\"], ";
  keyboard += "[\"⚪ White\", \"🟣 Purple\", \"🟡 Yellow\"], ";
  keyboard += "[\"🟠 Orange\", \"🌈 Rainbow\", \"⬇️ Brightness\"], ";
  keyboard += "[\"🔙 Back to Main Menu\"]]";
  
  bot.sendMessageWithReplyKeyboard(chat_id, message, "", keyboard, true);
}

// Send keyboard for RGB brightness control
void sendRGBBrightnessKeyboard(String chat_id) {
  String message = "Select RGB LED brightness level:\n";
  message += "Current brightness: " + String(ledBrightness) + " (0-255)";
  
  // Define the keyboard layout for brightness control
  String keyboard = "[[\"🔆 100%\", \"🔆 75%\", \"🔆 50%\"], ";
  keyboard += "[\"🔆 25%\", \"🔆 10%\", \"🔆 5%\"], ";
  keyboard += "[\"🔙 Back to RGB Menu\"]]";
  
  bot.sendMessageWithReplyKeyboard(chat_id, message, "", keyboard, true);
}

// Handle incoming messages
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    
    lastChatId = chat_id; // Save last chat ID for future reference
    
    if (from_name == "") from_name = "Guest";
    
    // Debug chat ID
    Serial.println("Chat ID: " + chat_id);
    Serial.println("Message: " + text);
    
    // Update LCD: new message received
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("New command:");
    lcd.setCursor(0, 1);
    // Limit message length to screen size
    if (text.length() > LCD_COLUMNS) {
      lcd.print(text.substring(0, LCD_COLUMNS));
    } else {
      lcd.print(text);
    }
    delay(1000); // Show message for 1 second
    
    // Check if we're waiting for custom position input
    if (waitingForCustomPosition) {
      // Cancel command
      if (text == "Cancel Custom Move") {
        waitingForCustomPosition = false;
        bot.sendMessage(chat_id, "Custom move canceled", "");
        sendMainKeyboard(chat_id);
        continue;
      }
      
      // Try to parse the position
      int position = text.toInt();
      if (position != 0 || text == "0") {
        // Valid number input
        waitingForCustomPosition = false;
        startMovement(position);
        bot.sendMessage(chat_id, "Moving to position " + String(position) + " units at " + 
                       String(secondsPerUnit) + " seconds per unit", "");
        sendMainKeyboard(chat_id);
      } else {
        // Invalid input
        bot.sendMessage(chat_id, "Please enter a valid number.", "");
        // Remain in custom position mode
      }
      continue;
    }
    
    // Check if we're waiting for custom rotation input
    if (waitingForCustomRotation) {
      // Cancel command
      if (text == "Cancel Custom Rotate") {
        waitingForCustomRotation = false;
        bot.sendMessage(chat_id, "Custom rotation canceled", "");
        sendMainKeyboard(chat_id);
        continue;
      }
      
      // Try to parse the rotation amount
      int rotation = text.toInt();
      if (rotation != 0 || text == "0") {
        // Valid number input
        waitingForCustomRotation = false;
        
        // Calculate target position
        int target = commandUnits + rotation;
        startMovement(target);
        
        if (rotation > 0) {
          bot.sendMessage(chat_id, "Rotating " + String(rotation) + " units clockwise", "");
        } else {
          bot.sendMessage(chat_id, "Rotating " + String(-rotation) + " units counter-clockwise", "");
        }
        sendMainKeyboard(chat_id);
      } else {
        // Invalid input
        bot.sendMessage(chat_id, "Please enter a valid number.", "");
        // Remain in custom rotation mode
      }
      continue;
    }
    
    // Handle standard button commands
    if (text == "CW 1 unit ➡️") {
      // Rotate clockwise by 1 unit
      int target = commandUnits + 1;
      startMovement(target);
      bot.sendMessage(chat_id, "Rotating 1 unit clockwise", "");
    }
    else if (text == "⬅️ CCW 1 unit") {
      // Rotate counter-clockwise by 1 unit
      int target = commandUnits - 1;
      startMovement(target);
      bot.sendMessage(chat_id, "Rotating 1 unit counter-clockwise", "");
    }
    else if (text == "CW 5 units ➡️") {
      // Rotate clockwise by 5 units
      int target = commandUnits + 5;
      startMovement(target);
      bot.sendMessage(chat_id, "Rotating 5 units clockwise", "");
    }
    else if (text == "⬅️ CCW 5 units") {
      // Rotate counter-clockwise by 5 units
      int target = commandUnits - 5;
      startMovement(target);
      bot.sendMessage(chat_id, "Rotating 5 units counter-clockwise", "");
    }
    else if (text == "⏹️ STOP") {
      // Stop movement
      if (isMoving) {
        stopMovement();
        bot.sendMessage(chat_id, "Movement stopped at position " + String(commandUnits) + " units", "");
      } else {
        bot.sendMessage(chat_id, "Servo is already stopped at position " + String(commandUnits) + " units", "");
      }
    }
    else if (text == "Position 0") {
      // Move to position 0
      startMovement(0);
      bot.sendMessage(chat_id, "Moving to position 0", "");
    }
    else if (text == "Position 10") {
      // Move to position 10
      startMovement(10);
      bot.sendMessage(chat_id, "Moving to position 10", "");
    }
    else if (text == "Custom Move") {
      // Enter custom position mode
      waitingForCustomPosition = true;
      waitingForCustomRotation = false;
      sendCustomPositionKeyboard(chat_id);
      continue; // Skip sending main keyboard
    }
    else if (text == "Custom Rotate") {
      // Enter custom rotation mode
      waitingForCustomRotation = true;
      waitingForCustomPosition = false;
      sendCustomRotationKeyboard(chat_id);
      continue; // Skip sending main keyboard
    }
    else if (text == "Speed +") {
      // Decrease seconds per unit (speed up)
      secondsPerUnit = max(1, secondsPerUnit - 1); // Minimum 1 second per unit
      bot.sendMessage(chat_id, "Speed set to " + String(secondsPerUnit) + " seconds per unit", "");
    }
    else if (text == "Speed -") {
      // Increase seconds per unit (slow down)
      secondsPerUnit = min(10, secondsPerUnit + 1); // Maximum 10 seconds per unit
      bot.sendMessage(chat_id, "Speed set to " + String(secondsPerUnit) + " seconds per unit", "");
    }
    else if (text == "Status") {
      // Show status
      String status = "System status:\n";
      status += "IP address: " + WiFi.localIP().toString() + "\n";
      status += "WiFi signal: " + String(WiFi.RSSI()) + " dBm\n";
      status += "Current position: " + String(commandUnits) + " units\n";
      status += "Speed: " + String(secondsPerUnit) + " seconds per unit\n";
      status += "1 unit = 180 physical degrees\n";
      
      if (isMoving) {
        status += "Moving to: " + String(targetUnits) + " units\n";
        status += "Progress: " + String(unitsMoved) + "/" + String(unitsToMove) + " units\n";
      } else {
        status += "Status: Stopped\n";
      }
      
      status += "RGB LED status: " + String(rgbEnabled ? "ON" : "OFF") + "\n";
      status += "RGB brightness: " + String(ledBrightness) + "\n";
      status += "Rainbow mode: " + String(rainbowMode ? "ON" : "OFF");
      
      bot.sendMessage(chat_id, status, "");
    }
    // RGB controls
    else if (text == "RGB ON") {
      enableRGB(true);
      bot.sendMessage(chat_id, "RGB LED strip turned ON", "");
    }
    else if (text == "RGB OFF") {
      enableRGB(false);
      bot.sendMessage(chat_id, "RGB LED strip turned OFF", "");
    }
    else if (text == "RGB Color") {
      sendRGBColorKeyboard(chat_id);
      continue; // Skip sending main keyboard
    }
    else if (text == "RGB Rainbow") {
      rainbowMode = !rainbowMode;
      rgbEnabled = true;
      bot.sendMessage(chat_id, "Rainbow mode: " + String(rainbowMode ? "ON" : "OFF"), "");
    }
    else if (text == "RGB Brightness") {
      sendRGBBrightnessKeyboard(chat_id);
      continue; // Skip sending main keyboard
    }
    else if (text == "System Info") {
      String info = "System information:\n";
      info += "ESP32 free heap: " + String(ESP.getFreeHeap()) + " bytes\n";
      info += "ESP32 SDK version: " + String(ESP.getSdkVersion()) + "\n";
      info += "WiFi SSID: " + String(ssid) + "\n";
      info += "ESP32 chip revision: " + String(ESP.getChipRevision()) + "\n";
      info += "Flash chip size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB\n";
      info += "CPU frequency: " + String(ESP.getCpuFreqMHz()) + " MHz";
      
      bot.sendMessage(chat_id, info, "");
    }
    // RGB color options
    else if (text == "🔴 Red") {
      setRGBColor(CRGB::Red);
      rgbEnabled = true;
      bot.sendMessage(chat_id, "RGB color set to Red", "");
      sendMainKeyboard(chat_id);
      continue;
    }
    else if (text == "🟢 Green") {
      setRGBColor(CRGB::Green);
      rgbEnabled = true;
      bot.sendMessage(chat_id, "RGB color set to Green", "");
      sendMainKeyboard(chat_id);
      continue;
    }
    else if (text == "🔵 Blue") {
      setRGBColor(CRGB::Blue);
      rgbEnabled = true;
      bot.sendMessage(chat_id, "RGB color set to Blue", "");
      sendMainKeyboard(chat_id);
      continue;
    }
    else if (text == "⚪ White") {
      setRGBColor(CRGB::White);
      rgbEnabled = true;
      bot.sendMessage(chat_id, "RGB color set to White", "");
      sendMainKeyboard(chat_id);
      continue;
    }
    else if (text == "🟣 Purple") {
      setRGBColor(CRGB::Purple);
      rgbEnabled = true;
      bot.sendMessage(chat_id, "RGB color set to Purple", "");
      sendMainKeyboard(chat_id);
      continue;
    }
    else if (text == "🟡 Yellow") {
      setRGBColor(CRGB::Yellow);
      rgbEnabled = true;
      bot.sendMessage(chat_id, "RGB color set to Yellow", "");
      sendMainKeyboard(chat_id);
      continue;
    }
    else if (text == "🟠 Orange") {
      setRGBColor(CRGB::Orange);
      rgbEnabled = true;
      bot.sendMessage(chat_id, "RGB color set to Orange", "");
      sendMainKeyboard(chat_id);
      continue;
    }
    else if (text == "🌈 Rainbow") {
      rainbowMode = true;
      rgbEnabled = true;
      bot.sendMessage(chat_id, "Rainbow mode enabled", "");
      sendMainKeyboard(chat_id);
      continue;
    }
    else if (text == "⬇️ Brightness") {
      sendRGBBrightnessKeyboard(chat_id);
      continue;
    }
    else if (text == "🔙 Back to Main Menu") {
      sendMainKeyboard(chat_id);
      continue;
    }
    else if (text == "🔙 Back to RGB Menu") {
      sendRGBColorKeyboard(chat_id);
      continue;
    }
    // RGB brightness levels
    else if (text == "🔆 100%") {
      setRGBBrightness(255);
      bot.sendMessage(chat_id, "RGB brightness set to 100%", "");
      sendRGBBrightnessKeyboard(chat_id);
      continue;
    }
    else if (text == "🔆 75%") {
      setRGBBrightness(191);
      bot.sendMessage(chat_id, "RGB brightness set to 75%", "");
      sendRGBBrightnessKeyboard(chat_id);
      continue;
    }
    else if (text == "🔆 50%") {
      setRGBBrightness(127);
      bot.sendMessage(chat_id, "RGB brightness set to 50%", "");
      sendRGBBrightnessKeyboard(chat_id);
      continue;
    }
    else if (text == "🔆 25%") {
      setRGBBrightness(64);
      bot.sendMessage(chat_id, "RGB brightness set to 25%", "");
      sendRGBBrightnessKeyboard(chat_id);
      continue;
    }
    else if (text == "🔆 10%") {
      setRGBBrightness(25);
      bot.sendMessage(chat_id, "RGB brightness set to 10%", "");
      sendRGBBrightnessKeyboard(chat_id);
      continue;
    }
    else if (text == "🔆 5%") {
      setRGBBrightness(13);
      bot.sendMessage(chat_id, "RGB brightness set to 5%", "");
      sendRGBBrightnessKeyboard(chat_id);
      continue;
    }
    // Handle commands too
    else if (text == "/start" || text == "/menu") {
      sendMainKeyboard(chat_id);
      continue; // Skip sending main keyboard again
    }
    else if (text.startsWith("/move")) {
      // Move to position command: /move X
      String posStr = text.substring(6);
      int position = posStr.toInt();
      
      startMovement(position);
      bot.sendMessage(chat_id, "Moving to position " + String(position) + " units at " + 
                     String(secondsPerUnit) + " seconds per unit", "");
    }
    else if (text.startsWith("/speed")) {
      // Set speed command: /speed X
      String speedStr = text.substring(7);
      int speed = speedStr.toInt();
      
      if (speed > 0) {
        secondsPerUnit = speed;
        bot.sendMessage(chat_id, "Speed set to " + String(secondsPerUnit) + " seconds per unit", "");
      } else {
        bot.sendMessage(chat_id, "Error: speed must be greater than 0", "");
      }
    }
    else if (text.startsWith("/rotate")) {
      // Rotate command: /rotate X
      String unitsStr = text.substring(8);
      int units = unitsStr.toInt();
      
      if (units != 0) {
        // Calculate target position
        int target = commandUnits + units;
        startMovement(target);
        
        if (units > 0) {
          bot.sendMessage(chat_id, "Rotating " + String(units) + " units clockwise", "");
        } else {
          bot.sendMessage(chat_id, "Rotating " + String(-units) + " units counter-clockwise", "");
        }
      } else {
        bot.sendMessage(chat_id, "Error: units must be non-zero", "");
      }
    }
    else if (text.startsWith("/stop")) {
      stopMovement();
      bot.sendMessage(chat_id, "Servo rotation stopped", "");
    }
    // For any other message, show the keyboard
    else {
      sendMainKeyboard(chat_id);
      continue; // Skip sending main keyboard again
    }
    
    // Send main keyboard after command execution (unless we're in custom input mode)
    sendMainKeyboard(chat_id);
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // Initialize LCD display
  initLCD();
  
  // LCD information
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  
  // Connect servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2500);
  
  // Set servo to stop position
  myServo.write(servoStopValue);
  
  // Initialize RGB LED strip
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(50); // Установка яркости (0-255)
  fill_solid(leds, NUM_LEDS, CRGB::Black); // Выключаем все светодиоды
  FastLED.show();
  
  // LCD information
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Servo initialized");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");
  
  // WiFi setup
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Wait for WiFi connection with LCD progress indication
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // Update progress indicator on LCD
    lcd.setCursor(wifiAttempts % LCD_COLUMNS, 1);
    lcd.print(".");
    wifiAttempts++;
    
    // If line is filled, clear it
    if (wifiAttempts % LCD_COLUMNS == 0) {
      lcd.setCursor(0, 1);
      lcd.print("                "); // 16 spaces
    }
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // LCD information about WiFi connection
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  
  // Secure connection setup
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  
  Serial.println("Bot started. Send any message to get the control menu");
  
  // System ready information
  delay(2000); // Show IP address for 2 seconds
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System ready");
  lcd.setCursor(0, 1);
  lcd.print("Unit: 0");
}

void loop() {
  // Update movement (advancing 1 unit per second or as configured)
  updateMovement();
  
  // Update RGB LED effects
  if (rainbowMode && rgbEnabled) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastLedUpdate >= ledUpdateInterval) {
      lastLedUpdate = currentMillis;
      updateRainbowEffect();
    }
  }
  
  // Update LCD information (if not moving)
  if (!isMoving) {
    updateLCD();
  }
  
  // Check for Telegram bot updates
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    
    if (numNewMessages) {
      Serial.println("New messages received");
      handleNewMessages(numNewMessages);
    }
    
    bot_lasttime = millis();
  }
}