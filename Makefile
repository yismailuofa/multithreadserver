CC=gcc
CFLAGS=-g -Wall
DEPS=common.h timer.h
LIBS=-lm -lpthread


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: server client attacker

server: server.o
	$(CC) -o main $^ $(CFLAGS) $(LIBS)

client: client.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

attacker: attacker.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o main client attacker server_output_time_aggregated