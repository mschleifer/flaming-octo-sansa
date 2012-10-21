all: sender requester

sender:
	gcc -Wall -Werror -o sender sender.c
	cp sender sender1/sender
	cp sender sender2/sender

requester:
	gcc -Wall -Werror -o requester requester.c
	cp requester request_dir/requester

clean:
	rm -rf sender requester *.o
	rm -rf sender1/*.o
	rm -rf sender2/*.o
	rm -rf request_dir/*.o
