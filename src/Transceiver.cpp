#pragma once
#include "Transceiver.h"
#include <Arduino.h>
#include <RF24.h>
#include <math.h>
#include <ArduinoJson.h>
#include <CircularBuffer.h>

// Secondary TRANSMITTER

int packets_sent = 0;
int packets_lost = 0;
int number_backoffs = 0;

TransceiverSecondary::TransceiverSecondary(int ce_pin, int csn_pin) : m_radio(ce_pin, csn_pin),
                                                                      m_connected(false),
                                                                      m_last_health_check(millis()),
                                                                      m_health_check_delay(1000),
                                                                      m_backoff_time(1),
                                                                      m_last_backoff(0),
                                                                      m_awaiting_acknoledge(true),
                                                                      m_buffer(new CircularBuffer<Buffered_packet, 20>()),
                                                                      m_send_packet(),
                                                                      m_received_packet(),
                                                                      m_last_packet_id(0),
                                                                      m_id_counter(0)
{
    this->m_send_packet.num_data_fields = 0;
};

TransceiverSecondary::~TransceiverSecondary()
{
    this->m_buffer->clear();
    delete this->m_buffer;
}

void TransceiverSecondary::setup(byte address[6])
{
    this->m_radio.begin();
    this->m_radio.openReadingPipe(0, address);
    this->m_radio.openWritingPipe(address);
    this->m_radio.enableDynamicAck();
    this->m_radio.setPALevel(RF24_PA_MIN); // RF24_PA_MAX
    this->m_radio.setDataRate(RF24_2MBPS); // RF24_250KBPS
    this->m_radio.setAutoAck(true);
    this->m_radio.setRetries(5, 15);
    this->m_radio.startListening();
}

void TransceiverSecondary::receive()
{
    if (this->m_radio.available())
    {
        this->m_radio.read(&this->m_received_packet, sizeof(this->m_received_packet));
        if (this->m_last_packet_id != this->m_received_packet.id && this->m_received_packet.num_data_fields != 0)
        {
            this->m_last_packet_id = this->m_received_packet.id;
            if (this->m_received_packet.data->key == 1)
            {
                this->m_awaiting_acknoledge = this->m_received_packet.data[0].value;
                this->write_data_to_serial();
                return;
            }
            this->write_data_to_serial();
        }
        this->m_awaiting_acknoledge = false;
        this->set_connected();
    }
}

void TransceiverSecondary::monitor_connection_health()
{
    if (this->m_last_health_check + this->m_health_check_delay < millis())
    {
        this->set_disconnected();
    }
}

void TransceiverSecondary::debug()
{
    // Serial.println();
    // Serial.print("Packets sent: ");
    // Serial.println(packets_sent);
    // packets_sent = 0;
    // Serial.print("Packets lost: ");
    // Serial.println(packets_lost);
    // Serial.print("Number of backoffs: ");
    // Serial.println(number_backoffs);
    // number_backoffs = 0;
    // packets_lost = 0;
    // Serial.print("Buffer available: ");
    // Serial.println(this->m_buffer->available());
    // Serial.print("Connected: ");
    // Serial.println(this->m_connected);
    // Serial.print("Awaiting acknoledge: ");
    // Serial.println(this->m_awaiting_acknoledge);
    // Serial.print("Backoff time: ");
    // Serial.println(this->m_backoff_time);
    // Serial.print("Free memory: ");
    // Serial.println(freeMemory());
    // Serial.print("Last health check: ");
    // Serial.println(this->m_last_health_check);
    // Serial.print("Health check delay: ");
    // Serial.println(this->m_health_check_delay);
    // Serial.print("Last backoff: ");
    // Serial.println(this->m_last_backoff);
    // Serial.print("Last packet id: ");
    // Serial.println(this->m_last_packet_id);
    // Serial.print("id counter: ");
    // Serial.println(this->m_id_counter);
}

void TransceiverSecondary::set_connected()
{
    if (!this->m_connected)
    {
        this->m_connected = true;
        this->write_connection_status_to_serial(true);
    }
    this->m_last_health_check = millis();
    this->reset_backoff();
}

void TransceiverSecondary::set_disconnected()
{
    if (this->m_connected)
    {
        this->m_connected = false;
        this->write_connection_status_to_serial(false);
    }
    this->m_last_health_check = millis();
}

void TransceiverSecondary::tick()
{
    this->load_data_from_serial();
    this->receive();
    this->monitor_connection_health();
    this->send_data();
    this->clear_buffer();
}

void TransceiverSecondary::write_data_to_serial()
{
    StaticJsonDocument<150> doc;
    for (int i = 0; i < this->m_received_packet.num_data_fields; i++)
    {
        doc[String(this->m_received_packet.data[i].key)] = this->m_received_packet.data[i].value;
    }
    serializeJson(doc, Serial);
    Serial.println();
}

void TransceiverSecondary::write_connection_status_to_serial(bool connected)
{
    StaticJsonDocument<20> doc;
    doc["0"] = (connected) ? 1 : 0;
    serializeJson(doc, Serial);
    Serial.println();
}

/*
    Loads an a single struct of data ready to be sent
*/
void TransceiverSecondary::load(Data data)
{
    Data dataArray[5];
    dataArray[0] = data;
    return this->load(dataArray, 1);
}

/*
    Loads an array of data with max size of 5, ready to be sent
*/
void TransceiverSecondary::load(Data data[5], unsigned char size)
{
    Packet packet;
    packet.id = this->increment_id();
    packet.num_data_fields = size;
    for (unsigned char i = 0; i < size; i++)
    {
        packet.data[i] = data[i];
    }

    if (this->m_send_packet.num_data_fields == 0)
    {
        this->m_send_packet = packet;
    }
    else
    {
        this->add_to_buffer(packet);
    }
}

/*
    Loads an array of data with specified size. The data will be split up into max packet size of 5.
*/
void TransceiverSecondary::load_large(Data *data, unsigned char size)
{
    const int max_size = 5;
    int num_packets = ceil((double)size / (double)max_size);
    int index = 0;
    for (int i = 0; i < num_packets; i++)
    {
        Data packet_data[5];
        unsigned char packet_size = 0;
        for (int j = 0; j < max_size && index < size; j++)
        {
            packet_data[j] = data[index++];
            packet_size = j;
        }

        this->load(packet_data, packet_size + 1);
    }
}

void TransceiverSecondary::load_data_from_serial()
{
    if (Serial.available())
    {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, Serial);
        if (error)
        {
            // Serial.println(error.f_str());
            return;
        }

        // serializeJson(doc, Serial);
        // Serial.println();

        Data packet_data[doc.size()];
        unsigned char index = 0;
        for (JsonPair kvp : doc.as<JsonObject>())
        {
            const char *key = kvp.key().c_str();
            float value = kvp.value().as<float>();

            Data data;
            data.key = atoi(key);
            data.value = value;

            packet_data[index] = data;
            index++;
        }

        this->load_large(packet_data, doc.size());
    }
}

void TransceiverSecondary::send_data()
{
    if (this->m_awaiting_acknoledge || this->m_last_backoff + this->m_backoff_time > millis())
    {
        return;
    }
    this->m_radio.stopListening();
    bool acknoledged = this->m_radio.write(&this->m_send_packet, sizeof(this->m_send_packet));
    this->m_radio.startListening();

    if (acknoledged)
    {
        this->set_connected();
        packets_sent++;
        this->m_send_packet.num_data_fields = 0;
        this->m_awaiting_acknoledge = true;

        if (!this->m_buffer->isEmpty())
        {
            this->m_send_packet = this->m_buffer->pop().packet;
        }
    }
    else
    {
        this->increase_backoff();
    }
}

void TransceiverSecondary::add_to_buffer(Packet packet)
{
    Buffered_packet buffered_packet;
    buffered_packet.packet = packet;
    buffered_packet.created_time = millis();
    this->m_buffer->unshift(buffered_packet);
}

void TransceiverSecondary::clear_buffer()
{
    if (this->m_buffer->isEmpty())
    {
        return;
    }

    const unsigned int max_packet_lifetime = 1000;
    while (!this->m_buffer->isEmpty() && this->m_buffer->last().created_time + max_packet_lifetime < millis())
    {
        this->m_buffer->pop();
        packets_lost++;
    }
}

void TransceiverSecondary::reset_backoff()
{
    this->m_backoff_time = 2;
}

void TransceiverSecondary::increase_backoff()
{
    const int max_backoff = random(800, 1100);
    this->m_backoff_time = min(this->m_backoff_time * (1 + (random(0, 99) / 100.0)), max_backoff);
    number_backoffs++;
    // Serial.println("Increasing backoff to: ");
    // Serial.println(this->m_backoff_time);
}

unsigned char TransceiverSecondary::increment_id()
{
    this->m_id_counter++;
    if (m_id_counter > 254)
    {
        m_id_counter = 1;
    }
    return m_id_counter;
}