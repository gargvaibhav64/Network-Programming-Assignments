all: clean trace

clean: 
	- rm trace

trace:
	gcc -pthread -g -o trace fastertraceroute.c helper.c

new:
	gcc -g new.c
