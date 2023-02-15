#pragma once
#include <RF24.h>

struct Data
{
    short key;
    short value;
};

struct Packet
{
    unsigned char id;
    unsigned char num_data_fields;
    Data data[7];
} __attribute__((packed));

class Transceiver
{
private:
    bool primary_transmitter;
    RF24 m_radio;
    bool m_connected;
    unsigned long m_last_health_check;
    unsigned short m_health_check_delay;
    bool m_radio_listening;
    Packet m_received_packet;
    unsigned char m_last_packet_id;
    short m_id_counter;

public:
    Transceiver(bool primary_transmitter, short ce_pin, short csn_pin, byte address[6]);
    void tick();
    bool send(Data data);
    bool send(Data data[7], int size);
    bool sendLarge(Data *data, int size);

private:
    void connect();
    void set_connected();
    void set_disconnected();
    void start_radio_listening();
    void stop_radio_listening();
    void monitor_connection_health();
    bool send_telemetry_error();
    void receive();
    bool send_split_payload(Data *data, int size);
    void write_data_to_serial();
    short incrementId();
};