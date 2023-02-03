#include <RF24.h>

struct Data
{
    short key;
    short value;
};

struct Packet
{
    Data data[7];
    bool first;
    char num_data_fields;
} __attribute__((packed));

struct Packets
{
    Data **packets;
    short num_data_fields;
} __attribute__((packed));

class Transceiver
{
private:
    RF24 m_radio;
    bool m_connected;
    unsigned long m_last_health_check;
    unsigned short m_health_check_delay;
    unsigned long m_last_packet_received;
    unsigned short m_max_packet_wait_time;
    bool m_radio_listening;
    bool m_reciving;
    Packet m_packet;
    short m_number_packets_recived;
    short m_number_data_fields;
    short m_number_packets;
    Data *m_pRecived_data;

public:
    Transceiver(short ce_pin, short csn_pin, byte address[6]);
    void tick();
    bool send(Data *data, int size);

private:
    void connect();
    void set_connected();
    void set_disconnected();
    void start_radio_listening();
    void stop_radio_listening();
    void monitor_connection_health();
    bool send_handshake();
    bool request_to_send(int &num_packets);
    void receive();
    void load_packet();
    void prepare_new_packet();
    void flush_recived_packets();
    Packets split_payload(Data *data, int size);
    void write_data_to_serial();

public:
};