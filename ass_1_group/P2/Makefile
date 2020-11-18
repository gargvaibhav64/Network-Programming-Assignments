all: clean signal

clean:
	- mkdir d1
	- mkdir d2
	- mkdir d3
	- mkdir d4
	- rm -rf *.out
	- rm -r d1/*
	- rm -r d2/*
	- rm -r d3/*
	- rm -r d4/*

signal:
	gcc -g -o m_server m_server.c
	gcc -g -o d_server d_server.c
	gcc -g -o client client.c
	cp d_server d1/d_server
	cp d_server d2/d_server
	cp d_server d3/d_server
	cp d_server d4/d_server

