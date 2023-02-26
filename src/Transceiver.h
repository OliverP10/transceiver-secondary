#pragma once
#include <RF24.h>
#include <CircularBuffer.h>

struct Data
{
    unsigned short key;
    unsigned short value;
};

struct Packet
{
    unsigned char id;
    unsigned char num_data_fields;
    Data data[7];
} __attribute__((packed));

struct Buffered_packet
{
    Packet packet;
    int created_time;
};

class TransceiverSecondary
{
private:
    RF24 m_radio;
    bool m_connected;
    unsigned long m_last_health_check;
    unsigned short m_health_check_delay;
    bool m_radio_listening;
    float m_backoff_time;
    unsigned long m_last_backoff;
    CircularBuffer<Buffered_packet, 10> *m_buffer;
    Packet m_received_packet;
    unsigned char m_last_packet_id;
    short m_id_counter;

public:
    TransceiverSecondary(short ce_pin, short csn_pin);
    ~TransceiverSecondary();
    TransceiverSecondary(const TransceiverSecondary &other) = delete;
    void tick();
    bool send(Data data, bool buffer = true);
    bool send(Data data[7], int size, bool buffer = true);
    bool sendLarge(Data *data, int size, bool buffer = true);
    void setup(byte address[6]);
    void debug();

private:
    void set_connected();
    void set_disconnected();
    void start_radio_listening();
    void stop_radio_listening();
    void monitor_connection_health();
    void receive();
    bool send_buffered_packet(Packet packet);
    void write_data_to_serial();
    void write_connection_status_to_serial(bool connected);
    void add_to_buffer(Packet Packet);
    void clear_buffer();
    void reset_backoff();
    void increase_backoff();
    unsigned char incrementId();
};