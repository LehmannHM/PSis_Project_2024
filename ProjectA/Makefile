# Compiler settings
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lzmq -lncurses

# Source files with path
SERVER_SRC = game_server/game-server.c
CLIENT_SRC = astronaut_client/astronaut-client.c
DISPLAY_SRC = outer_space_display/outer-space-display.c

# Executable names
SERVER_EXE = game_server/game-server
CLIENT_EXE = astronaut_client/astronaut-client
DISPLAY_EXE = outer_space_display/outer-space-display

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