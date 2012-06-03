CFLAGS=-Wall -g
nixhttpd: nixhttpd.o sock_wrap.o
	gcc -o nixhttpd nixhttpd.o sock_wrap.o

nixhttpd.o: nixhttpd.c
	gcc -c nixhttpd.c
sock_wrap.o: sock_wrap.c sock_wrap.h
	gcc -c sock_wrap.c

clean:
	-rm -f sock_wrap.o
	-rm -f nixhttpd.o
	-rm -f nixhttpd

.PHONY: clean
	
