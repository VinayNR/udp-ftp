### UDP-FTP - UDP-based FTP Protocol

This project implements a Best-Effort File Transfer Protocol on top of UDP (User Datagram Protocol). It consists of two main components: the client and the server. Due to the connectionless nature of UDP, there is no explicit handshake between them.

#### Components

- `client.cpp` - Runs the client software.
- `server.cpp` - Runs the server software.

#### Allowed Commands

The client can send the following commands to the server:

- `ls` - List the current files and directories in the server's current directory.
- `help` - Display a list of allowed commands for the client.
- `get <filename>` - Download a file from the remote server to the local directory on the client side.
- `put <filename>` - Upload a file from the client's local directory to the server's remote directory.
- `delete <filename>` - Delete a file on the server's remote directory.
- `exit` - Exit the server.

#### Components

- `commands.cpp` - Contains the list of allowed commands that the server accepts.
- `file_ops.cpp` - Includes primitive read, write, and delete file operations using the filestream C library.
- `messages.cpp` - Defines the structures of a UDP Message and UDP Packet and contains common routines for serialization and deserialization.
- `udp.cpp` - Provides basic send and receive primitives for UDP packets over the network.
