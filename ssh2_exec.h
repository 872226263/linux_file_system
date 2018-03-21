#pragma once
/*
* Sample showing how to use libssh2 to execute a command remotely.
*
* The sample code has fixed values for host name, user name, password
* and command to run.
*
* Run it like this:
*
* $ ./ssh2_exec 127.0.0.1 user password "uptime"
*
*/
#include "libssh2_config.h"
#include <libssh2.h>
# include <winsock2.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <libssh2_sftp.h>

#ifdef WIN32
#include <Winuser.h>
#endif

#define WSASTARTUP_FAILED				1001
#define LIBSSH2_INITIALIZATION_FAILED	1002
#define FAILED_TO_CONNECT				1003
#define FAILURE_ESTABLISHING_SSH_SESSION				1004
#define PASSWORD_ERROR 1005

typedef struct _SSHINFO {
	char username[64];
	char password[128];
	char loclfile[1024];
	char scppath[1024];
	char hostaddrstr[128];
	int port;
}SSHINFO;

#define MessageError(str) MessageBox(0,(str),"Error",0)

MYAPI int scp_write(SSHINFO si);
MYAPI int login_test(SSHINFO si);
MYAPI int ssh_exec_ls(SSHINFO si, char** alldir, int* alldir_len);
MYAPI int ssh_exec_read(SSHINFO si);
MYAPI void freeP(char** Block) { free(*Block); }
