# ESP32 Drone Control System

## Project Overview
This project implements a two-ESP32 drone control system consisting of:
1. **Drone Unit**: An ESP32 equipped with multiple sensors for environmental monitoring and positional awareness
2. **Controller Unit**: An ESP32 with LCD display, keypad interface, and web dashboard for remote control and monitoring

The system uses ESP-NOW for low-latency wireless communication between the units and provides both physical control through a keypad and remote control through a web interface.

## Hardware Components

### Drone Unit
- ESP32 microcontroller
- DHT11 temperature and humidity sensor
- 2× Ultrasonic distance sensors (front and rear)
- LDR (Light Dependent Resistor) for light level sensing
- 6× LEDs for directional indication:
  - Forward (pin 12)
  - Left (pin 14)
  - Back (pin 15)
  - Right (pin 13)
  - Up (pin 19)
  - Down (pin 21)

### Controller Unit
- ESP32 microcontroller
- 16×2 LCD display
- 4×4 matrix keypad
- Temperature warning LED (pin 35)
- WiFi connectivity for web server hosting

## Features

### Drone Unit
- **Sensor Array**: Collects comprehensive environmental data including:
  - Front and rear distance measurements
  - Ambient light levels
  - Temperature and humidity readings
- **Real-time Data Transmission**: Sends sensor data to the controller unit every 2 seconds
- **Direction Indicators**: Visual LED feedback for movement commands
- **Command Processing**: Receives and executes movement commands from the controller

### Controller Unit
- **Dual Control Interfaces**:
  - Physical 4×4 keypad for direct control
  - Web dashboard for remote monitoring and control
- **LCD Display**: Shows real-time sensor data with multiple viewing modes
- **Navigation Mode**: Dedicated mode for controlling drone movement
- **Web Dashboard**: Responsive HTML/CSS interface with:
  - Visually appealing cards for each sensor reading
  - Color-coded alerts for critical readings (e.g., high temperature)
  - Real-time data updates via AJAX

## Software Architecture

### Communication Protocol
- **ESP-NOW**: Low-latency peer-to-peer communication
- **Structured Data Transfer**:
  - `sensor_data_message`: Structured packet for sensor data
  - `command_message`: Simple command structure for movement control

### Drone Software Features
- Sensor data collection and processing
- ESP-NOW initialization and peer management
- Command reception and LED control
- Error handling for communication failures

### Controller Software Features
- Web server implementation with RESTful endpoints
- ESP-NOW communication with the drone
- LCD interface with multiple data views
- Keypad input processing
- Mode switching between data display and navigation

## Control System

### Keypad Layout and Functions
```
┌─────┬─────┬─────┬─────┐
│ 1   │ 2   │ 3   │ A   │  1: Up/Next View
│     │     │     │     │  2: Forward
├─────┼─────┼─────┼─────┤  3: Down/Previous View
│ 4   │ 5   │ 6   │ B   │  4: Left
├─────┼─────┼─────┼─────┤  5: Backward
│ 7   │ 8   │ 9   │ C   │  6: Right
├─────┼─────┼─────┼─────┤
│ *   │ 0   │ #   │ D   │  #: General Mode
│                       │  D: Navigation Mode
└───────────────────────┘
```

### Navigation Commands
- 'f': Move forward
- 'b': Move backward
- 'l': Turn left
- 'r': Turn right
- 'u': Move up
- 'd': Move down

### Display Modes
The controller cycles through multiple information displays:
1. Temperature and humidity
2. Front and rear distance readings
3. Light level and ESP32 distance
4. Device ID and system status

## Web Interface
The responsive web dashboard provides:
- Real-time monitoring of all sensor data
- Visual indicators for data status
- Clean, modern dark-themed UI with responsive design

## Installation and Setup

### Prerequisites
- Arduino IDE with ESP32 board support
- Required libraries:
  - WiFi.h
  - WebServer.h
  - LiquidCrystal.h
  - Keypad.h
  - esp_now.h
  - DHT.h

### Configuration
1. Update the MAC addresses in both files to match your ESP32 devices:
   - Set `receiverMacAddress` in the drone code
   - Set `senderMacAddress` in the controller code
2. Configure WiFi credentials in the controller code:
   ```cpp
   const char* ssid = "Your-WiFi-Name";
   const char* password = "Your-WiFi-Password";
   ```

### Wiring Diagrams
*Note: Add detailed wiring diagrams or reference to them here*

## Usage Instructions

### Controller Operation
1. **Power on** both the drone and controller units
2. The controller LCD will display the IP address for web dashboard access
3. **Navigation Mode**: Press 'T' to enter navigation mode, then use:
   - F, B, L, R, U, D keys for movement
4. **Data Display Mode**: Press 'G' to enter general mode, then use:
   - U/D to cycle through different sensor displays

### Web Dashboard
1. Connect to the same WiFi network as the controller
2. Navigate to the controller's IP address in a web browser
3. The dashboard will automatically update with real-time sensor data
4. Use the on-screen controls for remote operation

## Safety Features
- Temperature warning LED activates when temperature exceeds 30°C
- Visual indicators on the web dashboard for critical readings
- Distance sensor data helps prevent collisions

## Future Enhancements
- Battery level monitoring
- GPS integration
- Autonomous navigation capabilities
- Video streaming
- Enhanced data logging and analytics

## Troubleshooting
- **Communication Issues**: Verify MAC addresses are correctly configured
- **Sensor Errors**: Check wiring and connections
- **Web Dashboard Not Loading**: Confirm WiFi connectivity and correct IP address
