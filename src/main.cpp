// Include necessary libraries for BLE and VESC communication
#include <Arduino.h> // Core Arduino library for basic programming
#include <BLEDevice.h> // BLE library for creating BLE server and characteristics
#include <BLEServer.h> // Specific BLE server functionalities
#include <BLEUtils.h> // Utilities for BLE operations
#include <VescUart.h> // Library to interface with VESC over UART

// Define UUIDs for BLE service and characteristics, unique identifiers for BLE communication
#define SERVICE_UUID "82f736e2-0115-4a37-9498-f357806b45d8" // UUID for the main BLE service
#define BATTERY_PCT_UUID "b5fa25e0-884e-475a-9a70-a286b88df9f5" // UUID for battery percentage characteristic
#define BATTERY_WH_UUID "ec76c264-0bc4-4eaa-b32e-01c723e9cfe3" // UUID for battery watt-hours characteristic
#define RPM_UUID "abc4af9e-7220-45e5-bc87-137d35dafb4d" // UUID for RPM characteristic
#define PRD_RANGE_UUID "6043959f-c355-40c3-a069-9e511f335793" // UUID for predicted range characteristic

VescUart UART; // Create an instance of the VescUart class to communicate with the VESC

// Pointers for BLE characteristics
BLECharacteristic *pBattPct, *pBattWh, *pRpm, *pPrdRange;

void setup() {
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate for debugging
  Serial.println("Starting BLE work!"); // Debug print to indicate the start of BLE setup

  // Initialize BLE device with a specific name and create a BLE server
  BLEDevice::init("Electrium e-skateboard"); // Initialize BLE device with a name for the e-skateboard
  BLEServer *pServer = BLEDevice::createServer(); // Create a BLE server

  // Create a BLE service and its characteristics with read properties
  BLEService *pService = pServer->createService(SERVICE_UUID); // Create BLE service with a specific UUID
  pBattPct = pService->createCharacteristic(BATTERY_PCT_UUID, BLECharacteristic::PROPERTY_READ); // Create battery percentage characteristic
  pBattWh = pService->createCharacteristic(BATTERY_WH_UUID, BLECharacteristic::PROPERTY_READ); // Create battery watt-hours characteristic
  pRpm = pService->createCharacteristic(RPM_UUID, BLECharacteristic::PROPERTY_READ); // Create RPM characteristic
  pPrdRange = pService->createCharacteristic(PRD_RANGE_UUID, BLECharacteristic::PROPERTY_READ); // Create predicted range characteristic

  pService->start(); // Start the BLE service

  // Set up and start BLE advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); // Get the advertising object
  pAdvertising->addServiceUUID(SERVICE_UUID); // Advertise the service UUID
  pAdvertising->setScanResponse(true); // Allow scan responses
  pAdvertising->setMinPreferred(0x06); // Set minimum preferred advertising interval
  pAdvertising->setMinPreferred(0x12); // Set maximum preferred advertising interval (possible typo, should be setMaxPreferred)
  BLEDevice::startAdvertising(); // Start advertising

  Serial.println("Characteristic defined! Now you can read it on your phone!"); // Debug message indicating setup completion

  UART.setSerialPort(&Serial); // Associate the VescUart instance with the serial port for communication
}

void loop() {
  float x = 0; // Placeholder value for updating VESC data, not used effectively in this snippet

  // Placeholder for updating UART data, values are not being updated from actual readings
  UART.data.rpm = x;
  UART.data.wattHours = x;
  UART.data.inpVoltage = x;
  UART.data.tachometerAbs = x;

  // Attempt to read VESC values and update BLE characteristics if successful
  if (UART.getVescValues()) {
    pBattPct->setValue((uint8_t *)&UART.data.ampHours, 4); // Set battery percentage from VESC data
    pBattWh->setValue((uint8_t *)&UART.data.wattHours, 4); // Set battery watt-hours from VESC data
    pRpm->setValue((uint8_t *)&UART.data.rpm, 4); // Set RPM from VESC data
    pPrdRange->setValue((uint8_t *)&UART.data.tachometerAbs, 4); // Set predicted range from VESC data

    // Notify connected BLE clients of the updated values
    pBattPct->notify();
    pBattWh->notify();
    pRpm->notify();
    pPrdRange->notify();
  } else {
    Serial.println("Failed to get data!"); // Debug message if reading VESC values fails
  }

  delay(50); // Short delay to throttle loop execution speed
}
