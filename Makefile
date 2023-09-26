CC = g++  # Compiler
CFLAGS = -Wall  # Compiler flags

# Source files for the client and server
CLIENT_SRC = client.cpp commands.cpp file_ops.cpp message.cpp udp.cpp
SERVER_SRC = server.cpp commands.cpp file_ops.cpp message.cpp udp.cpp

# Output binary names
CLIENT_OUT = client
SERVER_OUT = server

all: $(CLIENT_OUT) $(SERVER_OUT)

# Compile the client program
$(CLIENT_OUT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_OUT) $(CLIENT_SRC)

# Compile the server program
$(SERVER_OUT): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_OUT) $(SERVER_SRC)

clean:
	rm -f $(CLIENT_OUT) $(SERVER_OUT)

.PHONY: all clean

client: $(CLIENT_OUT)

server: $(SERVER_OUT)
