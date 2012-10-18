all: sender requester

sender:
	gcc -Wall -Werror -o sender sender.c

requester:
	gcc -Wall -Werror -o requester requester.c
	cp requester request_dir/requester

clean:
	rm -rf sender requester *.o
	rm *~
