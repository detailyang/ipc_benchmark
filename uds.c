#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

double
getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

int main(int argc, char *argv[]) {
    int                  fd, nfd;
    int                  i, size, count, sum, n;
    char                *buf;
    size_t               len;
    struct timeval       begin, end;
    struct sockaddr_un   un;

    if (argc != 3) {
        printf("usage: ./uds <size> <count>\n");
        return 1;
    }

    size = atoi(argv[1]);
    count = atoi(argv[2]);

    buf = malloc(size);
    if (buf == NULL) {
        perror("malloc");
        return 1;
    }

    if (fork() == 0) {
        fd = socket(AF_UNIX, SOCK_STREAM, 0);

        unlink("./uds-ipc");
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, "./uds-ipc");
        len = offsetof(struct sockaddr_un, sun_path) + strlen("./uds-ipc");
        if (bind(fd, (struct sockaddr *)&un, len) == -1) {
            perror("bind");
            return 1;
        }

        listen(fd, 1024);
        if ((nfd = accept(fd, NULL, NULL)) == -1) {
            perror("accept");
            return 1;
        }
        sum = 0;
        for (i = 0; i < count; i++) {
            n = read(nfd, buf, size);
            if (n == -1) {
                return 1;
            }

            sum += n;
        }

        if (sum != count * size) {
            return 1;
        }

    } else {
        sleep(1);

        fd = socket(AF_UNIX, SOCK_STREAM, 0);

        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, "./uds-ipc");
        len = offsetof(struct sockaddr_un, sun_path) + strlen("./uds-ipc");
        nfd = connect(fd,  (struct sockaddr *)&un, len);
        if (fd == -1) {
            perror("connect");
            return 1;
        }

        gettimeofday(&begin, NULL);

        for (i = 0; i < count; i++) {
            if (write(nfd, buf, size) != size) {
                perror("wirte");
                return 1;
            }
        }

        gettimeofday(&end, NULL);

        printf("%.0fMb/s %.0fmsg/s\n",
            (count * size * 1.0 / getdetlatimeofday(&begin, &end)) * 8 / 1000000,
            (count * 1.0 / getdetlatimeofday(&begin, &end)));
    }

    return 0;
}
