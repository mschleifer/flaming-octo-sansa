all: sender requester emulator

sender:
	gcc -Wall -Werror -o sender sender.c
	cp sender sender1/sender
	cp sender sender2/sender

requester:
	gcc -Wall -Werror -o requester requester.c
	cp requester request_dir/requester
	rm requester

emulator:
	gcc -Wall -Werror -o emulator emulator.c

clean:
	rm -rf sender requester emulator *.o
	rm -rf sender1/*.o
	rm -rf sender2/*.o
	rm -rf request_dir/*.o
