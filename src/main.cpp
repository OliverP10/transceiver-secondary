#include <Arduino.h>
#include "Transceiver.h"

byte address[6] = {'R', 'x', 'A', 'A', 'A'};
TransceiverSecondary transceiver(9, 8);

unsigned long last_run_print = millis();
unsigned int run_delay_print = 1000;

unsigned long last_run_send = millis();
unsigned int run_delay_send = 1000;

void setup()
{
  Serial.begin(115200);
  transceiver.setup(address);
}

void loop()
{
  if (last_run_print + run_delay_print < millis())
  {
    last_run_print = millis();
    transceiver.debug();
  }

  if (last_run_send + run_delay_send < millis())
  {
    last_run_send = millis();

    Data data1;
    data1.key = 2;
    data1.value = random(0, 9999);

    Data data2;
    data2.key = 2;
    data2.value = random(0, 9999);

    Data data3;
    data3.key = 2;
    data3.value = random(0, 9999);

    Data data4;
    data4.key = 2;
    data4.value = random(0, 9999);

    Data data5;
    data5.key = 2;
    data5.value = random(0, 9999);

    Data data6;
    data6.key = 2;
    data6.value = random(0, 9999);

    Data data7;
    data7.key = 2;
    data7.value = random(0, 9999);

    Data packet_data[7];
    packet_data[0] = data1;
    packet_data[1] = data2;
    packet_data[2] = data3;
    packet_data[3] = data4;
    packet_data[4] = data5;
    packet_data[5] = data6;
    packet_data[6] = data7;

    transceiver.load(packet_data, 7);
  }

  transceiver.tick();
}