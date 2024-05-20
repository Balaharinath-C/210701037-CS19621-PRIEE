#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

const char* ssid = "THUNDERBOLT";
const char* password = "balaharinath";
const char* storeDataURL = "http://192.168.137.1:5000/storeData";
const char* alertURL = "http://192.168.137.1:5000/alert";

SoftwareSerial mySerial(4, 5);

char dataInformationType[2]; // Assuming it's a string of max length 1 character + null terminator
char dataString[128]; // Adjust size as needed
char enteredPassword[5]; // Assuming it's a string of max length 4 characters + null terminator

void setup() {
  Serial.begin(9600);
  delay(2000);
  Serial.println("Process Started...");
  mySerial.begin(115200);
  connectToWiFi();
}

void loop() {
  receiveSerialData();
  if (strcmp(dataInformationType, "D") == 0 || strcmp(dataInformationType, "A") == 0) {
    const char* url = (strcmp(dataInformationType, "D") == 0) ? storeDataURL : alertURL;
    sendHTTPRequest(url);
  }
  delay(3000);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to THUNDERBOLT Wifi...");
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

void receiveSerialData() {
  if (mySerial.available() > 0) {
    StaticJsonDocument<128> receiveData;
    char receiveJsonBuffer[128];
    mySerial.readBytesUntil('\n', receiveJsonBuffer, sizeof(receiveJsonBuffer));
    
    // Find the position of the first '{' and '}' characters
    char* braceStart = strstr(receiveJsonBuffer, "{");
    char* braceEnd = strstr(receiveJsonBuffer, "}");
    
    if (braceStart != NULL && braceEnd != NULL) {
        // Calculate the length of the substring between '{' and '}'
        int substringLength = braceEnd - braceStart + 1;
        
        // Create a buffer to store the substring
        char extractedDataBuffer[substringLength];
        
        // Copy the substring between '{' and '}' to the new buffer
        strncpy(extractedDataBuffer, braceStart, substringLength);
        extractedDataBuffer[substringLength] = '\0'; // Null-terminate the string
        
        DeserializationError error = deserializeJson(receiveData, extractedDataBuffer);
        if (!error) {
            strcpy(dataInformationType, receiveData["IT"]);
            strcpy(dataString, extractedDataBuffer); // Store the extracted data string
            Serial.println(dataString);
        } else {
            Serial.println("Failed to parse JSON...");
            Serial.println(error.c_str());
        }
    } else {
        Serial.println("Failed to find '{' and/or '}' characters in the received data...");
    }
  }
}

void sendHTTPRequest(const char* url) {
  HTTPClient http;
  WiFiClient client;
  if (http.begin(client, url)) {
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(dataString);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Failed to connect to server.");
  }
  dataString[0] = '\0';
  dataInformationType[0] = '\0';
}