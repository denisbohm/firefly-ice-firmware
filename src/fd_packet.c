#include "fd_packet.h"

fd_packet_output_t *fd_packet_output_head;

void fd_packet_initialize(void) {
    fd_packet_output_head = 0;
}

void fd_packet_output_initialize(fd_packet_output_t *packet_output, fd_lock_owner_t owner, fd_packet_is_available_t is_available, fd_packet_write_t write) {
    packet_output->owner = owner;
    packet_output->is_available = is_available;
    packet_output->write = write;
    packet_output->callback = 0;
    packet_output->subscribed_properties = 0;
    packet_output->notify_properties = 0;
    packet_output->next = fd_packet_output_head;
    fd_packet_output_head = packet_output;
}

void fd_packet_output_unavailable(fd_packet_output_t *packet_output) {
    packet_output->subscribed_properties = 0;
    packet_output->notify_properties = 0;
}
