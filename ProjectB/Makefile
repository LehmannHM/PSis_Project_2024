# Compiler settings
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lzmq -lncurses -lprotobuf-c

# Source files with path
COMMON_SRC = common.c
SERVER_SRC = game_server/game-server.c game_server/score_update.pb-c.c $(COMMON_SRC)
CLIENT_SRC = astronaut_client/astronaut-client.c $(COMMON_SRC)
CLIENT_DISPLAY_SRC = astronaut_display_client/astronaut-display-client.c $(COMMON_SRC)
DISPLAY_SRC = outer_space_display/outer-space-display.c $(COMMON_SRC)

# Executable names
SERVER_EXE = game_server/game-server
CLIENT_EXE = astronaut_client/astronaut-client
CLIENT_DISPLAY_EXE = astronaut_display_client/astronaut-display-client
DISPLAY_EXE = outer_space_display/outer-space-display

# Default target to build all executables
all: $(SERVER_EXE) $(CLIENT_EXE) $(DISPLAY_EXE) $(CLIENT_DISPLAY_EXE)

# Build server executable
$(SERVER_EXE): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build client executable
$(CLIENT_EXE): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build client display executable
$(CLIENT_DISPLAY_EXE): $(CLIENT_DISPLAY_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build display executable
$(DISPLAY_EXE): $(DISPLAY_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Clean target to remove compiled files
clean:
	rm -f $(SERVER_EXE) $(CLIENT_EXE) $(CLIENT_DISPLAY_EXE) $(DISPLAY_EXE)