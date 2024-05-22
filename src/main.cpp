// Include necessary libraries for BLE and VESC communication
#include <Arduino.h> // Core Arduino library for basic programming
#include <BLEDevice.h> // BLE library for creating BLE server and characteristics
#include <BLEServer.h> // Specific BLE server functionalities
#include <BLEUtils.h> // Utilities for BLE operations
#include <VescUart.h> // Library to interface with VESC over UART
#include <FastLED.h>

#define NUM_LEDS 25 // Change this to the number of LEDs in your strip
#define LED_PIN 18   // Choose the GPIO pin connected to the LED strip data pin

CRGB leds[NUM_LEDS];

// #include <ArduinoSTL.h>
// #include <deque>

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
#define WINDOW_TIME 30 // seconds
#define WINDOW_SIZE 20 // samples

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

struct distanceData{
  float distance;
  float time;
};

distanceData speedQueue[WINDOW_SIZE];
int l=0, r=-1, cnt=0;

void setup() {
  FastLED.addLeds<WS2813, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.show();
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

bool fetchVescData(){
  // Serial.print("R");
  return UART.getVescValues();
}

float bootupPct = -1;

int t = 0;
float currentSpeed = -1;


float getBatteryPercentage() {
  t++;
  currentSpeed = (sin(t*2*PI/360.0) * 25/2.0 + 25/2.0);
  return (sin(t*2*PI/360.0) * 50 + 50);
  // return ((UART.data.inpVoltage - BAT_MIN_VOLTAGE) / (BAT_MAX_VOLTAGE - BAT_MIN_VOLTAGE)) * 100;
}

float getTrip() {
  float tach = (UART.data.tachometerAbs) / (POLES * 3.0);

  // We do not need to pay attention to the translation because it is 1 to 1.
  float distance = tach * PI * WHEEL_DIAMETER;
  return distance;
}

void processSpeedQueue(){
  auto dist = getTrip();
  auto time = millis() / 1000.0f;
  r = (r + 1) % WINDOW_SIZE;
  speedQueue[r] = {dist, time};
  cnt++;

  while(speedQueue[r].time - speedQueue[l].time > WINDOW_TIME || cnt > WINDOW_SIZE){
    l = (l + 1) % WINDOW_SIZE;
    cnt--;
  }

  float distance = speedQueue[r].distance - speedQueue[l].distance;
  float dtime = speedQueue[r].time - speedQueue[l].time;
  state.speed = distance / dtime;
  currentSpeed = state.speed;
}

void updateState(){
  state.batteryPct = getBatteryPercentage();
  if(fetchVescData()){
    
    if(bootupPct == -1){
      bootupPct = state.batteryPct;
    }
    state.distanceTraveled = getTrip();

    auto pctDelta = bootupPct - state.batteryPct;
    pctDelta = max(0.01f, pctDelta);
    auto distPpct = state.distanceTraveled / pctDelta;
    state.distanceRemaining = distPpct * state.batteryPct;
    processSpeedQueue();
    state.speed = currentSpeed;
  }
}

void updateLED(){
  int ledIndex = (state.batteryPct / 100) * NUM_LEDS;
  for(int i=0; i<NUM_LEDS; i++){
    if(i < ledIndex){
      CRGB color;
      float range = 80;

      color.setHSV(123 + range * max(min(sin((((float)i) / NUM_LEDS * 5 + millis() / 300.0f)),0.7f),-0.7f), 255, 255);
      leds[i] = color;
    }else{
      leds[i] = CRGB::Red;
    }
  }
  FastLED.show();
}

void loop() {
  updateState();
  pSpeed->setValue(currentSpeed);
  pBattPct->setValue(state.batteryPct);
  pDistRem->setValue(state.distanceRemaining);
  // float v = getBatteryPercentageBootup();
  // pDistRem->setValue(v);

  pDistTr->setValue(state.distanceTraveled);

  // float f1 = UART.data.tachometer * 1000;
  // float f2 = UART.data.tachometerAbs * 1000;
  // pDistRem->setValue(f1);
  // pDistTr->setValue(f2);

  pSpeed->notify();
  pBattPct->notify();
  pDistRem->notify();
  pDistTr->notify();

  updateLED();

  delay(50); // Short delay to throttle loop execution speed
}
