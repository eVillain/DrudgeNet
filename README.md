DrudgeNet: A C++ network library for communicating over UDP.
This library is designed with real-time games in mind, with a focus on physics
simulation synchronization over a network as well as simple implementations of
standard game networking methods.

Supported platforms are Win/Mac/Linux for now, possibly mobile and/or consoles
after the desktop version of the library is feature-complete.

    --- Current feature set ---
Sockets:
  Address - IPv4 address for socket connections. IPv6 is not (yet) supported.
  Socket - Cross-platform UDP Socket class, connects to an Address.
Connection:
  Connection - Simple server-client connection using the Socket.
  ReliableConnection - P2P connection using ReliabilitySystem.
Reliability:
  PacketQueue - Stores information about sent and received packets sorted in sequence order.
  ReliabilitySystem - Manages sent, received, pending ack and acked PacketQueues.
  FlowControl - Provides simple binary flow control.
Matchmaking:
  Beacon - Sends broadcast UDP packets to the LAN to advertise a server.
  Listener - Listens for broadcast packets sent over the LAN to find all servers.
NodeMesh:
  Mesh - Manages a network of Nodes.
  Node - Client node in a Mesh, gets info about all other nodes from the Mesh.
DataStream:
  BitPacker - Reads and writes non-8 multiples of bits efficiently.
  Stream - Unifies read and write into a serialize operation.
Transport Layer:
  Transport - Abstract network transport interface.
  TransportLAN - LAN transport implementation.
    Servers are advertised with a Beacon.
    LAN lobby is filled via Listener.
    A Mesh runs on the server IP and manages Node connections.
    A Node runs on each Transport and a local Node also runs on the server with the Mesh.

      --- TODO List ---
IPv6 support
(see http://long.ccaba.upc.edu/long/045Guidelines/eva/ipv6.html)
Internet Transport
Data Compression
(see http://gafferongames.com/2015/03/14/the-networked-physics-data-compression-challenge/
and https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6)
NAT punchthrough
Chipmunk2D integration layer
Bullet Physics integration layer


This library is heavily influenced by Glenn Fiedler's Simple Network Library
from the "Networking for Game Programmers" series of articles:
http://www.gafferongames.com/networking-for-game-programmers
A fork of the original code can be found here:
https://github.com/ThisIsRobokitty/netgame

All the code is public domain.
