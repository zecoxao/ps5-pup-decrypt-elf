#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <netinet/tcp.h>

extern "C" {
#include <ps5/kernel.h>
}

#include "defines.h"
#include "debug.h"
#include "decrypt.h"

#define IP(a, b, c, d) (((a) << 0) + ((b) << 8) + ((c) << 16) + ((d) << 24))


int g_debug_sock;

int sock;

int main(){
	
	pid_t pid;
	
	pid = getpid();
	kernel_set_ucred_authid(pid, 0x4801000000000013L);
    
	
	// Jailbreak
    kernel_set_proc_rootdir(getpid(), kernel_get_root_vnode());

	printf_notification("PUP Decrypt started!");
  
	struct sockaddr_in server;

	server.sin_len = sizeof(server);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = DEBUG_ADDR;                //in defines.h
	server.sin_port = htons (DEBUG_PORT);          //in defines.h
	memset(server.sin_zero, 0, sizeof(server.sin_zero));
	g_debug_sock = socket(AF_INET, SOCK_STREAM, 0);
	connect(sock, (struct sockaddr *)&server, sizeof(server));

	int flag = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

	//Output paths must already exist!
	decrypt_pups("/mnt/usb0/safe.PS5UPDATE.PUP", "/mnt/usb0/%s.dec"); // replace with /mnt/usb0/safe.PROSPEROUPDATE.PUP for legacy < 2.xx pups

	printf_notification("Complete!");
  
	return 0;
}
