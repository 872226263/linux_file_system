#pragma once
#include "ssh2_exec.h"

int scp_write(SSHINFO si)
{
	int sock,  auth_pw = 1;
	struct sockaddr_in sin;
	LIBSSH2_SESSION *session = NULL;
	LIBSSH2_CHANNEL *channel;
	char *username = si.username;
	char *password = si.password;
	char *loclfile = si.loclfile;
	char *scppath = si.scppath;
	int i = strlen(loclfile) - 1; int j = 0;
	for (; i >= 0; i--) {
		if (loclfile[i] == '\\') {
			i++;
			break;
		}
	}
	scppath[strlen(scppath)] = '/'; j++;
	for (j = 0; i < strlen(loclfile) && (strlen(scppath) < 1023); i++,j++) {
		scppath[strlen(scppath)] = loclfile[i];
	}
	scppath[strlen(scppath) + j] = '\0';
	unsigned long  hostaddr = inet_addr(si.hostaddrstr);
	FILE *local;
	int rc;
	char mem[1024];
	size_t nread;
	char *ptr;
	struct stat fileinfo;

#ifdef WIN32
	WSADATA wsadata;
	int err;

	err = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (err != 0) {
		char messtr[256];
		sprintf(messtr, "WSAStartup failed with error: %d\n", err);

		MessageError(messtr);
		return 1;
	}
#endif

	rc = libssh2_init(0);
	if (rc != 0) {
		char messtr[256];
		sprintf(messtr, "libssh2 initialization failed (%d)\n", rc);
		MessageError(messtr);
		return 1;
	}

	local = fopen(loclfile, "rb");
	if (!local) {
		char messtr[256];
		sprintf(messtr, "Can't open local file %s\n", loclfile);
		MessageError(messtr);
		return -1;
	}

	stat(loclfile, &fileinfo);

	/* Ultra basic "connect to port 22 on localhost"
	* Your code is responsible for creating the socket establishing the
	* connection
	*/
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sock) {
		MessageError("failed to create socket!\n");
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(si.port);
	sin.sin_addr.s_addr = hostaddr;
	if (connect(sock, (struct sockaddr*)(&sin),
		sizeof(struct sockaddr_in)) != 0) {
		MessageError("failed to connect!\n");
		return -1;
	}

	/* Create a session instance
	*/
	session = libssh2_session_init();
	if (!session)
		return -1;

	/* ... start it up. This will trade welcome banners, exchange keys,
	* and setup crypto, compression, and MAC layers
	*/
	rc = libssh2_session_handshake(session, sock);
	if (rc) {
		char messtr[256];
		sprintf(messtr, "Failure establishing SSH session: %d\n", rc);
		MessageError(messtr);
		return -1;
	}

	/* At this point we havn't yet authenticated.  The first thing to do
	* is check the hostkey's fingerprint against our known hosts Your app
	* may have it hard coded, may go to a file, may present it to the
	* user, that's your call
	*/

	/*fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
	{
		char messtr[256];
		int len = 0;
		len += sprintf(messtr + len, "Fingerprint: ");
		for (i = 0; i < 20; i++) {
			len += sprintf(messtr + len, "%02X ", (unsigned char)fingerprint[i]);
		}
		len += sprintf(messtr + len, "\n");
		MessageError(messtr);
	}*/

	if (auth_pw) {
		/* We could authenticate via password */
		if (libssh2_userauth_password(session, username, password)) {
			MessageError("Authentication by password failed.\n");
			goto shutdown;
		}
	}
	else {
		/* Or by public key */
		if (libssh2_userauth_publickey_fromfile(session, username,
			"/home/username/.ssh/id_rsa.pub",
			"/home/username/.ssh/id_rsa",
			password)) {
			MessageError("\tAuthentication by public key failed\n");
			goto shutdown;
		}
	}

	/* Send a file via scp. The mode parameter must only have permissions! */
	channel = libssh2_scp_send(session, scppath, fileinfo.st_mode & 0777,
		(unsigned long)fileinfo.st_size);

	if (!channel) {
		char *errmsg;
		int errlen;
		int err = libssh2_session_last_error(session, &errmsg, &errlen, 0);
		char messtr[256];
		sprintf(messtr, "Unable to open a session: (%d) %s\n", err, errmsg);
		MessageError(messtr);
		goto shutdown;
	}

	do {
		nread = fread(mem, 1, sizeof(mem), local);
		if (nread <= 0) {
			/* end of file */
			break;
		}
		ptr = mem;

		do {
			/* write the same data over and over, until error or completion */
			rc = libssh2_channel_write(channel, ptr, nread);
			if (rc < 0) {
				char messtr[256];
				sprintf(messtr, "ERROR %d\n", rc);
				MessageError(messtr);
				break;
			}
			else {
				/* rc indicates how many bytes were written this time */
				ptr += rc;
				nread -= rc;
			}
		} while (nread);
	} while (1);

	libssh2_channel_send_eof(channel);

	libssh2_channel_wait_eof(channel);

	{
		char messtr[256];
		sprintf(messtr, "Complete,%d BYTE had been send\n", rc);
		MessageBox(0, messtr, "", 0);
	}
	libssh2_channel_wait_closed(channel);

	libssh2_channel_free(channel);
	channel = NULL;

shutdown:

	if (session) {
		libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
		libssh2_session_free(session);
	}
#ifdef WIN32
	closesocket(sock);
#else
	close(sock);
#endif
	if (local)
		fclose(local);

	libssh2_exit();

	return 0;
}

int login_test(SSHINFO si)
{
	int sock, auth_pw = 1;
	struct sockaddr_in sin;
	LIBSSH2_SESSION *session = NULL;
	char *username = si.username;
	char *password = si.password;
	unsigned long  hostaddr = inet_addr(si.hostaddrstr);
	int rc;

#ifdef WIN32
	WSADATA wsadata;
	int err;

	err = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (err != 0) {
		char messtr[256];
		sprintf(messtr, "WSAStartup failed with error: %d\n", err);

		MessageError(messtr);
		return 1;
	}
#endif

	rc = libssh2_init(0);
	if (rc != 0) {
		char messtr[256];
		sprintf(messtr, "libssh2 initialization failed (%d)\n", rc);
		MessageError(messtr);
		return 1;
	}

	/* Ultra basic "connect to port 22 on localhost"
	* Your code is responsible for creating the socket establishing the
	* connection
	*/
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sock) {
		MessageError("failed to create socket!\n");
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(si.port);
	sin.sin_addr.s_addr = hostaddr;
	if (connect(sock, (struct sockaddr*)(&sin),
		sizeof(struct sockaddr_in)) != 0) {
		MessageError("failed to connect!\n");
		return -1;
	}

	/* Create a session instance
	*/
	session = libssh2_session_init();
	if (!session)
		return -1;

	/* ... start it up. This will trade welcome banners, exchange keys,
	* and setup crypto, compression, and MAC layers
	*/
	rc = libssh2_session_handshake(session, sock);
	if (rc) {
		char messtr[256];
		sprintf(messtr, "Failure establishing SSH session: %d\n", rc);
		MessageError(messtr);
		return -1;
	}

	if (auth_pw) {
		/* We could authenticate via password */
		if (libssh2_userauth_password(session, username, password)) {
			MessageError("Authentication by password failed.\n");
			goto shutdown;
		}
	}
	else {
		/* Or by public key */
		if (libssh2_userauth_publickey_fromfile(session, username,
			"/home/username/.ssh/id_rsa.pub",
			"/home/username/.ssh/id_rsa",
			password)) {
			MessageError("\tAuthentication by public key failed\n");
			goto shutdown;
		}
	}

	/* Send a file via scp. The mode parameter must only have permissions! */

shutdown:

	if (session) {
		libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
		libssh2_session_free(session);
	}
#ifdef WIN32
	closesocket(sock);
#else
	close(sock);
#endif

	libssh2_exit();

	return 0;
}

static int waitsocket(int socket_fd, LIBSSH2_SESSION *session)
{
	struct timeval timeout;
	int rc;
	fd_set fd;
	fd_set *writefd = NULL;
	fd_set *readfd = NULL;
	int dir;

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	FD_ZERO(&fd);

	FD_SET(socket_fd, &fd);

	/* now make sure we wait in the correct direction */
	dir = libssh2_session_block_directions(session);

	if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;

	if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;

	rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

	return rc;
}

int ssh_exec_ls(SSHINFO si,char** alldir, int* alldir_len)
{
	int sock, auth_pw = 1;  
	struct sockaddr_in sin;
	LIBSSH2_SESSION *session = NULL;
	LIBSSH2_CHANNEL *channel;
	char *username = si.username;
	char *password = si.password;
	char *scppath = si.scppath;

	char commandline[256];
	sprintf(commandline, "ls -F -all %s", scppath);

	unsigned long  hostaddr = inet_addr(si.hostaddrstr);
	int rc;

#ifdef WIN32
	WSADATA wsadata;
	int err;

	err = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (err != 0) {
		char messtr[256];
		sprintf(messtr, "WSAStartup failed with error: %d\n", err);

		MessageError(messtr);
		return 1;
	}
#endif

	rc = libssh2_init(0);
	if (rc != 0) {
		char messtr[256];
		sprintf(messtr, "libssh2 initialization failed (%d)\n", rc);
		MessageError(messtr);
		return 1;
	}


	/* Ultra basic "connect to port 22 on localhost"
	* Your code is responsible for creating the socket establishing the
	* connection
	*/
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sock) {
		MessageError("failed to create socket!\n");
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(si.port);
	sin.sin_addr.s_addr = hostaddr;
	if (connect(sock, (struct sockaddr*)(&sin),
		sizeof(struct sockaddr_in)) != 0) {
		MessageError("failed to connect!\n");
		return -1;
	}

	/* Create a session instance
	*/
	session = libssh2_session_init();
	if (!session)
		return -1;

	/* ... start it up. This will trade welcome banners, exchange keys,
	* and setup crypto, compression, and MAC layers
	*/
	rc = libssh2_session_handshake(session, sock);
	if (rc) {
		char messtr[256];
		sprintf(messtr, "Failure establishing SSH session: %d\n", rc);
		MessageError(messtr);
		return -1;
	}

	/* At this point we havn't yet authenticated.  The first thing to do
	* is check the hostkey's fingerprint against our known hosts Your app
	* may have it hard coded, may go to a file, may present it to the
	* user, that's your call
	*/

	/*fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
	{
	char messtr[256];
	int len = 0;
	len += sprintf(messtr + len, "Fingerprint: ");
	for (i = 0; i < 20; i++) {
	len += sprintf(messtr + len, "%02X ", (unsigned char)fingerprint[i]);
	}
	len += sprintf(messtr + len, "\n");
	MessageError(messtr);
	}*/

	if (auth_pw) {
		/* We could authenticate via password */
		if (libssh2_userauth_password(session, username, password)) {
			MessageError("Authentication by password failed.\n");
			goto shutdown;
		}
	}
	else {
		/* Or by public key */
		if (libssh2_userauth_publickey_fromfile(session, username,
			"/home/username/.ssh/id_rsa.pub",
			"/home/username/.ssh/id_rsa",
			password)) {
			MessageError("\tAuthentication by public key failed\n");
			goto shutdown;
	}
}

	/* Exec non-blocking on the remove host */
	while ((channel = libssh2_channel_open_session(session)) == NULL &&
		libssh2_session_last_error(session, NULL, NULL, 0) ==
		LIBSSH2_ERROR_EAGAIN)
	{
		waitsocket(sock, session);
	}
	if (channel == NULL)
	{
		MessageError("Error\n");
		return 1;
	}
	while ((rc = libssh2_channel_exec(channel, commandline)) ==
		LIBSSH2_ERROR_EAGAIN)
	{
		waitsocket(sock, session);
	}
	if (rc != 0)
	{
		MessageError("Error\n");
		return 1;
	}
	int bytecount = 0;
	*alldir_len = 0x4000;
	(*alldir) = malloc(*alldir_len);
	for (;; )
	{
		/* loop until we block */
		int rc;
		do
		{
			char buffer[0x4000];
			rc = libssh2_channel_read(channel, buffer, sizeof(buffer));
			if (rc > 0)
			{
				int i;
				if (rc + bytecount >= *alldir_len) {
					realloc(alldir, *alldir_len *= 2);
				}
				for (i = 0; i < rc; ++i)
					(*alldir)[bytecount + i ] = buffer[i];
				bytecount += rc;
			}
		} while (rc > 0);

		/* this is due to blocking that would occur otherwise so we loop on
		this condition */
		if (rc == LIBSSH2_ERROR_EAGAIN)
		{
			waitsocket(sock, session);
		}
		else
			break;
	}
	(*alldir)[bytecount] = '\0';
	realloc(*alldir, bytecount + 1);
	int exitcode;
	char *exitsignal = (char *)"none";
	exitcode = 127;
	while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sock, session);

	if (rc == 0)
	{
		exitcode = libssh2_channel_get_exit_status(channel);
		libssh2_channel_get_exit_signal(channel, &exitsignal,
			NULL, NULL, NULL, NULL, NULL);
	}

	if (exitsignal) {
		char messtr[256];
		sprintf(messtr, "\nGot signal: %s\n", exitsignal);
		MessageError(messtr);
	}

	libssh2_channel_free(channel);
	channel = NULL;

shutdown:

	libssh2_session_disconnect(session,
		"Normal Shutdown, Thank you for playing");
	libssh2_session_free(session);

#ifdef WIN32
	closesocket(sock);
#else
	close(sock);
#endif

	libssh2_exit();

	return 0;
}

int ssh_exec_read(SSHINFO si)
{
	int sock, auth_pw = 1;
	struct sockaddr_in sin;
	LIBSSH2_SESSION *session = NULL;
	LIBSSH2_CHANNEL *channel;
	char *username = si.username;
	char *password = si.password;
	char *scppath = si.scppath;
	char *localfile = si.loclfile;

	int i = strlen(scppath) - 1; int j = 0;
	for (; i >= 0; i--) {
		if (scppath[i] == '/') {
			i++;
			break;
		}
	}
	localfile[strlen(localfile)] = '\\'; j++;
	for (j = 0; i < strlen(scppath) && (strlen(localfile) < 1023); i++, j++) {
		localfile[strlen(localfile)] = scppath[i];
	}
	localfile[strlen(localfile) + j] = '\0';

	char commandline[256];
	sprintf(commandline, "cat %s", scppath);

	unsigned long  hostaddr = inet_addr(si.hostaddrstr);
	int rc;

#ifdef WIN32
	WSADATA wsadata;
	int err;

	err = WSAStartup(MAKEWORD(2, 0), &wsadata);
	if (err != 0) {
		char messtr[256];
		sprintf(messtr, "WSAStartup failed with error: %d\n", err);

		MessageError(messtr);
		return 1;
	}
#endif

	rc = libssh2_init(0);
	if (rc != 0) {
		char messtr[256];
		sprintf(messtr, "libssh2 initialization failed (%d)\n", rc);
		MessageError(messtr);
		return 1;
	}


	/* Ultra basic "connect to port 22 on localhost"
	* Your code is responsible for creating the socket establishing the
	* connection
	*/
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sock) {
		MessageError("failed to create socket!\n");
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(si.port);
	sin.sin_addr.s_addr = hostaddr;
	if (connect(sock, (struct sockaddr*)(&sin),
		sizeof(struct sockaddr_in)) != 0) {
		MessageError("failed to connect!\n");
		return -1;
	}

	/* Create a session instance
	*/
	session = libssh2_session_init();
	if (!session)
		return -1;

	/* ... start it up. This will trade welcome banners, exchange keys,
	* and setup crypto, compression, and MAC layers
	*/
	rc = libssh2_session_handshake(session, sock);
	if (rc) {
		char messtr[256];
		sprintf(messtr, "Failure establishing SSH session: %d\n", rc);
		MessageError(messtr);
		return -1;
	}

	if (auth_pw) {
		/* We could authenticate via password */
		if (libssh2_userauth_password(session, username, password)) {
			MessageError("Authentication by password failed.\n");
			goto shutdown;
		}
	}
	else {
		/* Or by public key */
		if (libssh2_userauth_publickey_fromfile(session, username,
			"/home/username/.ssh/id_rsa.pub",
			"/home/username/.ssh/id_rsa",
			password)) {
			MessageError("\tAuthentication by public key failed\n");
			goto shutdown;
		}
	}

	/* Exec non-blocking on the remove host */
	while ((channel = libssh2_channel_open_session(session)) == NULL &&
		libssh2_session_last_error(session, NULL, NULL, 0) ==
		LIBSSH2_ERROR_EAGAIN)
	{
		waitsocket(sock, session);
	}
	if (channel == NULL)
	{
		MessageError("Error\n");
		return 1;
	}
	while ((rc = libssh2_channel_exec(channel, commandline)) ==
		LIBSSH2_ERROR_EAGAIN)
	{
		waitsocket(sock, session);
	}
	if (rc != 0)
	{
		MessageError("Error\n");
		return 1;
	}
	int bytecount = 0;
	FILE* file = fopen(localfile, "wb");
	for (;; )
	{
		/* loop until we block */
		int rc;
		do
		{
			char buffer[0x4000];
			rc = libssh2_channel_read(channel, buffer, sizeof(buffer));
			if (rc > 0)
			{
				int i;
				bytecount += rc;
				for (i = 0; i < rc; ++i)
					fputc(buffer[i], file);
			}
			else {
				if (rc != LIBSSH2_ERROR_EAGAIN)
					;// fprintf(stderr, "libssh2_channel_read returned %d\n", rc);
			}
		} while (rc > 0);

		/* this is due to blocking that would occur otherwise so we loop on
		this condition */
		if (rc == LIBSSH2_ERROR_EAGAIN)
		{
			waitsocket(sock, session);
		}
		else
			break;
	}
	fclose(file);
	int exitcode;
	char *exitsignal = (char *)"none";
	exitcode = 127;
	while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sock, session);

	if (rc == 0)
	{
		exitcode = libssh2_channel_get_exit_status(channel);
		libssh2_channel_get_exit_signal(channel, &exitsignal,
			NULL, NULL, NULL, NULL, NULL);
	}

	if (exitsignal) {
		char messtr[256];
		sprintf(messtr, "\nGot signal: %s\n", exitsignal);
		MessageError(messtr);
	}

	libssh2_channel_free(channel);
	channel = NULL;

shutdown:

	libssh2_session_disconnect(session,
		"Normal Shutdown, Thank you for playing");
	libssh2_session_free(session);

#ifdef WIN32
	closesocket(sock);
#else
	close(sock);
#endif

	libssh2_exit();

	return 0;
}


int main() {
	SSHINFO si = { "root","√‹¬Î","C:\\Users\\87222\\Desktop\\","/home/Plaintext.txt","104.224.131.123" ,27308 };
	ssh_exec_read(si);
	return 0;
}