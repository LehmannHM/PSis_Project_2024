# game-server: game-server.c
# 	gcc game-server.c -o game-server -lzmq -lncurses -lm

# astronaut-client: astronaut-client.c
# 	gcc astronaut-client.c -o astronaut-client -lzmq -lncurses -lm

# outer-space-display: outer-space-display.c
# 	gcc outer-space-display.c -o outer-space-display -lzmq -lncurses -lm

# Compiler settings
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lzmq -lncurses

# Source files with path
SERVER_SRC = ./game-server.c
CLIENT_SRC = ./astronaut-client.c
DISPLAY_SRC = ./outer-space-display.c

# Executable names
SERVER_EXE = ./game-server
CLIENT_EXE = ./astronaut-client
DISPLAY_EXE = ./outer-space-display

# Default target to build all executables
all: $(SERVER_EXE) $(CLIENT_EXE) $(DISPLAY_EXE)

# Build server executable
$(SERVER_EXE): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Build client executable
$(CLIENT_EXE): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Build display executable
$(DISPLAY_EXE): $(DISPLAY_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Clean target to remove compiled files
clean:
	rm -f $(SERVER_EX)