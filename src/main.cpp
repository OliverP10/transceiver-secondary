#include <Arduino.h>

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// create an RF24 object
RF24 radio(9, 8); // CE, CSN

// address through which two modules communicate.
const byte address[6] = "12345";

unsigned long counter = 0;
unsigned long current_time = 0;
unsigned long packetStartCount = 0;

void setup()
{
  while (!Serial)
    ;
  Serial.begin(9600);

  radio.begin();

  // set the address
  radio.openReadingPipe(0, address);
  radio.setChannel(125);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(true);
  radio.setDataRate(RF24_250KBPS); // RF24_250KBPS RF24_2MBPS
  radio.setRetries(1, 15);
  // Set module as receiver
  radio.startListening();
}

void loop()
{
  // Read the data if available in buffer
  if (radio.available())
  {
    counter++;
    char text[32] = {0};
    unsigned long packetCount;
    radio.read(&packetCount, sizeof(unsigned long));
    // Serial.println(packetCounter);

    if (current_time + 1000 <= millis())
    {
      Serial.println("Speed: " + String((counter * 32) / 8) + " Bps,  " + String(counter) + "pps,  " + String(packetCount - (packetStartCount + counter)) + " droped packets");

      counter = 0;
      current_time = millis();
      packetStartCount = packetCount;
    }
  }
}