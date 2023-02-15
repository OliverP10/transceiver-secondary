#include <Arduino.h>
#include "Transceiver.h"

byte address[6] = {'R', 'x', 'A', 'A', 'A'};
Transceiver transceiver = Transceiver(true, 9, 8, address);

void setup()
{
  Serial.println("Starting");
}

void loop()
{
  transceiver.tick();
}