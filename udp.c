#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

double
getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

int main(int argc, char *argv[]) {
    int                  fd, yes;
    int                  i, size, count, sum, n;
    char                *buf;
    struct timeval       begin, end;
    struct sockaddr_in   in;

    if (argc != 3) {
        printf("usage: ./udp <size> <count>\n");
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
        fd = socket(AF_INET, SOCK_DGRAM, 0);

        in.sin_family = AF_INET;
        in.sin_port = htons(15000);
        inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);

        if (bind(fd, (struct sockaddr *)&in, sizeof(in)) == -1) {
            perror("bind");
            return 1;
        }

        yes = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        sum = 0;
        for (i = 0; i < count; i++) {
            n = recv(fd, buf, size, 0);
            if (n == -1) {
                perror("recv");
                return 1;
            }

            sum += n;
        }

        if (sum != count * size) {
            return 1;
        }

    } else {
        sleep(1);

        fd = socket(AF_INET, SOCK_DGRAM, 0);

        in.sin_family = AF_INET;
        in.sin_port = htons(15000);
        inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);

        gettimeofday(&begin, NULL);

        for (i = 0; i < count; i++) {
            if (sendto(fd, buf, size, 0, (struct sockaddr *)&in, sizeof(in)) != size) {
                perror("sendto");
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
