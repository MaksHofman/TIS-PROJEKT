extern unsigned long last_master_message_time;

void send(byte* data, size_t size, int node_id);

size_t receive();
