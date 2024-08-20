/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

// Libraries for SD card
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>

//DS18B20 libraries
#include "DHT.h"

// Libraries to get time from NTP Server
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Replace with your network credentials
const char* ssid     = "Hoppala4";
const char* password = "Tekirdag2011!";

// Define CS pin for the SD card module
#define SD_CS 5
//Define hoe lang WiFi mag proberen te connecten 
#define WIFI_TIMEOUT_MS 20000

// Save reading number on RTC memory
char readingID[] = "DHT11";

String dataMessage;

// Data wire is connected to ESP32 GPIO 7
#define DHTPIN 32
#define DHTTYPE DHT11
// Setup a oneWire instance to communicate with a OneWire device
DHT dht(DHTPIN, DHTTYPE);

// Temperature Sensor variables
float temperature;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

// Function to get temperature
void getReadings(){
  temperature = dht.readTemperature(); // Temperature in Celsius
    
  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.println(temperature);
}

// Function to get date and time from NTPClient
void getTimeStamp() {
      timeClient.update();
      // Extract time
      timeStamp = String(timeClient.getEpochTime() * 1000ULL);
      Serial.println(timeStamp);
}

// Write the sensor readings on the SD card
void logSDCard() {
  dataMessage = String(readingID) + "," + String(dayStamp) + "," + String(timeStamp) + "," + 
                String(temperature) + "\r\n";
  Serial.print("Save data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/data.txt", dataMessage.c_str());
}

// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void keepWifiAlive(void * parameters){
  for(;;){
    if(WiFi.status() == WL_CONNECTED){
      Serial.println("WiFi still connected");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();

    while(WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS){}

    if(WiFi.status() != WL_CONNECTED){
      Serial.println("WIFI FAILED");
      vTaskDelay(20000 / portTICK_PERIOD_MS);
      continue;
    }

    Serial.println("WiFi Connected: " + WiFi.localIP());
  }
}

void task1(void *parameters) {
  dht.begin(); 
  for(;;){
    getReadings();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task2(void *parameters) {
  timeClient.begin();
  timeClient.setTimeOffset(7200);
  for(;;){
     getTimeStamp();
     vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task3(void *parameters) {
  for(;;){
    logSDCard();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // Start serial communication for debugging purposes
  Serial.begin(115200);

  // Initialize SD card
  SD.begin(SD_CS);  
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/data.txt");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "Reading ID, Date, Hour, Temperature \r\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();

  // Start the DallasTemperature library
   
   xTaskCreatePinnedToCore(keepWifiAlive, "keep Wifi alive", 5000, NULL, 2, NULL, CONFIG_ARDUINO_RUNNING_CORE);
   xTaskCreate(task1, "Temp sensor", 1000, NULL, 1, NULL);
   xTaskCreate(task2, "date", 5000, NULL, 1, NULL);
   
}

void loop() {
  // The ESP32 will be in deep sleep
  // it never reaches the loop()
}

