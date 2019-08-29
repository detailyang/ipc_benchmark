#include <stdio.h>
#include <string.h>
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

int main(int argc, char *argv[])
{
    int fd, nfd;
    int i, size, count, sum, n;
    char *buf;
    size_t len;
    struct timeval begin, end;
    struct sockaddr_un un;

    if (argc != 3)
    {
        printf("usage: ./uds <size> <count>\n");
        return 1;
    }

    size = atoi(argv[1]);
    count = atoi(argv[2]);
    buf = malloc(size);

    memset(&un, 0, sizeof(un));
    if (fork() == 0)
    {
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        unlink("./uds-ipc");
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, "./uds-ipc");
        len = offsetof(struct sockaddr_un, sun_path) + strlen("./uds-ipc");

        if (bind(fd, (struct sockaddr *)&un, len) == -1)
        {
            perror("bind");
            return 1;
        }

        if (listen(fd, 128) == -1)
        {
            perror("listen");
            return 1;
        }

        if ((nfd = accept(fd, NULL, NULL)) == -1)
        {
            perror("accept");
            return 1;
        }
        sum = 0;
        for (;;)
        {
            n = read(nfd, buf, size);
            if (n == 0)
            {
                break;
            }
            else if (n == -1)
            {
                perror("read");
                return 1;
            }
            sum += n;
        }

        if (sum != count * size)
        {
            fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
            return 1;
        }
    }
    else
    {
        sleep(1);

        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, "./uds-ipc");
        len = offsetof(struct sockaddr_un, sun_path) + strlen("./uds-ipc");
        if (connect(fd, (struct sockaddr *)&un, len) == -1)
        {
            perror("connect");
            return 1;
        }

        gettimeofday(&begin, NULL);

        for (i = 0; i < count; i++)
        {
            if (write(fd, buf, size) != size)
            {
                perror("wirte");
                return 1;
            }
        }

        gettimeofday(&end, NULL);

        double tm = getdetlatimeofday(&begin, &end);
        printf("%.0fMB/s %.0fmsg/s\n",
               count * size * 1.0 / (tm * 1024 * 1024),
               count * 1.0 / tm);
    }

    return 0;
}
