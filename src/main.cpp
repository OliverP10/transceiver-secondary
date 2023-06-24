#include <Arduino.h>
#include "Transceiver.h"

byte address[6] = {'R', 'x', 'A', 'A', 'A'};
TransceiverSecondary transceiver(9, 8);

unsigned long last_run_print = millis();
unsigned int run_delay_print = 1000;

unsigned long last_run_send = millis();
unsigned int run_delay_send = 500;

void onDataRecived(Packet packet, int size)
{
  Serial.println("Received packet:");
  for (int i = 0; i < size; i++)
  {
    Serial.print(packet.data[i].key);
    Serial.print(": ");
    Serial.println(packet.data[i].value);
  }
  Serial.println();
}

void setup()
{
  Serial.begin(57600);
  transceiver.setup(address, NULL);
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
    data1.key = 20;
    data1.value = random(0, 160);

    Data data2;
    data2.key = 21;
    data2.value = random(0, 160);

    Data data3;
    data3.key = 22;
    data3.value = random(0, 160);

    Data data4;
    data4.key = 23;
    data4.value = random(0, 160);

    Data data5;
    data5.key = 24;
    data5.value = random(80, 140);

    Data data6;
    data6.key = 25;
    data6.value = random(80, 140);

    Data data7;
    data7.key = 26;
    data7.value = random(80, 140);

    Data data8;
    data8.key = 27;
    data8.value = random(80, 140);

    Data data9;
    data9.key = 28;
    data9.value = random(0, 60);

    Data data10;
    data10.key = 29;
    data10.value = random(0, 60);

    Data data11;
    data11.key = 30;
    data11.value = random(0, 60);

    Data data12;
    data12.key = 31;
    data12.value = random(0, 60);

    Data packet_data[4];
    packet_data[0] = data1;
    packet_data[1] = data2;
    packet_data[2] = data3;
    packet_data[3] = data4;
    // packet_data[4] = data5;
    // packet_data[5] = data6;
    // packet_data[6] = data7;
    // packet_data[7] = data8;
    // packet_data[8] = data9;
    // packet_data[9] = data10;
    // packet_data[10] = data11;
    // packet_data[11] = data12;

    transceiver.load_data(packet_data, 4);
  }

  transceiver.tick();
}