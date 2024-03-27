// Include necessary libraries for BLE and VESC communication
#include <Arduino.h> // Core Arduino library for basic programming
#include <BLEDevice.h> // BLE library for creating BLE server and characteristics
#include <BLEServer.h> // Specific BLE server functionalities
#include <BLEUtils.h> // Utilities for BLE operations
#include <VescUart.h> // Library to interface with VESC over UART
#include <ArduinoSTL.h>

// Define UUIDs for BLE service and characteristics, uni1que identifiers for BLE communication
#define SERVICE_UUID "82f736e2-0115-4a37-9498-f357806b45d8"
#define SPEED_UUID "b5fa25e0-884e-475a-9a70-a286b88df9f5"
#define BATTERY_PCT_UUID "ec76c264-0bc4-4eaa-b32e-01c723e9cfe3"
#define DIST_REMAINING_UUID "abc4af9e-7220-45e5-bc87-137d35dafb4d" 
#define DIST_TRAVELED_UUID "6043959f-c355-40c3-a069-9e511f335793"

// Calculations
#define POLES 14.0
#define WHEEL_DIAMETER 0.1524 // in meter
#define BAT_MIN_VOLTAGE 33 // Empty
#define BAT_MAX_VOLTAGE 42 // Full

VescUart UART; // Create an instance of the VescUart class to communicate with the VESC

// Pointers for BLE characteristics
BLECharacteristic *pSpeed, *pBattPct, *pDistRem, *pDistTr;

HardwareSerial uartSerial(2);

void advertiseBle(){
  // Set up and start BLE advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); // Get the advertising object
  pAdvertising->addServiceUUID(SERVICE_UUID); // Advertise the service UUID
  pAdvertising->setScanResponse(true); // Allow scan responses
  pAdvertising->setMinPreferred(0x06); // Set minimum preferred advertising interval
  pAdvertising->setMinPreferred(0x12); // Set maximum preferred advertising interval (possible typo, should be setMaxPreferred)
  BLEDevice::startAdvertising(); // Start advertising

  // Serial.println("Characteristic defined! Now you can read it on your phone!"); // Debug message indicating setup completion
}

void setupBle(){
  // Serial.println("Starting BLE!");
  BLEServer *pServer = BLEDevice::createServer(); // Create a BLE server

  // Create a BLE service and its characteristics with read properties
  BLEService *pService = pServer->createService(SERVICE_UUID); // Create BLE service with a specific UUID
  pSpeed = pService->createCharacteristic(SPEED_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY); // Create battery percentage characteristic
  pBattPct = pService->createCharacteristic(BATTERY_PCT_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY); // Create battery watt-hours characteristic
  pDistRem = pService->createCharacteristic(DIST_REMAINING_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY); // Create RPM characteristic
  pDistTr = pService->createCharacteristic(DIST_TRAVELED_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY); // Create predicted range characteristic

  pService->start(); // Start the BLE service

  advertiseBle();
}



struct sb_data{
  float speed;
  float batteryPct;
  float distanceRemaining;
  float distanceTraveled;
} state;

std::deque<float> speedQueue;

void setup() {
  // Serial2.begin(115200, SERIAL_8N1, 3, 1);
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate for debugging
  // Serial.println("Starting BLE work!"); // Debug print to indicate the start of BLE setup

  // Initialize BLE device with a specific name and create a BLE server
  BLEDevice::init("Electrium e-skateboard"); // Initialize BLE device with a name for the e-skateboard

  setupBle();
  advertiseBle();

  // init state
  state = {
    .speed = 0,
    .batteryPct = 1,
    .distanceRemaining = -1,
    .distanceTraveled = 0,
  };

  uartSerial.begin(115200, SERIAL_8N1, 3, 1); // Initialize UART serial communication with VESC (RX, TX, RTS, CTS
  UART.setSerialPort(&uartSerial); // Associate the VescUart instance with the serial port for communication
}
float t = 0;

bool fetchVescData(){
  Serial.print("R");
  return UART.getVescValues();
}

bool fetchDummyData(){
  // Serial.print("E");
  // UART.data.rpm = 5;
  // UART.data.inpVoltage = 40;
  // UART.data.tachometerAbs = 1000;
  return true;
}

float getBatteryPercentage() {
  return ((UART.data.inpVoltage - BAT_MIN_VOLTAGE) / (BAT_MAX_VOLTAGE - BAT_MIN_VOLTAGE)) * 100;
}

float getSpeed() {
  int rpm = (UART.data.rpm) / (POLES / 2);
        
  // We do not need to pay attention to the translation because it is 1 to 1.
  float velocity = abs(rpm) * PI * (60.0 / 1000.0) * WHEEL_DIAMETER;

  if (velocity > 99) {
    return 99.0;
  }
  return velocity;
}

float getTrip() {
  float tach = (UART.data.tachometerAbs) / (POLES * 3.0);
        
  // We do not need to pay attention to the translation because it is 1 to 1.
  float distance = tach * PI * (1.0/1000.0) * WHEEL_DIAMETER;

  if (distance > 99) {
    return 99.0;
  }
  return distance;
}


void updateState(){
  if(fetchVescData() || fetchDummyData()){
    state.speed = getSpeed();
    state.batteryPct = getBatteryPercentage();
    state.distanceTraveled = getTrip();

    auto distPpct = state.distanceTraveled / state.batteryPct;
    state.distanceRemaining = distPpct * (100-state.batteryPct);

    // Serial.print(" Speed: ");
    // Serial.print(state.speed);

    // Serial.print(" Batt Pct: ");
    // Serial.print(state.batteryPct);

    // Serial.print(" Dist Tr: ");
    // Serial.print(state.distanceTraveled);

    // Serial.print(" Dist Rem: ");
    // Serial.print(state.distanceRemaining);

    // Serial.println();
  }
}

void loop() {
  updateState();

  pSpeed->setValue(state.speed); // Set battery percentage from VESC data
  pBattPct->setValue(state.batteryPct); // Set battery watt-hours from VESC data
  pDistRem->setValue(state.distanceRemaining); // Set RPM from VESC data
  pDistTr->setValue(state.distanceTraveled); // Set predicted range from VESC data
  pSpeed->notify();
  pBattPct->notify();
  pDistRem->notify();
  pDistTr->notify();

  delay(50); // Short delay to throttle loop execution speed
}
