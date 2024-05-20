#include <Arduino.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>

// Define sensor pins and other constants
const int trigPin = 7;
const int echoPin = 6;
const int mqPin = A0;
const int ledPin = 12;
const int buzzerPin = 11;
int distance = 0;
int gasValue = 0;
bool oilStatus=false;
bool gasStatus=false;
bool keyPadClicked = true;
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
const char correctCode[5] = "4717";
char enteredCode[5];
byte codeIndex = 0;

// Initialize LCD and SoftwareSerial
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(2, 3);
byte rowPins[ROWS] = {A1, A2, A3, 4};
byte colPins[COLS] = {5, 8, 9, 10};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
    // Initialize serial communication and LCD
    Serial.begin(9600);
    delay(2000);
    Serial.println("Process Started...");
    mySerial.begin(115200);
    lcd.init();
    lcd.backlight();
    
    // Set pin modes
    pinMode(ledPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(mqPin, INPUT);
}

void loop() {
    // Check oil and gas sensor status
    oilStatus = OilSensor();
    gasStatus = GasSensor();
    bool overallStatus = oilStatus && gasStatus;
    if (overallStatus) {
        // If both sensors are okay, display good status
        ledGood();
        buzzerGood();
        lcdGood();
    } else {
        // If any sensor detects a problem, display bad status, sound alarm, and send alert
        ledBad();
        buzzerBad();
        lcdBad();
        sendAlert();
        while(keyPadClicked){
            receiveKeyPad();
            delay(100);
        }
        keyPadClicked = true;
    }
    // Send sensor data periodically
    sendStoreData();
    delay(3000);
}

// Function to check oil sensor status
bool OilSensor() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;
    Serial.print(distance);
    Serial.println(" cm");
    return distance <= 8;
}

// Function to check gas sensor status
bool GasSensor() {
    gasValue = analogRead(mqPin);
    Serial.print(gasValue);
    Serial.println(" units");
    return gasValue <= 1500;
}

// Functions to control LED and buzzer for good and bad status
void ledGood() {
    digitalWrite(ledPin,HIGH);
    delay(750);
    digitalWrite(ledPin,LOW);
    delay(750);
    digitalWrite(ledPin,HIGH);
    delay(750);
    digitalWrite(ledPin,LOW);
    delay(750);
    digitalWrite(ledPin,HIGH);
}

void ledBad() {
    digitalWrite(ledPin,HIGH);
    delay(150);
    digitalWrite(ledPin,LOW);
    delay(150);
    digitalWrite(ledPin,HIGH);
    delay(150);
    digitalWrite(ledPin,LOW);
    delay(150);
    digitalWrite(ledPin,HIGH);
}

void buzzerGood() {
    digitalWrite(buzzerPin,LOW);
}

void buzzerBad() {
    digitalWrite(buzzerPin,HIGH);
}

// Functions to control LCD display for good, bad, and invalid status
void lcdGood() {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Status: GOOD");
}

void lcdBad() {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Status: BAD");
}

// Function to send sensor data over serial
void sendStoreData() {
    StaticJsonDocument<512> sendData;
    sendData["IT"] = "D";
    sendData["OV"] = distance;
    sendData["OS"] = oilStatus;
    sendData["GV"] = gasValue;
    sendData["GS"] = gasStatus;
    sendData["T"] = 0;
    String sendJsonString;
    serializeJson(sendData, sendJsonString);
    mySerial.print(sendJsonString);
    Serial.println(sendJsonString);
}

// Function to send alert over serial
void sendAlert() {
  if(oilStatus){
    mySerial.print("{\"IT\":\"A\",\"SN\":2}");
    Serial.println("{\"IT\":\"A\",\"SN\":2}");
  }
  else{
    mySerial.print("{\"IT\":\"A\",\"SN\":1}");
    Serial.println("{\"IT\":\"A\",\"SN\":1}");
  }
}

// Function to receive keypad input over serial
void receiveKeyPad() {
    char key = keypad.getKey();
    if (key) {
        Serial.print("Key pressed: ");
        Serial.println(key);
        enteredCode[codeIndex] = key;
        codeIndex++;
        if (codeIndex == 4) {
            enteredCode[4] = '\0';
            if (strcmp(enteredCode, correctCode) == 0) {
                keyPadClicked = false;
            }
            else{
              keyPadClicked = true;
            }
            codeIndex = 0; // Reset code index for next entry
        }
    }
}