#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define SPP_FD_PROBE_SOCKET_PATH "/var/run/osaig_spp_fd_probe.sock"
#define SPP_FD_PROBE_TX "OSAIG_SPP_FD_PROBE_TX hello from glasses probe\n"

static void log_ascii_hex(const char *prefix, const unsigned char *data, ssize_t len)
{
	ssize_t i;

	printf("%s len=%zd ascii=\"", prefix, len);
	for (i = 0; i < len; i++) {
		unsigned char ch = data[i];
		putchar((ch >= 32 && ch <= 126) ? ch : '.');
	}
	printf("\" hex=");
	for (i = 0; i < len; i++) {
		printf("%02X", data[i]);
		if (i + 1 < len)
			putchar(' ');
	}
	putchar('\n');
	fflush(stdout);
}

static int recv_spp_fd(int client_fd)
{
	struct msghdr msg;
	struct iovec iov;
	char payload;
	char control[CMSG_SPACE(sizeof(int))];
	struct cmsghdr *cmsg;
	ssize_t received;
	int fd = -1;

	memset(&msg, 0, sizeof(msg));
	memset(control, 0, sizeof(control));
	iov.iov_base = &payload;
	iov.iov_len = sizeof(payload);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);

	received = recvmsg(client_fd, &msg, 0);
	if (received <= 0) {
		printf("[SPP_FD_PROBE] recvmsg failed received=%zd errno=%d\n",
		       received, errno);
		fflush(stdout);
		return -1;
	}

	cmsg = CMSG_FIRSTHDR(&msg);
	if (!cmsg || cmsg->cmsg_level != SOL_SOCKET ||
	    cmsg->cmsg_type != SCM_RIGHTS ||
	    cmsg->cmsg_len < CMSG_LEN(sizeof(int))) {
		printf("[SPP_FD_PROBE] missing SCM_RIGHTS fd\n");
		fflush(stdout);
		return -1;
	}

	memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
	printf("[SPP_FD_PROBE] received fd=%d\n", fd);
	fflush(stdout);
	return fd;
}

static void exercise_spp_fd(int spp_fd)
{
	ssize_t written;
	int i;

	written = write(spp_fd, SPP_FD_PROBE_TX, strlen(SPP_FD_PROBE_TX));
	printf("[SPP_FD_PROBE] write probe tx fd=%d written=%zd errno=%d\n",
	       spp_fd, written, written < 0 ? errno : 0);
	fflush(stdout);

	for (i = 0; i < 20; i++) {
		struct pollfd pfd;
		int ret;

		memset(&pfd, 0, sizeof(pfd));
		pfd.fd = spp_fd;
		pfd.events = POLLIN | POLLERR | POLLHUP;
		ret = poll(&pfd, 1, 1500);
		if (ret < 0) {
			printf("[SPP_FD_PROBE] poll failed errno=%d\n", errno);
			fflush(stdout);
			break;
		}
		if (ret == 0) {
			printf("[SPP_FD_PROBE] poll timeout index=%d\n", i);
			fflush(stdout);
			continue;
		}
		printf("[SPP_FD_PROBE] poll index=%d revents=0x%x\n", i,
		       pfd.revents);
		fflush(stdout);

		if (pfd.revents & POLLIN) {
			unsigned char buf[512];
			ssize_t len = recv(spp_fd, buf, sizeof(buf), MSG_DONTWAIT);
			if (len > 0)
				log_ascii_hex("[SPP_FD_PROBE] read", buf, len);
			else
				printf("[SPP_FD_PROBE] recv after poll len=%zd errno=%d\n",
				       len, errno);
			fflush(stdout);
		}

		if (pfd.revents & (POLLERR | POLLHUP))
			break;
	}
}

int main(void)
{
	int server_fd;
	struct sockaddr_un addr;

	signal(SIGPIPE, SIG_IGN);
	unlink(SPP_FD_PROBE_SOCKET_PATH);

	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("socket");
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
		 SPP_FD_PROBE_SOCKET_PATH);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(server_fd);
		return 1;
	}

	chmod(SPP_FD_PROBE_SOCKET_PATH, 0666);

	if (listen(server_fd, 4) < 0) {
		perror("listen");
		close(server_fd);
		unlink(SPP_FD_PROBE_SOCKET_PATH);
		return 1;
	}

	printf("[SPP_FD_PROBE] listening on %s\n", SPP_FD_PROBE_SOCKET_PATH);
	fflush(stdout);

	for (;;) {
		int client_fd = accept(server_fd, NULL, NULL);
		int spp_fd;

		if (client_fd < 0) {
			printf("[SPP_FD_PROBE] accept failed errno=%d\n", errno);
			fflush(stdout);
			continue;
		}

		spp_fd = recv_spp_fd(client_fd);
		close(client_fd);
		if (spp_fd < 0)
			continue;

		exercise_spp_fd(spp_fd);
		close(spp_fd);
		printf("[SPP_FD_PROBE] closed received fd\n");
		fflush(stdout);
	}
}
