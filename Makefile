all: emulator routetrace

emulator:
	g++ -Wall -Werror -o emulator emulator_p3.cpp

routetrace:
	g++ -Wall -Werror -o trace routetrace.cpp

clean:
	rm -rf emulator *.o
	rm -rf trace *.o