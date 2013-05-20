/*  http_post_simple - do a POST to an HTTP URL and return the values

 Copyright GPL 2003 by Mike Chirico <mchirico@brainsquash.org>

 A few things to note with this example:
 .. note the \r\n .. it needs a carrage return line feed

 strcpy(sendline,"POST /test.php HTTP/1.0\r\n");
 strcat(sendline,"Host: souptonuts.sourceforge.net\r\n");

 souptonuts.sourceforge.net ip address is 66.35.250.209
 these need to match for it to work.

 strcat(sendline,"Content-length: 34\r\n\r\n");
 strcat(sendline,"mode=login&user=test&password=test\r\n");

 Content-length, 34,  is the length of the string "mode=login&user=test&password=test"
 if you add your own string... you need to adjust this value.

 References:

 http://tecfa.unige.ch/moo/book2/node93.html



 [root@third-fl-71 root]# tcpdump src port 8081
 tcpdump: listening on eth0
 10:57:24.356112 192.168.1.155.tproxy > third-fl-71.localdomain.32878: S 525851741:525851741(0) ack 1480974760 win 5792 <mss 1460,sackOK,timestamp 82790685 17692834,nop,wscale 0> (DF)
 10:57:24.358147 192.168.1.155.tproxy > third-fl-71.localdomain.32878: . ack 130 win 6432 <nop,nop,timestamp 82790685 17692834> (DF)
 10:57:24.681157 192.168.1.155.tproxy > third-fl-71.localdomain.32878: . 1:1449(1448) ack 130 win 6432 <nop,nop,timestamp 82790717 17692834> (DF)
 10:57:24.683207 192.168.1.155.tproxy > third-fl-71.localdomain.32878: . 1449:2897(1448) ack 130 win 6432 <nop,nop,timestamp 82790717 17692834> (DF)
 10:57:24.686681 192.168.1.155.tproxy > third-fl-71.localdomain.32878: P 2897:4345(1448) ack 130 win 6432 <nop,nop,timestamp 82790717 17692867> (DF)
 10:57:24.688116 192.168.1.155.tproxy > third-fl-71.localdomain.32878: FP 4345:5079(734) ack 130 win 6432 <nop,nop,timestamp 82790717 17692867> (DF)
 10:57:24.702899 192.168.1.155.tproxy > third-fl-71.localdomain.32878: . ack 131 win 6432 <nop,nop,timestamp 82790718 17692868> (DF)

 Note the size of n that is returned
 [chirico@third-fl-71 spider]$ ./test > out
 1448
 1448
 1448
 734



 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define SA      struct sockaddr
#define MAXLINE 4096

/* Following could be derived from SOMAXCONN in <sys/socket.h>, but many
 kernels still #define it as 5, while actually supporting many more */
#define LISTENQ         1024    /* 2nd argument to listen() */

void process_http(int sockfd)
{
	char sendline[MAXLINE], recvline[MAXLINE];
	ssize_t n;

	//  strcpy(sendline,"POST /chirico/test.php HTTP/1.0\r\n");
	strcpy(sendline, "GET /test.php  HTTP/1.0\r\n");
	strcat(sendline, "Connection: Keep-Alive\r\n");
	strcat(sendline, "Pragma: no-cache\r\n");
	strcat(sendline, "Host: 192.168.1.155\r\n"); //127.0.0.1
	strcat(sendline, "Accept: www/source\r\n");
	strcat(sendline, "Accept: text/html\r\n\r\n");

	write(sockfd, sendline, strlen(sendline));
	while ((n = read(sockfd, recvline, MAXLINE)) != 0)
	{
		recvline[n] = '\0';
		printf("%s", recvline);

	}

}

int main(void)
{
	int sockfd;
	struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8081);
	inet_pton(AF_INET, "192.168.1.155", &servaddr.sin_addr); //127.0.0.1

	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
	process_http(sockfd);
	exit(0);

}
