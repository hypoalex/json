CC = gcc

SRCS = src/duktape.c src/json.c

all:
	$(CC) -Wall  $(SRCS) -o json

release:
	$(CC) -Wall -O2 $(SRCS) -o json

debug: 
	$(CC) -Wall -g $(SRCS) -o json
	
clean:
	@rm json
	
install:
	@cp man/man1/json.1 /usr/share/man/man1
	@cp json /usr/bin
	
uninstall:
	@rm /usr/share/man/man1/json.1
	@rm /usr/bin/json
