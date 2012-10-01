all: sender requester

sender:
	gcc -Wall -Werror -o sender sender.c

requester:
	gcc -Wall -Werror -o requester requester.c

clean:
	rm -rf sender requester *.o
