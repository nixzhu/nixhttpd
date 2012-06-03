#ifndef __SOCK_WRAP_H
#define __SOCK_WRAP_H
void print_error_and_exit(const char *s);
int Accept(int fd, struct sockaddr *sa, socklen_t *salen_p);
void Bind(int fd, const struct sockaddr *sa, socklen_t salen);
void Connect(int fd, const struct sockaddr *da, socklen_t salen);
void Listen(int fd, int backlog);
int Socket(int family, int type, int protocol);
ssize_t Read(int fd, void *ptr, size_t nbytes);
ssize_t Write(int fd, const void *ptr, size_t nbytes);
void Close(int fd);
#endif
