all: emulator

emulator:
	gcc -Wall -Werror -o emulator emulator.cpp

clean:
	rm -rf sender requester emulator *.o