all: emulator

emulator:
	g++ -Wall -Werror -o emulator emulator_p3.cpp

clean:
	rm -rf emulator *.o