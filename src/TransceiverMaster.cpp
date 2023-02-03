#pragma once
#include "TransceiverMaster.h"
#include <RF24.h>
#include <Arduino.h>
#include <math.h>
#include <ArduinoJson.h>

Transceiver::Transceiver(short ce_pin, short csn_pin, byte address[6]) : m_radio(ce_pin, csn_pin),
                                                                         m_connected(false),
                                                                         m_last_health_check(millis()),
                                                                         m_health_check_delay(1000),
                                                                         m_last_packet_received(millis()),
                                                                         m_max_packet_wait_time(5),
                                                                         m_radio_listening(false),
                                                                         m_reciving(false),
                                                                         m_packet(),
                                                                         m_number_packets_recived(0),
                                                                         m_number_data_fields(0),
                                                                         m_number_packets(0),
                                                                         m_pRecived_data(nullptr)

{
    Serial.begin(9600);
    this->m_radio.begin();
    this->m_radio.openReadingPipe(0, address); // Might need to change
    this->m_radio.openWritingPipe(address);
    this->m_radio.enableDynamicAck();
    this->m_radio.setPALevel(RF24_PA_MAX); // Set power amplifier level to maximum
    this->m_radio.setDataRate(RF24_250KBPS);
    this->m_radio.setAutoAck(true);
    this->m_radio.setRetries(0, 15);
    this->m_radio.stopListening();

    this->connect();
};

void Transceiver::connect()
{

    while (this->m_connected == false)
    {
        this->send_handshake();
        delay(100);
    }
}

void Transceiver::set_connected()
{
    this->m_connected = true;
}

void Transceiver::set_disconnected()
{
    this->m_connected = false;
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

// redo can just send stats on radio or just have it send telem every so often
bool Transceiver::send_handshake()
{
    this->stop_radio_listening();
    const char hand_shake = '1';
    bool acknowledged = this->m_radio.write(&hand_shake, sizeof(hand_shake), 1);
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
        this->send_handshake();
        this->m_last_health_check = millis();
    }
}

void Transceiver::tick()
{
    this->monitor_connection_health();
    this->flush_recived_packets();
}

bool Transceiver::request_to_send(int &num_data_fields)
{
    this->stop_radio_listening();
    RequestToSendPacket packet;
    packet.request_type = 'W';
    packet.num_data_fields = num_data_fields;

    const int max_request_time = 25;
    int start_time = millis();

    while (start_time + max_request_time > millis())
    {
        if (this->m_radio.write(&packet, sizeof(packet), 1))
        {
            return true;
        }
        delay(random(0, 6));
    }
    return false;
}

// Add feature so if the last packet is not recived within time frame it writes the others to serial
void Transceiver::receive()
{
    this->start_radio_listening();
    if (this->m_radio.available())
    {
        this->m_radio.read(&this->m_packet, sizeof(this->m_packet));

        if (!this->m_reciving)
        { // New packet group
            this->prepare_new_packet();
            this->load_packet();
        }
        else if (this->m_reciving && !this->m_packet.first)
        { // Appeding to packet group
            this->load_packet();
        }
        else if (this->m_reciving && this->m_packet.first)
        { // flush and start new packet group
            this->write_data_to_serial();
            this->prepare_new_packet();
            this->load_packet();
        }
    }
}

void Transceiver::load_packet()
{
    short packet_size = (this->m_number_packets_recived == this->m_number_packets - 1 && this->m_number_data_fields != 7) ? this->m_number_data_fields % 7 : 7;
    short start = this->m_number_packets_recived * 7;
    for (short i = 0; i < packet_size; i++)
    {
        this->m_pRecived_data[start + i] = this->m_packet.data[i];
    }
    this->m_number_packets_recived++;
    if (this->m_number_packets_recived == this->m_number_packets)
    {
        this->write_data_to_serial();
        this->m_reciving = false;
    }
}

void Transceiver::prepare_new_packet()
{
    this->m_reciving = true;
    this->m_number_packets_recived = 0;
    this->m_number_data_fields = this->m_packet.num_data_fields;
    this->m_number_packets = ceil(this->m_number_data_fields / 7);
    if (this->m_pRecived_data != nullptr)
    {
        delete[] this->m_pRecived_data;
    }
    this->m_pRecived_data = new Data[this->m_number_data_fields];
}

void Transceiver::flush_recived_packets()
{
    if (this->m_reciving && this->m_last_packet_received + this->m_max_packet_wait_time < millis())
    {
        this->write_data_to_serial();
        this->prepare_new_packet();
    };
}

bool Transceiver::send(Data *data, int size)
{
    this->start_radio_listening();
    Packets packets = this->split_payload(data, size);

    for (int i = 0; i < packets.num_data_fields; i++)
    {
        bool acknowledged = this->m_radio.write(&packets.packets[i], sizeof(packets.packets[i]), 1);
        if (acknowledged == false)
        {
        };
    }
}

Packets Transceiver::split_payload(Data *data, int size)
{
    const int max_size = 7;
    int num_packets = ceil(size / max_size);
    Data **packets = new Data *[num_packets];

    int index = 0;
    for (int i = 0; i < num_packets; i++)
    {
        if (i == num_packets - 1 && size != max_size)
        {
            packets[i] = new Data[size % max_size];
        }
        else
        {
            packets[i] = new Data[max_size];
        }

        for (int j = 0; j < max_size && index < size; j++)
        {
            packets[i][j] = data[index++];
        }
    }
    return {packets, num_packets};
}

void Transceiver::write_data_to_serial()
{
    DynamicJsonDocument doc(sizeof(*this->m_pRecived_data));
    short last_recived_packet_size = (this->m_number_packets_recived == this->m_number_packets - 1 && this->m_number_data_fields != 7) ? this->m_number_data_fields % 7 : 7;
    for (short i = 0; i < ((this->m_number_packets_recived - 1) * 7) + last_recived_packet_size; i++)
    {
        doc[this->m_pRecived_data[i].key] = this->m_pRecived_data[i].value;
    }
    serializeJson(doc, Serial);
}