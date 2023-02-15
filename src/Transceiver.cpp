#include "Transceiver.h"
#include <RF24.h>
#include <Arduino.h>
#include <math.h>
#include <ArduinoJson.h>

Transceiver::Transceiver(bool primary, short ce_pin, short csn_pin, byte address[6]) : m_radio(ce_pin, csn_pin),
                                                                                       m_connected(false),
                                                                                       m_last_health_check(millis()),
                                                                                       m_health_check_delay(1000),
                                                                                       m_radio_listening(false),
                                                                                       m_received_packet(),
                                                                                       m_last_packet_id(0),
                                                                                       m_id_counter(0)
{
    Serial.begin(9600);
    Serial.println("Starting radio");
    this->primary_transmitter = primary;
    this->m_radio.begin();
    this->m_radio.openReadingPipe(0, address);
    this->m_radio.openWritingPipe(address);
    this->m_radio.enableDynamicAck();
    this->m_radio.setPALevel(RF24_PA_MAX);
    this->m_radio.setDataRate(RF24_250KBPS);
    this->m_radio.setAutoAck(true);
    this->m_radio.setRetries(0, 15);
    this->m_radio.stopListening();

    if (this->primary_transmitter)
    {
        this->connect();
    }
    Serial.println("Startup complete");
};

void Transceiver::connect()
{
    Data data;
    data.key = 0;
    data.value = 1;
    this->send(data);
}

void Transceiver::set_connected()
{
    if (!this->m_connected)
    {
        this->m_connected = true;
        StaticJsonDocument<200> doc;
        doc["0"] = 1;
        serializeJson(doc, Serial);
    }
    this->m_last_health_check = millis();
}

void Transceiver::set_disconnected()
{
    if (this->m_connected)
    {
        this->m_connected = false;
        StaticJsonDocument<200> doc;
        doc["0"] = 0;
        serializeJson(doc, Serial);
    }
}

void Transceiver::start_radio_listening()
{
    if (this->m_radio_listening == false)
    {
        this->m_radio.startListening();
        this->m_radio_listening = true;
    }
}

void Transceiver::stop_radio_listening()
{
    if (this->m_radio_listening == true)
    {
        this->m_radio.stopListening();
        this->m_radio_listening = false;
    }
}

// Sends telemetry error warning that the it has recivied no telemetry to send from serial but its still connected (if the message gets through)
bool Transceiver::send_telemetry_error()
{
    this->stop_radio_listening();
    Packet packet;
    packet.id = this->incrementId();
    packet.data[0].key = 100;
    packet.data[0].value = 1;
    packet.num_data_fields = 1;
    bool acknowledged = this->m_radio.write(&packet, sizeof(packet), 1);
    if (acknowledged == true)
    {
        this->set_connected();
    }
    else
    {
        this->set_disconnected();
    }
    return acknowledged;
}

void Transceiver::monitor_connection_health()
{
    if (this->m_last_health_check + this->m_health_check_delay < millis())
    {
        if (this->primary_transmitter)
        {
            this->send_telemetry_error();
        }
        else
        {
            this->set_disconnected();
        }
    }
}

void Transceiver::tick()
{
    this->receive();
    this->monitor_connection_health();
}

void Transceiver::receive()
{
    this->start_radio_listening();
    if (this->m_radio.available())
    {
        this->m_radio.read(&this->m_received_packet, sizeof(this->m_received_packet));
        if (this->m_last_packet_id != this->m_received_packet.id)
        {
            this->write_data_to_serial();
            this->m_last_packet_id = this->m_received_packet.id;
        }
        this->set_connected();
    }
}

void Transceiver::write_data_to_serial()
{
    DynamicJsonDocument doc(sizeof(this->m_received_packet.data));
    for (short i = 0; i < this->m_received_packet.num_data_fields; i++)
    {
        doc[this->m_received_packet.data[i].key] = this->m_received_packet.data[i].value;
    }
    serializeJson(doc, Serial);
}

bool Transceiver::send(Data data)
{
    this->stop_radio_listening();

    Packet packet;
    packet.data[0] = data;

    packet.id = this->incrementId();
    packet.num_data_fields = 1;

    bool acknowledged = this->m_radio.write(&data, sizeof(data), 1);

    if (acknowledged)
    {
        this->set_connected();
    }
    else
    {
        // add to buffer
    };
    this->start_radio_listening();
    return acknowledged;
}

bool Transceiver::send(Data data[7], int size)
{
    this->stop_radio_listening();

    Packet packet;
    for (short i = 0; i < size; i++)
    {
        packet.data[i] = data[i];
    }
    packet.id = this->incrementId();
    packet.num_data_fields = size;

    bool acknowledged = this->m_radio.write(&data, sizeof(data), 1);

    if (acknowledged)
    {
        this->set_connected();
    }
    else
    {
        // add to buffer
    };
    return acknowledged;
}

bool Transceiver::sendLarge(Data *data, int size)
{
    this->stop_radio_listening();
    return this->send_split_payload(data, size);
}

bool Transceiver::send_split_payload(Data *data, int size)
{
    bool all_packets_sent = true;
    const int max_size = 7;
    int num_packets = ceil(size / max_size);

    int index = 0;
    for (int i = 0; i < num_packets; i++)
    {
        Packet packet;
        packet.id = this->incrementId();
        unsigned char packet_size = 0;
        for (int j = 0; j < max_size && index < size; j++)
        {
            packet.data[i] = data[index++];
            packet_size = j;
        }
        packet.num_data_fields = packet_size;

        if (this->m_radio.write(&packet, sizeof(packet), 1))
        {
            this->set_connected();
        }
        else
        {
            // add to buffer
            all_packets_sent = false;
        };
    }
    return all_packets_sent;
}

short Transceiver::incrementId()
{
    this->m_id_counter++;
    if (m_id_counter > 255)
    {
        m_id_counter = 1;
    }
    return m_id_counter;
}