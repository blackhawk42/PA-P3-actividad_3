CC=gcc
CFLAGS=-pthread

.PHONY: all clean

all: tunnel.exe

tunnel.exe: tunnel.c
	$(CC) $(CFLAGS) -o tunnel.exe tunnel.c

clean:
	rm -rf *.o *.exe
