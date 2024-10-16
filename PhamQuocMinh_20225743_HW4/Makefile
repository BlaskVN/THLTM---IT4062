CC = gcc
CFLAGS = -Wall

SERVER_SRC = server.c
SERVER_EXE = server

CLIENT_SRC = client.c
CLIENT_EXE = client

all: $(SERVER_EXE) $(CLIENT_EXE)

$(SERVER_EXE): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_EXE)

$(CLIENT_EXE): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_EXE)

clean:
	rm -f $(SERVER_EXE) $(CLIENT_EXE)
