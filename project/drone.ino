#include "DHT.h"
#include <WiFi.h>
#include <esp_now.h>

// Pin definitions
#define FRONT_TRIG_PIN 5
#define FRONT_ECHO_PIN 18
#define REAR_TRIG_PIN 22    
#define REAR_ECHO_PIN 23    
#define LDR_PIN 34
#define DHTTYPE DHT11
#define DHTPIN 4

// LED pins
#define FORWARD 12
#define LEFT 14
#define BACK 15
#define RIGHT 13
#define UP 19
#define DOWN 21

    int packID = 1;

// Distance between ESP32s in meters
const float ESP32_DISTANCE = 10.0;

// MAC Address of the controller ESP32 (replace with your controller's MAC)
uint8_t receiverMacAddress[] = {0x3C, 0x71, 0xBF, 0xF9, 0xE1, 0x50};

// Sensor data structure
typedef struct sensor_data_message {
    int id;  // Added ID field from working code
    float front_distance_cm;
    float rear_distance_cm;
    int ldr_value;
    int temperature;
    int humidity;
    float esp32_distance;
} sensor_data_message;

// Command structure
typedef struct command_message {
    char direction;
} command_message;

// Create structured objects
sensor_data_message sensorData;
command_message commandReceived;
esp_now_peer_info_t peerInfo;

DHT dht(DHTPIN, DHTTYPE);

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    
    Serial.print("Last Packet Sent to: "); 
    Serial.println(macStr);
    Serial.print("Last Packet Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void OnDataReceived(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
    if (data_len == sizeof(command_message)) {
        memcpy(&commandReceived, data, sizeof(command_message));
        
        // Turn off all LEDs first
        digitalWrite(FORWARD, LOW);
        digitalWrite(LEFT, LOW);
        digitalWrite(BACK, LOW);
        digitalWrite(RIGHT, LOW);
        digitalWrite(UP, LOW);
        digitalWrite(DOWN, LOW);
        
        // Process command
        switch(commandReceived.direction) {
            case 'f': digitalWrite(FORWARD, HIGH); break;
            case 'b': digitalWrite(BACK, HIGH); break;
            case 'l': digitalWrite(LEFT, HIGH); break;
            case 'r': digitalWrite(RIGHT, HIGH); break;
            case 'u': digitalWrite(UP, HIGH); break;
            case 'd': digitalWrite(DOWN, HIGH); break;
        }
        
        Serial.print("Command received: ");
        Serial.println(commandReceived.direction);
    }
}

float readUltrasonicDistance(int trigPin, int echoPin) {
    long duration;
    
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    return (duration * 0.0343) / 2;
}

void setup() {
    Serial.begin(115200);
    
    // Initialize pins
    pinMode(FRONT_TRIG_PIN, OUTPUT);
    pinMode(FRONT_ECHO_PIN, INPUT);
    pinMode(REAR_TRIG_PIN, OUTPUT);
    pinMode(REAR_ECHO_PIN, INPUT);
    pinMode(LDR_PIN, INPUT);
    
    pinMode(FORWARD, OUTPUT);
    pinMode(LEFT, OUTPUT);
    pinMode(BACK, OUTPUT);
    pinMode(RIGHT, OUTPUT);
    pinMode(UP, OUTPUT);
    pinMode(DOWN, OUTPUT);
    
    // Initialize DHT sensor
    dht.begin();
    
    // Initialize ESP-NOW
    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    // Register callbacks
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataReceived);
    
    // Register peer
    memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    // Print this device's MAC address
    Serial.print("Drone MAC Address: ");
    Serial.println(WiFi.macAddress());
    
}

void loop() {
    // Update sensor data
    sensorData.id = packID++;  // Added from working code
    sensorData.front_distance_cm = readUltrasonicDistance(FRONT_TRIG_PIN, FRONT_ECHO_PIN);
    sensorData.rear_distance_cm = readUltrasonicDistance(REAR_TRIG_PIN, REAR_ECHO_PIN);
    sensorData.ldr_value = analogRead(LDR_PIN);
    sensorData.humidity = dht.readHumidity();
    sensorData.temperature = dht.readTemperature();
    sensorData.esp32_distance = ESP32_DISTANCE;

    // Print sensor values
    Serial.print("ID: ");
    Serial.print(sensorData.id);
    Serial.print(" | Front Distance: ");
    Serial.print(sensorData.front_distance_cm);
    Serial.print(" cm | Rear Distance: ");
    Serial.print(sensorData.rear_distance_cm);
    Serial.print(" cm | LDR: ");
    Serial.print(sensorData.ldr_value);
    Serial.print(" | Temp: ");
    Serial.print(sensorData.temperature);
    Serial.print("°C | Humidity: ");
    Serial.print(sensorData.humidity);
    Serial.print("% | ESP32 Distance: ");
    Serial.print(sensorData.esp32_distance);
    Serial.println(" m");

    // Send sensor data
    esp_err_t result = esp_now_send(receiverMacAddress, (uint8_t *)&sensorData, sizeof(sensorData));
    
    if (result != ESP_OK) {
        Serial.println("Error sending the data");
    }

    delay(2000);
}