all: emulator

emulator:
	g++ -Wall -Werror -o emulator emulator.cpp

clean:
	rm -rf emulator *.o