/*
 * 2012/05/02 - 2012/05/31 @nixzhu
 * a simple httpd server, here we go.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <ctype.h>

#include "sock_wrap.h"

#define MAXLINE 102400
#define PATHLEN 256
#define LEN 1024

#define UPLOADPATH "/upload.cgi"

#define FILE_JSON 4
#define FILE_JS 3
#define FILE_CSS 2
#define FILE_HTML 1
#define FILE_DIRECT 0

struct param {
        char name[128];
        char value[128];
        struct param *next;
};

void showbuf(char *buf, int n);
char *sh2html(char *filepath);
int read_config();
int rsp_root_page(char *buf, int connfd);
void rsp_404_head(int connfd);
void rsp_js_head(int connfd);
void rsp_json_head(int connfd);
void rsp_css_head(int connfd);
void rsp_http_head(int connfd);
void rsp_plain_head(int connfd);
void rsp_404_page(char *buf, int connfd);
char *full_path(char *filepath);
int rsp_file(char *buf, int connfd, char *filepath, int normal);
int get_file_path(char *buf, char *path);
int get_filetype(char *filepath);
int file_in_httpcontent(int httpfd, int filelen, char *str);
char get_req_type(char *buf);
struct param * parse_GET_param(char *buf);
struct param * parse_POST_param(char *buf);

/* Global */
char wwwdir[PATHLEN];
int port;

int main()
{
	int filetype = 1;
        struct sockaddr_in servaddr, cliaddr;
        socklen_t cliaddr_len = sizeof(struct sockaddr_in); /* in/out arg */
        int listenfd, connfd;
        char buf[MAXLINE];
        char filepath[PATHLEN];
        char boundary[128];
	int total_recv;
	int total_all;
	int header_len;
	char content_length[128];
	int content_len;
        int tmpfd;
        char *content;
        char *begin;
        char *end;
        int n;
        int i;

        int childpid;

        int opt = 1;
        struct stat st;
        char req_type;
        struct param *prm;

	/* the server as a daemon run in background, try `ps -x' to find it.*/
	//daemon(0,0);

	signal(SIGPIPE, SIG_IGN);

	filetype = 0;
        read_config(&port, wwwdir);

        listenfd = Socket(AF_INET, SOCK_STREAM, 0);

        memset(&servaddr, 0, sizeof(struct sockaddr_in));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));

        Listen(listenfd, 20);

        while(1) {
		memset(&cliaddr, 0, sizeof(struct sockaddr_in));
                connfd = Accept(listenfd, 
                                (struct sockaddr*)(&cliaddr),
                                &cliaddr_len);

                memset(buf, '\0', MAXLINE);
                n = read(connfd, buf, MAXLINE);
                if (strlen(buf) < 4) {
                        printf("NOTICE:request too short.. continue\n");
                        Close(connfd);
                        continue;
                } else {
			showbuf(buf,20);
		}

                get_file_path(buf, filepath);
		printf("file path: %s\n", filepath);

                /* if it's upload request */
                if (strcmp(filepath, full_path(UPLOADPATH)) == 0) {
                        printf("UPLOAD now... %s\n", UPLOADPATH);
                        /* now we save all request content in a tmp file */
                        tmpfd = open("nixtmp.dat", O_RDWR|O_CREAT, 0666);
                        write(tmpfd, buf, n);

			/* but we get some info first */
			/* get content-length */
			memset(content_length, '\0', 128);
			content = strstr(buf, "Content-Length: ");
			begin = content + strlen("Content-Length: ");
			end = strstr(begin, "\r\n");
			strncpy(content_length, begin, (int)end-(int)begin);
			printf("content-length str = |%s|\n", content_length);
			content_len = atoi(content_length);
			printf("content-length int = |%d|\n", content_len);

			/* get the Header length */
			content = strstr(buf,"\r\n\r\n");
			header_len = (int)(content)+strlen("\r\n\r\n") - (int)buf;
			printf("Header length: %d\n", header_len);
			/* length of header and content */
			total_all = header_len + content_len;

			/* get the boundary string */
			memset(boundary, '\0', 128);
                        content = strstr(buf, "boundary=");
                        content += strlen("boundary=");
                        boundary[0] = '-';
                        boundary[1] = '-'; /* end boundary have two more '-' */
                        for (i = 0; strncmp(content+i, "\r\n", strlen("\r\n")); i++) {
                                boundary[i+2] = *(content+i);
                        }
                        printf("boundary is |%s|\n", boundary);

			/* OK, continue recv the rest of content */
			total_recv = n; /* the first n bytes */
			while(total_recv < total_all) {
				n = read(connfd, buf, MAXLINE);
				total_recv += n;
				write(tmpfd, buf, n);
			}
			printf("recv:%d, total:%d.\n", total_recv, total_all);

                        /* let's take a look tmp file */
                        lseek(tmpfd, 0, SEEK_SET);
                        memset(&st, 0, sizeof(struct stat)); // every time fresh it
                        stat("nixtmp.dat", &st);
                        printf("upcontent size:%d\n", (int)st.st_size); /* TODO */

			/* now, we rebuild the upload file*/
			file_in_httpcontent(tmpfd, (int)st.st_size, boundary);

                        close(tmpfd);

                        /* resonse something */
                        rsp_file(buf, connfd, full_path("/uploadsucc.html"), FILE_HTML);
                        Close(connfd);
                        continue;
                }


		/* Normal request, no upload */
                printf("\nA[%d]---$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n", n);
                for (i = 0; i < n; i++) {
                        printf("%c",buf[i]);
                }
                printf("\nB[%d]---$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n", n);




                /* 
                 * parse URL now
                 * TODO
                 */
                req_type = get_req_type(buf);
                if (req_type == 'G') { /* GET */
                        prm = parse_GET_param(buf);
                } else if (req_type == 'P') { /* POST */
                        parse_POST_param(buf);
                }


                printf("------------------------------------\n");
                /* get URL request from browser */
                get_file_path(buf, filepath);
                //printf("%d %d\n", strlen(buf), strlen(wwwdir));
                if (strlen(filepath) == (strlen(wwwdir)+1)) {
                        rsp_root_page(buf, connfd);
                        Close(connfd);
                        continue;
                }
                memset(&st, 0, sizeof(struct stat)); // every time fresh new
//              stat(buf, &st);
                stat(filepath, &st);
                //printf("file st_mode:%x, %x\n", st.st_mode, S_IXUSR);
                if ((st.st_mode & S_IXUSR) == S_IXUSR) {
                        printf("[%s] can be EXEC.\n", filepath);   


			//Write(connfd, plain, strlen(plain));
			//rsp_plain_head(connfd);
			
			rsp_http_head(connfd);
			//rsp_file(buf, connfd, sh2html(filepath), FILE_HTML);
			//Close(connfd);



                        childpid = fork();
                        if (childpid == 0) {

                                printf("child run\n");
                                dup2(connfd, STDOUT_FILENO);
                                //execl(buf, NULL, NULL); // cgi process
                                //execl(buf, buf, prm->value, NULL); // cgi process
                                //execl(filepath, filepath, NULL,NULL); // cgi process
                                //execl("/var/www_nix/cgi.py", "xxx", NULL); // cgi process
                                if (prm) {
                                        execl(filepath, filepath, prm->value, NULL); // cgi process
                                } else {
                                        execl(filepath, filepath, NULL,NULL);
                                }
                                //execl("/usr/bin/python",buf, "xxxxxxxxxxx", NULL); // cgi process
                                //execl(buf, "xxxxxxxxxxx", NULL); // cgi process
                        } else {
                                waitpid(childpid, NULL, 0);
                                Close(connfd);
                                continue; /* parent goto other connect */
                        }
                } else {
                        //rsp_file(buf, connfd, "/var/www_nix/index.html");
			//filetype = get_filetype(filepath);
                        //rsp_file(buf, connfd, filepath, 1);
                        rsp_file(buf, connfd, filepath, get_filetype(filepath));
                        Close(connfd);
                }
                //Close(connfd);
        }
        return 0;
}

void showbuf(char *buf, int n)
{
        int i;
	printf("show buf\n");
        for (i = 0; i < n; i++) {
                if (buf[i] == '\0')
                        break;
                printf("%c", buf[i]);
        }
	printf("\nshow buf end\n");
}

char *sh2html(char *filepath)
{
	int len = strlen(filepath) + 3;
	char *html = malloc(len);
	memset(html, '\0', len);
	char *index = strchr(filepath, '.');
	int i;
	for ( i = 0; filepath+i != index; i++) {
		html[i] = filepath[i];
	}
	html[i] = '.';
	html[i+1] = 'h';
	html[i+2] = 't';
	html[i+3] = 'm';
	html[i+4] = 'l';
	return html;
}

int get_filetype(char *filepath)
{
	//int ft;
	static int json_num = 0;
	char *index;
	//printf("get FFFFFFFFFFFFfile type..:|%s|\n", filepath);
	index = strchr(filepath, '.');
	if (index != NULL) {
		//printf("33333:%c%c%c\n", *(index+1),*(index+2),*(index+3));
		if (strncmp(index, ".html", 5) == 0) {
			return FILE_HTML;
		}
		if (strncmp(index, ".json", 5) == 0) {
			printf("JSON________%d\n", json_num++);
			return FILE_JSON;
		}
		if (strncmp(index, ".css", 4) == 0) {
			return FILE_CSS;
		}
		if (strncmp(index, ".js", 3) == 0) {
			return FILE_JS;
		}	
	}
	//printf("get FFFFFFFFFFFfile type end\n");
	return FILE_DIRECT;
}

char get_req_type(char *buf)
{
        return buf[0];
}

struct param * parse_GET_param(char *buf)
{
        char *index;
        int i;
        int n;
//      char *bufhead[256];
        struct param *preone;
        struct param *one = (struct param *)malloc(sizeof(struct param));
        struct param *head = one;
        struct param *thisone = head;

        memset(head, 0, sizeof(struct param));
        //printf(">>>%c\n", buf[10]);
        /* find '?' first */
        //index = strstr(buf, "\r\n");
        //memset(bufhead, '\0', 256);
        //for (i = 0; buf+i != index && i < 256-1; i++) {
        //      bufhead[i] = buf[i];
        //}
        //printf("i = %d\n", i);
        //sleep(1);
        //index = strchr(bufhead, '?');
        index = strchr(buf, '?');
        if (index == NULL)
                return NULL;
        n = 0;
        while (1 && n++ < 5) {
                index++;
                for (i = 0; *index != '='; i++) {
                        one->name[i] = *index ;
                        index++;
                }
                //xindex++;
                //index = strchr(index, '=');
                //if (index == NULL)
                //      return NULL;
                index++;
                for (i = 0; *index != ' ' && *index != '&' ; i++) {
                        one->value[i] = *index;
                        index++;
                        //if (*(index + i) == ' ')
                        //      goto end;
                }
                if (*index == ' ')
                        break;//goto end;
                preone = one;
                one = (struct param *)malloc(sizeof(struct param));
                memset(one, 0, sizeof(struct param));
                preone->next = one;
        }
//end:
        n = 0;
        while (thisone) {
                printf("n%d\n", n++);
                printf("param:|%s|\n", thisone->name);
                printf("value:|%s|\n", thisone->value);
                thisone = thisone->next;
        }
        return head;
}
struct param * parse_POST_param(char *buf)
{
        return NULL;
}

int read_config(int *port, char *wwwdir)
{
        int i;
        int fd;
        char *d;
        char buf[MAXLINE];
        char portbuf[MAXLINE];
        char *dirstr = "directory=";
        char *portstr = "port=";

        memset(wwwdir, '\0', PATHLEN);
        memset(buf, '\0', MAXLINE);

        fd = open("/etc/nixhttpd.conf", O_RDONLY);
	if (fd < 0) {
		printf("can not open /etc/nixhttp.conf\n");
		exit(1);
	}
        read(fd, buf, MAXLINE);

        d = strstr(buf, dirstr);
        d = d + strlen(dirstr);
        for (i = 0; *(d+i) != '\n'; i++) {
                wwwdir[i] = *(d+i);
        }
        printf("|%s|\n", wwwdir);

        d = strstr(buf, portstr);
        d = d + strlen(portstr);
        memset(portbuf, '\0', MAXLINE);
        for (i = 0; *(d+i) != '\n'; i++) {
                //p = p*10 + (*(d+i)-'0');
                portbuf[i] = *(d+i);
        }
        *port = atoi(portbuf);
        printf("|%d|\n", *port);

        return 0;
}

int rsp_root_page(char *buf, int connfd)
{
        rsp_file(buf, connfd, full_path("/index.html"), 1);

        /*
          memset(buf, '\0', MAXLINE);
          strcat(buf, "HTTP/1.1 200 OK\r\n");
          strcat(buf, "Content-Type: text/html\r\n");
          strcat(buf, "\r\n");
          strcat(buf, "<html><head><title>Home</title></head>");
          strcat(buf, "<body><p>Hi...</p></body>");
          strcat(buf, "\r\n");
          strcat(buf, "\r\n");
          write(connfd, buf, MAXLINE);
        */
        return 0;
}
void rsp_404_page(char *buf, int connfd)
{
        rsp_file(buf, connfd, full_path("/404.html"), 0);
        return;
        char *content[] = {
                "<html><head><title>404 Not Found</title></head><body>",
                "<h1>404: Not Found</h1>",
                "<p>so, <a href=\"/\">go home</a> please!</p>",
                "</body></html>"
        };
        int i;

        for (i = 0; i < sizeof(content)/sizeof(char *); i++) {
                write(connfd, content[i], strlen(content[i]));
        }
}
void rsp_404_head(int connfd)
{
        char *httphead = 
                "HTTP/1.1 404 Not Found\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
        write(connfd, httphead, strlen(httphead));
}
void rsp_js_head(int connfd)
{
        char *httphead = "HTTP/1.1 200 OK\r\nContent-Type: application/x-javascript; charset=UTF-8\r\n\r\n";
        //char *httphead = "HTTP/1.1 200 OK\r\nContent-Type: text/javascript\r\n\r\n";
        write(connfd, httphead, strlen(httphead));
}
void rsp_json_head(int connfd)
{
        char *httphead = "HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=UTF-8\r\n\r\n";        //char *httphead = "HTTP/1.1 200 OK\r\nContent-Type: text/javascript\r\n\r\n";
        write(connfd, httphead, strlen(httphead));
}
void rsp_css_head(int connfd)
{
        char *httphead = "HTTP/1.1 200 OK\r\nContent-Type: text/css; charset=UTF-8\r\n\r\n";
        write(connfd, httphead, strlen(httphead));
}
void rsp_http_head(int connfd)
{
        char *httphead = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
        write(connfd, httphead, strlen(httphead));
}
void rsp_plain_head(int connfd)
{
        char *httphead = "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=UTF-8\r\n\r\n";
        write(connfd, httphead, strlen(httphead));
}
char *full_path(char *filepath)
{
	int len = strlen(wwwdir) + strlen(filepath) + 1;
	char *fullpath = (char *)malloc(len);
	memset(fullpath, '\0', len);
	memcpy(fullpath, &wwwdir, strlen(wwwdir));
	memcpy(fullpath + strlen(wwwdir), filepath, strlen(filepath));
	return fullpath;
}
int rsp_file(char *buf, int connfd, char *filepath, int normal)
{
        int fd;
        int n;
 
        fd = open(filepath, O_RDONLY);
        if (fd == -1) {
                printf("open failed %s\n", filepath);
                rsp_404_page(buf, connfd);
                return -1;
        }
	switch(normal) {
	case FILE_HTML:
                rsp_http_head(connfd);
		break;
	case FILE_CSS:
		rsp_css_head(connfd);
		break;
	case FILE_JS:
		rsp_js_head(connfd);
		break;
	case FILE_JSON:
		rsp_json_head(connfd);
		break;
	case FILE_DIRECT:
		break;
	}
/*
  if (normal == FILE_HTML)
  rsp_http_head(connfd);
  else if (normal == FILE_DIRECT)
  rsp_404_head(connfd);
*/
	//else if (normal == 2)
	/* CSS file */

        memset(buf, '\0', MAXLINE);
        while((n = read(fd, buf, MAXLINE)) > 0) {
                write(connfd, buf, n);
                memset(buf, '\0', MAXLINE);
        }
	close(fd);
        printf("rsp file %s\n", filepath);
        return 0;
}

/* "/xxx" is our target */
//GET /xxx?a=apple HTTP/1.1
//Host: 192.16.14.26:8888
//Connection: keep-alive
//User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.79 Safari/535.11
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
//Accept-Encoding: gzip,deflate,sdch
//Accept-Language: zh-CN,zh;q=0.8
//Accept-Charset: UTF-8,*;q=0.5
int get_file_path(char *buf, char *path)
{
        char bufhead[256];
        int i;
        char *b1;
        char *b2;
        char *index;

        memset(path, '\0', PATHLEN);
        strcat(path, wwwdir);
        //printf("buf:|%s|\n", buf);
        index = strstr(buf, "\r\n");

        memset(bufhead, '\0', 256);
        for (i = 0; buf+i != index && i < 256-1; i++) {
                bufhead[i] = buf[i];
        }
        printf(">i = %d\n", i);

//      printf("SSS1\n");
        b1 = strchr(bufhead, ' ');      /* first blank */
//      printf("SSS2:%c\n", *(b1+1));
        b2 = strchr(b1+1,' ');  /* second blank */
//      printf("SSS3:%c\n", *(b2+1));
        for (b1+=1, i = 0; b1 != b2; b1++, i++) {
                if (*b1 == '?')
                        break;
                path[i+strlen(wwwdir)] = *b1;
        }

        printf("request path is \"%s\"\n", path);
        return 0;
}




int file_in_httpcontent(int httpfd, int filelen, char *str)
{
	char *p;
	int offset = 0;;
	int findcount = 0;
	int i, n;
	char buf[LEN];
	char *content;
	char *begin;
	char *end;
	char fname[128];
	int newfd;
	int first = 1;
	int IEfile = 0;

	n = read(httpfd, buf, LEN);
	content = strstr(buf, "filename=\"");
	begin = content+strlen("filename=\"");
	end = strchr(begin, '\"');

	/* deal IE used long file name */
	for (content = end; content != begin; content--) {
		if (*content == '\\') {
			IEfile = 1;
			break;
		}
	}
	if (IEfile)
		begin = content+1;


	memset(fname, '\0', 128);

	/* now we set the filename */
	//strcpy(fname, wwwdir);
	//strcpy(fname+strlen(wwwdir), "/");
	//strncpy(fname+strlen(wwwdir)+1, begin, (int)end-(int)begin);

	strcpy(fname, full_path("/upload/"));
	//strcpy(fname+strlen(wwwdir), "/");
	strncpy(fname+strlen(full_path("/upload/")), begin, (int)end-(int)begin);

	printf(">>>File Name=|%s|\n", fname);

	newfd = open(fname, O_RDWR|O_CREAT, 0777);

	content = strstr(content, "\r\n\r\n");
	content += strlen("\r\n\r\n");

	offset = (int)content - (int)buf;
	printf(">>>begin offset: %d\n", offset);

/*
  begin = content;
//                      end = strstr(content, "\r\n\r\n");
//                      end = strstr(content, boundary);
end = NULL;
end = strstr(begin, boundary);
*/


	memset(buf, '\0', LEN);
	p = buf;//(char *)malloc(LEN);
	while (offset+LEN-filelen < LEN) {

		lseek(httpfd, offset, SEEK_SET);
		//printf("===offset: %d\n", offset);

		n = 0;
		if (offset+LEN < filelen) {
			n = read(httpfd, p, LEN);
			printf("<<<<<<<<<<<<@@1: %d\n", n);
		} else {
			n = read(httpfd, p, offset+LEN-filelen);
			printf("<<<<<<<<<<<<@@2: %d\n", n);
		}
		for (i = strlen(str); i < n; i++) {
			//printf("%c", p[i]);
		}
		printf(">>>n = %d\n", n);
		if (n >= strlen(str)) {
			for (i = 0; i < n-strlen(str); i++) {
				if (memcmp(p+i, str, strlen(str)) == 0) {
					printf(">>>got you: %d\n", offset+i);
					findcount += 1;
					break;
				} else {

				}
			}
			if (findcount == 0) {
				printf("in!!!!!!!!!!   i=%d\n", i);
				if (first) {
					write(newfd, p, n);
					first = 0;
				}
				else
					write(newfd, p+strlen(str), n-strlen(str));

			} else {
				//write(newfd, p, i);
				if (first)
					write(newfd, p, i-strlen("\r\n"));
				else
					write(newfd, p+strlen(str), i-strlen(str)-strlen("\r\n"));
				printf("out!!!!!!!!!!   i=%d\n", i);
				break;
			}
			if (n == strlen(str)) {
				break;
			}
			offset += (n-strlen(str));
		} else {
			offset = filelen;
			printf("---------------------------------\n");
			break;
		}
	}
	printf("findcount = %d\n", findcount);

	close(newfd);
	return 0;
}
