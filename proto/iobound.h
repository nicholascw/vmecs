#ifndef _PROTO_IOBOUND_H_
#define _PROTO_IOBOUND_H_

/*

a general packet consists of
1. target = ((ipv4 | ipv6 | domain), port)
2. cmd = udp | tcp
3. source socket
4. data

1. an inbound receives a packet from the client and sends it to the outbound,
   and receives packet from the outbound and send back
2. a outbound works similarly

inbound of protocol P:
    1. receives enc(P, packet)
    2. decode and get packet
    3. send packet to outbound

*/

#endif
