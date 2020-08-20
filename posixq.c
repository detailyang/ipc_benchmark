#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <mqueue.h>

double timediff(struct timeval *begin, struct timeval *end)
{
  return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
         (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

int PosixQ(int size, int count)
{

#define SERVER_QUEUE_NAME "/mq-test"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10

  char           errMsg[64] = {0};
  int            i, sum;
  char *         buf;
  struct timeval begin, end;
  pid_t          pid;
  int            status;
  mqd_t          qd_server; 
  struct mq_attr attr;

  buf = malloc(size);
  if (buf == NULL)
  {
    perror("malloc");
    return 1;
  }

  memset(buf, 0x55, size);

  attr.mq_flags   = 0;
  attr.mq_maxmsg  = MAX_MESSAGES;
  attr.mq_msgsize = size;
  attr.mq_curmsgs = 0;

  pid = fork();
  if (pid < 0)
  {
    perror("Fork failed");
    exit(-1);
  }
  else if (pid == 0)
  {
    if ((qd_server =
           mq_open(SERVER_QUEUE_NAME, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS,
                   &attr)) == -1)
    {
      perror("Server: mq_open (server)");
      exit(1);
    }
    sum = 0;
    for (i = 0; i < count; i++)
    {
      if (mq_receive(qd_server, buf, size, NULL) == -1)
      {
        perror("Server: mq_receive");
      }

      sum += size;
    }

    if (mq_close(qd_server) == -1)
    {
      perror("Client: mq_close");
    }

    mq_unlink(SERVER_QUEUE_NAME);

    if (sum != count * size)
    {
      fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
      exit(-1);
    }
    exit(1);
  }
  else
  {
    if ((qd_server =
           mq_open(SERVER_QUEUE_NAME, O_CREAT | O_WRONLY, QUEUE_PERMISSIONS,
                   &attr)) == -1)
    {
      perror("Server: mq_open (server)");
      exit(1);
    }
    gettimeofday(&begin, NULL);

    for (i = 0; i < count; i++)
    {
      if (mq_send(qd_server, buf, size, 0) == -1)
      {
        sprintf(errMsg, "Server Not able to send message # %d to client", i);
        perror(errMsg);
        continue;
      }
    }
    gettimeofday(&end, NULL);

    mq_close(qd_server);

    mq_unlink(SERVER_QUEUE_NAME);


    waitpid(pid, &status, WUNTRACED | WCONTINUED);
    double tm = timediff(&begin, &end);
    printf("%3.0fMB/s %6.0fmsg/s\n\n",
           (count * size * 1.0 / (tm * 1024 * 1024)),
           (count * 1.0 / tm));
  }
  return status;
}

int main(int argc, char *argv[])
{
  int size;
  int count;

  if (argc != 3)
  {
    printf("usage: ./posixq <size> <count>\n");
    return 1;
  }

  size  = atoi(argv[1]);
  count = atoi(argv[2]);

  PosixQ(size, count);
}
