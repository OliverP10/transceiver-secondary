#include <RF24.h>

struct Data
{
    short key;
    short value;
};

struct Packet
{
    unsigned char packet_id;
    unsigned char group_id;
    unsigned char policy;
    unsigned char num_data_fields;
    Data data[7];
} __attribute__((packed));

struct Packets
{
    Packet *packets;
    short num_data_fields;
};

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
    unsigned char m_received_packet_id;
    unsigned char m_received_group_id;
    Packet m_received_packet;
    short m_received_total_packets;
    short m_received_number_packets;
    short m_received_number_data_fields;
    Data *m_pRecived_data;
    unsigned char m_sent_packet_id;
    unsigned char m_sent_group_id;

public:
    Transceiver(short ce_pin, short csn_pin, byte address[6]);
    void tick();
    bool send(Data *data, int size, unsigned char policy);

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
    Packets split_payload(Data *data, int size, unsigned char policy);
    void write_data_to_serial();
    int boundedIncrement(int number);

public:
};