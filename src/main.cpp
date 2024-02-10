#include <Arduino.h>
#include <VescUart.h>

VescUart UART;

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    
  }

  UART.setSerialPort(&Serial);
}

float x = 0;

void loop() {
  x += 0.01;
  auto k = sin(x) * 6000;
  UART.setRPM(k);
  Serial.println(k);
  // Serial.print("hi");
  /** Call the function getVescValues() to acquire data from VESC */
  if ( UART.getVescValues() ) {

    Serial.println(UART.data.rpm);
    Serial.println(UART.data.inpVoltage);
    Serial.println(UART.data.ampHours);
    Serial.println(UART.data.tachometerAbs);

  }
  else
  {
    Serial.println("Failed to get data!");
  }

  delay(50);
}