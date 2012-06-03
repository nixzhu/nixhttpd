#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include "sock_wrap.h"

void print_error_and_exit(const char *s)
{
	perror(s);
	exit(1);
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salen_p)
{
	int n;
again:
	n = accept(fd, sa, salen_p);
	if (n < 0) {
		if ((errno == ECONNABORTED) || (errno == EINTR))
			goto again;
		else
			print_error_and_exit("accept error");
	}
	return n;
}

void Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (bind(fd, sa, salen) < 0)
		print_error_and_exit("bind error");
}

void Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (connect(fd, sa, salen) < 0)
		print_error_and_exit("connect error");
}

void Listen(int fd, int backlog)
{
	if (listen(fd, backlog) < 0)
		print_error_and_exit("listen error");
}

int Socket(int family, int type, int protocol)
{
	int n;
	if ((n = socket(family, type, protocol)) < 0)
		print_error_and_exit("socket error");
	return n;
}

ssize_t Read(int fd, void *ptr, size_t nbytes)
{
	ssize_t n;
again:
	if((n = read(fd, ptr, nbytes)) == -1) {
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}

ssize_t Write(int fd, const void *ptr, size_t nbytes)
{
	ssize_t n;
again:
	if((n = write(fd, ptr, nbytes)) == -1) {
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}

void Close(int fd)
{
	if (close(fd) == -1)
		print_error_and_exit("close error");
}
