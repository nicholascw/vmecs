#ifndef _PROTO_SOCKS5_H_
#define _PROTO_SOCKS5_H_

/*
    1. method negotiation
        client: [1: version] [1: # of methods] [n: supported methods]
        server: [1: version] [1: method #]
    
    2. connection
        client: [1: version] [1: cmd] [1: resv, 00] [1: addr type] [x: addr] [2: port]
        server: try to create the connection and then allocate a port for proxy
            [1: version] [1: status] [1: resv, 00] [1: addr type] [x: bind addr] [2: bind port]
 */

#endif
