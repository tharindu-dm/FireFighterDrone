#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <esp_now.h>

// WiFi credentials
const char* ssid = "Dimuthu's Galaxy A53 5G";      // Replace with your WiFi name
const char* password = "Dimuthu@28";   // Replace with your WiFi password

WebServer server(80);

// LED pin for temperature warning
#define TEMP_WARNING_LED 35

// LCD setup (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(19, 23, 18, 17, 16, 15);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'U', 'F', 'D', ' '},
  {'L', 'B', 'R', ' '},
  {' ', ' ', ' ', ' '},
  {' ', ' ', 'G', 'T'}
};

byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12, 13};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// Sensor data structure
typedef struct sensor_data_message {
    int id;
    float front_distance_cm;
    float rear_distance_cm;
    int ldr_value;
    int temperature;
    int humidity;
    float esp32_distance;
} sensor_data_message;

// Command structure for navigation
typedef struct command_message {
    char direction;
} command_message;

sensor_data_message receivedData;
command_message commandData;
volatile bool newDataReceived = false;
int currentView = 0;
bool navigationMode = false;

// MAC Address of the drone ESP32 (replace with your drone's MAC)
uint8_t senderMacAddress[] = {0x24, 0x6F, 0x28, 0x15, 0xB3, 0xD0};
esp_now_peer_info_t peerInfo;

// HTML webpage
const char* html_page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Drone Control Dashboard</title>
    <style>
        :root {
            --primary: #ff6b00;
            --secondary: #cc5500;
            --success: #22c55e;
            --warning: #eab308;
            --danger: #ef4444;
            --background: #1a1a1a;
            --card-bg: #2d2d2d;
            --text: #ffffff;
            --text-secondary: #a3a3a3;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        }

        body {
            background: var(--background);
            padding: 2rem;
            color: var(--text);
        }

        .container {
            max-width: 1200px;
            margin: 0 auto;
        }

        .header {
            text-align: center;
            margin-bottom: 2rem;
            color: var(--primary);
        }

        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 1.5rem;
            margin-bottom: 2rem;
        }

        .card {
            background: var(--card-bg);
            border-radius: 1rem;
            padding: 1.5rem;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.3);
            transition: transform 0.2s;
        }

        .card:hover {
            transform: translateY(-5px);
        }

        .card-title {
            color: var(--text);
            font-size: 1.1rem;
            margin-bottom: 1rem;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        .value {
            font-size: 2rem;
            font-weight: bold;
            color: var(--primary);
            margin: 0.5rem 0;
        }

        .unit {
            color: var(--text-secondary);
            font-size: 0.9rem;
        }

        .controls {
            background: var(--card-bg);
            border-radius: 1rem;
            padding: 2rem;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.3);
        }

        .control-title {
            text-align: center;
            color: var(--primary);
            margin-bottom: 1.5rem;
        }

        .navigation {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 0.5rem;
            max-width: 300px;
            margin: 0 auto;
        }

        .nav-btn {
            background: var(--primary);
            color: var(--text);
            border: none;
            padding: 1rem;
            border-radius: 0.5rem;
            cursor: pointer;
            font-size: 1.1rem;
            transition: background 0.2s;
        }

        .nav-btn:hover {
            background: var(--secondary);
        }

        .nav-btn:active {
            transform: scale(0.95);
        }

        .mode-switch {
            text-align: center;
            margin-top: 1.5rem;
        }

        .switch-btn {
            background: var(--success);
            color: var(--text);
            border: none;
            padding: 0.75rem 1.5rem;
            border-radius: 0.5rem;
            cursor: pointer;
            margin: 0 0.5rem;
            transition: background 0.2s;
        }

        .switch-btn:hover {
            opacity: 0.9;
        }

        .warning {
            color: var(--warning);
        }

        .danger {
            color: var(--danger);
        }

        @media (max-width: 768px) {
            body {
                padding: 1rem;
            }
            
            .dashboard {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <header class="header">
            <h1>Drone Control Dashboard</h1>
            <p>Real-time sensor monitoring and navigation control</p>
        </header>

        <section class="dashboard">
            <div class="card">
                <h2 class="card-title">
                    <span class="material-icons">🌡</span>
                    Temperature
                </h2>
                <div class="value" id="temperature">--</div>
                <div class="unit">°C</div>
            </div>

            <div class="card">
                <h2 class="card-title">
                    <span class="material-icons">💧</span>
                    Humidity
                </h2>
                <div class="value" id="humidity">--</div>
                <div class="unit">%</div>
            </div>

            <div class="card">
                <h2 class="card-title">
                    <span class="material-icons">↔</span>
                    Front Distance
                </h2>
                <div class="value" id="front-distance">--</div>
                <div class="unit">cm</div>
            </div>

            <div class="card">
                <h2 class="card-title">
                    <span class="material-icons">↔</span>
                    Rear Distance
                </h2>
                <div class="value" id="rear-distance">--</div>
                <div class="unit">cm</div>
            </div>

            <div class="card">
                <h2 class="card-title">
                    <span class="material-icons">💡</span>
                    Light Level
                </h2>
                <div class="value" id="light-level">--</div>
                <div class="unit">LDR value</div>
            </div>
        </section>
    </div>

    <script>
        function updateSensorData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temperature').textContent = data.temperature;
                    document.getElementById('humidity').textContent = data.humidity;
                    document.getElementById('front-distance').textContent = data.front_distance_cm.toFixed(1);
                    document.getElementById('rear-distance').textContent = data.rear_distance_cm.toFixed(1);
                    document.getElementById('light-level').textContent = data.ldr_value;
                    document.getElementById('esp32-distance').textContent = data.esp32_distance.toFixed(1);

                    if (data.temperature > 30) {
                        document.getElementById('temperature').classList.add('danger');
                    } else {
                        document.getElementById('temperature').classList.remove('danger');
                    }
                })
                .catch(error => console.error('Error:', error));
        }

        function sendCommand(direction) {
            fetch('/command', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ direction: direction })
            })
            .then(response => response.text())
            .then(data => console.log('Command sent:', data))
            .catch(error => console.error('Error:', error));
        }

        function toggleMode(mode) {
            fetch('/mode', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ mode: mode })
            })
            .then(response => response.text())
            .then(data => console.log('Mode changed:', data))
            .catch(error => console.error('Error:', error));
        }

        // Update sensor data every 2 seconds
        setInterval(updateSensorData, 2000);
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
    server.send(200, "text/html", html_page);
}

void handleData() {
    String json = "{";
    json += "\"id\":" + String(receivedData.id) + ",";
    json += "\"front_distance_cm\":" + String(receivedData.front_distance_cm) + ",";
    json += "\"rear_distance_cm\":" + String(receivedData.rear_distance_cm) + ",";
    json += "\"ldr_value\":" + String(receivedData.ldr_value) + ",";
    json += "\"temperature\":" + String(receivedData.temperature) + ",";
    json += "\"humidity\":" + String(receivedData.humidity) + ",";
    json += "\"esp32_distance\":" + String(receivedData.esp32_distance);
    json += "}";
    
    server.send(200, "application/json", json);
}

void handleCommand() {
    if (server.hasArg("plain")) {
        String message = server.arg("plain");
        char direction = message[message.indexOf("direction") + 11];
        sendCommand(direction);
        server.send(200, "text/plain", "Command received: " + String(direction));
    }
}

void handleMode() {
    if (server.hasArg("plain")) {
        String message = server.arg("plain");
        char mode = message[message.indexOf("mode") + 7];
        if (mode == 'T') {
            navigationMode = true;
            displayNavigationMode();
        } else if (mode == 'G') {
            navigationMode = false;
            updateDisplay();
        }
        server.send(200, "text/plain", "Mode changed: " + String(mode));
    }
}

void displayNavigationMode() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Navigation Mode");
    lcd.setCursor(0, 1);
    lcd.print("Use F,B,L,R,U,D");
}

void updateDisplay() {
    lcd.clear();
    
    switch(currentView) {
        case 0:
            lcd.setCursor(0, 0);
            lcd.print("Temp: ");
            lcd.print(receivedData.temperature);
            lcd.print("C");
            lcd.setCursor(0, 1);
            lcd.print("Humidity: ");
            lcd.print(receivedData.humidity);
            lcd.print("%");
            break;
            
        case 1:
            lcd.setCursor(0, 0);
            lcd.print("Front: ");
            lcd.print(receivedData.front_distance_cm, 1);
            lcd.print("cm");
            lcd.setCursor(0, 1);
            lcd.print("Rear: ");
            lcd.print(receivedData.rear_distance_cm, 1);
            lcd.print("cm");
            break;
            
        case 2:
            lcd.setCursor(0, 0);
            lcd.print("Light: ");
            lcd.print(receivedData.ldr_value);
            lcd.setCursor(0, 1);
            lcd.print("Dist: ");
            lcd.print(receivedData.esp32_distance, 1);
            lcd.print("m");
            break;
            
        case 3:
            lcd.setCursor(0, 0);
            lcd.print("Device ID: ");
            lcd.print(receivedData.id);
            lcd.setCursor(0, 1);
            lcd.print("System Active");
            break;
    }
}

void sendCommand(char direction) {
    commandData.direction = direction;
    esp_err_t result = esp_now_send(senderMacAddress, (uint8_t *)&commandData, sizeof(commandData));
    
    if (result == ESP_OK) {
        lcd.setCursor(0, 1);
        lcd.print("Sent: ");
        lcd.print(direction);
        lcd.print("       ");
    }
}

void onReceive(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
    if (len == sizeof(sensor_data_message)) {
        memcpy(&receivedData, incomingData, sizeof(sensor_data_message));
        newDataReceived = true;
        
        // Check temperature and control warning LED
        if (receivedData.temperature > 30) {
            digitalWrite(TEMP_WARNING_LED, HIGH);
        } else {
            digitalWrite(TEMP_WARNING_LED, LOW);
        }
        
        if (!navigationMode) {
            updateDisplay();
        }
        
        // Print to Serial for debugging
        Serial.print("ID: ");
        Serial.print(receivedData.id);
        Serial.print(" | Temp: ");
        Serial.print(receivedData.temperature);
        Serial.print("C | Humidity: ");
        Serial.print(receivedData.humidity);
        Serial.print("% | Front: ");
        Serial.print(receivedData.front_distance_cm);
        Serial.print("cm | Rear: ");
        Serial.print(receivedData.rear_distance_cm);
        Serial.print("cm | Light: ");
        Serial.print(receivedData.ldr_value);
        Serial.print(" | Distance: ");
        Serial.print(receivedData.esp32_distance);
        Serial.println("m");
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize temperature warning LED
    pinMode(TEMP_WARNING_LED, OUTPUT);
    digitalWrite(TEMP_WARNING_LED, LOW);
    
    // Initialize LCD
    lcd.begin(16, 2);
    lcd.print("Drone Dashboard");
    lcd.setCursor(0, 1);
    lcd.print("Connecting...");
    
    // Connect to WiFi
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    lcd.print("IP Address:");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Failed!");
        lcd.clear();
        lcd.print("ESP-NOW Failed!");
        return;
    }
    
    // Register peer
    memcpy(peerInfo.peer_addr, senderMacAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
    
    esp_now_register_recv_cb(onReceive);
    
    // Set up web server routes
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.on("/command", HTTP_POST, handleCommand);
    server.on("/mode", HTTP_POST, handleMode);
    
    // Start server
    server.begin();
    Serial.println("HTTP server started");
    
    delay(2000);
    lcd.clear();
    updateDisplay();
}

void loop() {
    server.handleClient();
    
    char customKey = customKeypad.getKey();
    
    if (customKey) {
        if (customKey == 'T') {
            navigationMode = true;
            displayNavigationMode();
        }
        else if (customKey == 'G') {
            navigationMode = false;
            updateDisplay();
        }
        else if (navigationMode) {
            switch(customKey) {
                case 'F': sendCommand('f'); break;
                case 'B': sendCommand('b'); break;
                case 'L': sendCommand('l'); break;
                case 'R': sendCommand('r'); break;
                case 'U': sendCommand('u'); break;
                case 'D': sendCommand('d'); break;
            }
        }
        else {
            switch(customKey) {
                case 'U':
                    currentView = (currentView + 1) % 4;
                    updateDisplay();
                    break;
                case 'D':
                    currentView = (currentView + 3) % 4;
                    updateDisplay();
                    break;
            }
        }
    }
    
    if (newDataReceived && !navigationMode) {
        updateDisplay();
        newDataReceived = false;
    }
}