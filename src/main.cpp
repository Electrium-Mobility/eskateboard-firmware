#include <Arduino.h>
#include <VescUart.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "82f736e2-0115-4a37-9498-f357806b45d8"
#define BATTERY_PCT_UUID "b5fa25e0-884e-475a-9a70-a286b88df9f5"
#define BATTERY_WH_UUID "ec76c264-0bc4-4eaa-b32e-01c723e9cfe3"
#define RPM_UUID "abc4af9e-7220-45e5-bc87-137d35dafb4d"
#define PRD_RANGE_UUID "6043959f-c355-40c3-a069-9e511f335793"

VescUart UART;

BLECharacteristic *pBattPct, *pBattWh, *pRpm, *pPrdRange;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEDevice::init("Electrium e-skateboard");  // set the device name
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pBattPct = pService->createCharacteristic(
    BATTERY_PCT_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  pBattWh = pService->createCharacteristic(
    BATTERY_WH_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  pRpm = pService->createCharacteristic(
    RPM_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  pPrdRange = pService->createCharacteristic(
    PRD_RANGE_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
  UART.setSerialPort(&Serial);
}

float x = 0;

void loop() {

  UART.data.rpm = x;
  UART.data.wattHours = x;
  UART.data.inpVoltage = x;
  UART.data.tachometerAbs = x;
  // put your main code here, to run repeatedly:
  if ( UART.getVescValues() ) {
    pBattPct->setValue((uint8_t*)&UART.data.ampHours, 4);
    pBattWh->setValue((uint8_t*)&UART.data.wattHours, 4);
    pRpm->setValue((uint8_t*)&UART.data.rpm, 4);
    pPrdRange->setValue((uint8_t*)&UART.data.tachometerAbs, 4);
    pBattPct->notify();
    pBattWh->notify();
    pRpm->notify();
    pPrdRange->notify();
  }
  else
  {
    Serial.println("Failed to get data!");
  }
  delay(50);
}
