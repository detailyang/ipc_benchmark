
// all test combined. Easier for ARM embedded target.
// Rather modify all the other files 
// I just pulled them in here and did some 
// cleanup and turned them into function calls.


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <mqueue.h>

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
#define WRITE_SEM 0
#define READ_SEM 1

typedef union
{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
  struct seminfo *__buf;
  void *__pad;
} semun;

void InitSema4(int sem_id, int sem_num, int init_valve)
{
  semun sem_union;
  sem_union.val = init_valve;
  if (semctl(sem_id, sem_num, SETVAL, sem_union))
  {
    perror("semctl");
    exit(-1);
  }
}

void Sema4release(int sem_id, int sem_num)
{
  struct sembuf sem_b;
  sem_b.sem_num = sem_num;
  sem_b.sem_op  = 1;
  sem_b.sem_flg = 0;
  if (semop(sem_id, &sem_b, 1) == -1)
  {
    perror("Sema4release");
    exit(-1);
  }
}

void Sema4Lock(int sem_id, int sem_num)
{
  struct sembuf sem_b;
  sem_b.sem_num = sem_num;
  sem_b.sem_op  = -1;
  sem_b.sem_flg = 0;
  if (semop(sem_id, &sem_b, 1) == -1)
  {
    perror("Sema4Lock");
    exit(-1);
  }
}

double timediff(struct timeval *begin, struct timeval *end)
{
  return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
         (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}



// ***************************************************************************
//
// Unit Name    : 
//
// \Brief     	: 
//                
//
// \Param       :
//
// Return type	: N/A
//
// Limitations  : 
//
// Notes        : 
//
// ***************************************************************************
int ShrMem(int size, int count)
{

  pid_t          pid;
  int            sem_id;
  int            shm_id;
  int            i;
  int            status;
  struct timeval begin, end;

  unsigned char *buf = malloc(size);

  pid = fork();
  if (pid == 0)   // child
  {
    sem_id = semget(SEM_KEY, 2, 0600 | IPC_CREAT);
    if (sem_id == -1)
    {
      perror("parent: semget");
      return -1;
    }
    InitSema4(sem_id, WRITE_SEM, 1);
    InitSema4(sem_id, READ_SEM, 0);

    shm_id = shmget(SHM_KEY, size, IPC_CREAT | 0600);
    if (shm_id == -1)
    {
      perror("parent: shmget");
      return -1;
    }
    void *addr = shmat(shm_id, NULL, 0);
    if (addr == (void*)-1)
    {
      perror("parent: shmat");
      return -1;
    }

    for (i = 0; i < count; i++)
    {
      Sema4Lock(sem_id, READ_SEM);
      memcpy(buf, addr, size);
      Sema4release(sem_id, WRITE_SEM);
    }

    if (shmdt(addr) == -1)
    {
      perror("child: shmdt");
      return -1;
    }
    exit(1);
  }
  else   // parent
  {
    sleep(1);
    sem_id = semget(SEM_KEY, 0, 0);
    if (sem_id == -1)
    {
      perror("child: semget");
      return -1;
    }

    shm_id = shmget(SHM_KEY, 0, 0);
    if (shm_id == -1)
    {
      perror("child: shmget");
      return -1;
    }
    void *addr = (int*)shmat(shm_id, NULL, 0);
    if (addr == (void*)-1)
    {
      perror("child: shmat");
      return -1;
    }

    gettimeofday(&begin, NULL);

    for (i = 0; i < count; i++)
    {
      Sema4Lock(sem_id, WRITE_SEM);
      // *(int*)buf = i;
      memcpy(addr, buf, size);
      Sema4release(sem_id, READ_SEM);
    }

    gettimeofday(&end, NULL);

    waitpid(pid, &status, WEXITED);
    double tm = timediff(&begin, &end);
    printf("Shared Memory:\n%3.0f MB/s %6.0f msg/s\n\n",
           (count * size * 1.0 / (tm * 1024 * 1024)),
           (count * 1.0 / tm));

    Sema4Lock(sem_id, WRITE_SEM);
    semun dummy = {0};
    semctl(sem_id, 0, IPC_RMID, dummy);
    if (shmdt(addr) == -1)
    {
      perror("shmdt");
      return -1;
    }
    shmctl(shm_id, IPC_RMID, 0);
  }

  return status;
}



// ***************************************************************************
//
// Unit Name    : 
//
// \Brief     	: 
//                
//
// \Param       :
//
// Return type	: N/A
//
// Limitations  : 
//
// Notes        : 
//
// ***************************************************************************
int UDSocket(int size, int count)
{
    pid_t          pid;
    int fd, nfd;
    int i, sum, n;
  int            status;
    char *buf;
    size_t len;
    struct timeval begin, end;
    struct sockaddr_un un;


    buf = malloc(size);

    memset(&un, 0, sizeof(un));
    pid = fork();
    if (pid == 0)
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

        waitpid(pid, &status, WEXITED);
        double tm = timediff(&begin, &end);
        printf("Unix Domain Socket:\n %3.0f MB/s %6.0f msg/s\n\n",
               (count * size * 1.0 / (tm * 1024 * 1024)),
               (count * 1.0 / tm));
    }

  return status;
}





// ***************************************************************************
//
// Unit Name    : 
//
// \Brief     	: 
//                
//
// \Param       :
//
// Return type	: N/A
//
// Limitations  : 
//
// Notes        : 
//
// ***************************************************************************
int UDPSocket(int size, int count)
{
  int                fd, yes;
  int                i, sum, n;
  char *             buf;
  struct timeval     begin, end;
  struct sockaddr_in in;
  pid_t pid;
  int            status;

  buf   = malloc(size);

  memset(&in, 0, sizeof(in));
  pid = fork();
  if (pid == 0)
  {
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    in.sin_family = AF_INET;
    in.sin_port   = htons(55000);
    inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);

    yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(fd, (struct sockaddr *)&in, sizeof(in)) == -1)
    {
      perror("bind");
      exit(-1);
    }

    sum = 0;
    for (i = 0; i < count; i++)
    {
      n = recv(fd, buf, size, 0);
      if (n == 0)
      {
        break;
      }
      if (n == -1)
      {
        perror("recv");
      }
      sum += n;
    }
    if (sum != count * size)
    {
      fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
      close(fd);
      exit(-1);
    }
    close(fd);
    exit(1);
  }
  else
  {
    sleep(1);

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    in.sin_family = AF_INET;
    in.sin_port   = htons(55000);
    inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);

    gettimeofday(&begin, NULL);

    for (i = 0; i < count; i++)
    {
      if (sendto(fd, buf, size, 0, (struct sockaddr *)&in, sizeof(in)) != size)
      {
        perror("sendto");
      }
    }
    gettimeofday(&end, NULL);
    waitpid(pid, &status, WEXITED);
    double tm = timediff(&begin, &end);
    printf("UDP Socket:\n%3.0f MB/s %6.0f msg/s\n\n",
           (count * size * 1.0 / (tm * 1024 * 1024)),
           (count * 1.0 / tm));
  }
  return status;
}

// ***************************************************************************
//
// Unit Name    :
//
// \Brief       :
//
//
// \Param       :
//
// Return type	: N/A
//
// Limitations  :
//
// Notes        :
//
// ***************************************************************************
int TCPSocket(int size, int count)
{
  int                fd, nfd, yes;
  int                i, sum, n;
  unsigned char *    buf;
  struct timeval     begin, end;
  struct sockaddr_in in;
  pid_t pid;
  int            status;


  buf = malloc(size);

  memset(&in, 0, sizeof(in));
  pid = fork();
  if (pid == 0)
  {
    fd            = socket(AF_INET, SOCK_STREAM, 0);
    in.sin_family = AF_INET;
    in.sin_port   = htons(55001);
    inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);

    yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(fd, (struct sockaddr *)&in, sizeof(in)) == -1)
    {
      perror("bind");
      exit(-1);
    }

    if (listen(fd, 128) == -1)
    {
      perror("listen");
      exit(-1);
    }

    if ((nfd = accept(fd, NULL, NULL)) == -1)
    {
      perror("accept");
      exit(-1);
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
      }
      sum += n;
    }

    if (sum != count * size)
    {
      fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
      exit(-1);
    }
    exit(1);
  }
  else
  {
    sleep(1);

    fd            = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    in.sin_family = AF_INET;
    in.sin_port   = htons(55001);
    inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);
    if (connect(fd,  (struct sockaddr *)&in, sizeof(in)) == -1)
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

    waitpid(pid, &status, WEXITED);
    double tm = timediff(&begin, &end);
    printf("TCP Socket:\n%3.0f MB/s %6.0f msg/s\n\n",
           (count * size * 1.0 / (tm * 1024 * 1024)),
           (count * 1.0 / tm));
  }

  return status;
}

// ***************************************************************************
//
// Unit Name    :
//
// \Brief       :
//
//
// \Param       :
//
// Return type	: N/A
//
// Limitations  :
//
// Notes        :
//
// ***************************************************************************
int SockPair(int size, int count)
{
  int            sv[2];
  int            i, sum, n;
  char *         buf;
  struct timeval begin, end;
  pid_t pid;
  int            status;


  buf = malloc(size);
  if (buf == NULL)
  {
    perror("malloc");
    return 1;
  }


  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1)
  {
    perror("socketpair");
    return 1;
  }

  pid = fork();
  if (pid == 0)
  {
    sum = 0;
    for (i = 0; i < count; i++)
    {
      n = read(sv[0], buf, size);
      if (n == -1)
      {
        perror("read");
      }

      sum += n;
    }

    if (sum != count * size)
    {
      fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
      exit(-1);
    }
    exit(1);
  }
  else
  {
    gettimeofday(&begin, NULL);

    for (i = 0; i < count; i++)
    {
      if (write(sv[1], buf, size) != size)
      {
        perror("wirte");
      }
    }
    gettimeofday(&end, NULL);

    waitpid(pid, &status, WEXITED);
    double tm = timediff(&begin, &end);
    printf("Socket Pair:\n%3.0f MB/s %6.0f msg/s\n\n",
           (count * size * 1.0 / (tm * 1024 * 1024)),
           (count * 1.0 / tm));
  }

  return status;
}


// ***************************************************************************
//
// Unit Name    : 
//
// \Brief     	: 
//                
//
// \Param       :
//
// Return type	: N/A
//
// Limitations  : 
//
// Notes        : 
//
// ***************************************************************************
int PosixQ(int size, int count)
{

#define SERVER_QUEUE_NAME "/mq-test"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10

  char errMsg[64] ={0};
  int            i, sum;
  char *         buf;
  struct timeval begin, end;
  pid_t pid;
  int            status;
  mqd_t qd_server;
  struct mq_attr attr;

  buf = malloc(size);
  if (buf == NULL)
  {
    perror("malloc");
    return 1;
  }

  memset(buf, 0x55, size);

  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGES;
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
    if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_CREAT|O_RDONLY, QUEUE_PERMISSIONS, &attr)) == -1) 
    {
      perror ("Server: mq_open (server)");
      exit (1);
    }
    sum = 0;
    for (i = 0; i < count; i++)
    {
      if (mq_receive (qd_server, buf, size, NULL) == -1) 
      {
        perror ("Server: mq_receive");
      }

      sum += size;
    }
    
    if (mq_close (qd_server) == -1) 
    {
        perror ("Client: mq_close");
    }

    mq_unlink (SERVER_QUEUE_NAME);
    
    if (sum != count * size)
    {
      fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
      exit(-1);
    }
    exit(1);
  }
  else
  {
    if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_CREAT|O_WRONLY, QUEUE_PERMISSIONS, &attr)) == -1) 
    {
      perror ("Server: mq_open (server)");
      exit (1);
    }
    gettimeofday(&begin, NULL);

    for (i = 0; i < count; i++)
    {
      if (mq_send (qd_server, buf, size, 0) == -1) 
      {
        sprintf(errMsg, "Server Not able to send message # %d to client", i);
        perror (errMsg);
        continue;
      }
    }
    gettimeofday(&end, NULL);
    
    mq_close (qd_server); 

    mq_unlink (SERVER_QUEUE_NAME); 
   

    waitpid(pid, &status, WUNTRACED|WCONTINUED);
    double tm = timediff(&begin, &end);
    printf("Posix Q:\n%3.0f MB/s %6.0f msg/s\n\n",
           (count * size * 1.0 / (tm * 1024 * 1024)),
           (count * 1.0 / tm));
  }
  return status;
}

// ***************************************************************************
//
// Unit Name    :
//
// \Brief       :
//
//
// \Param       :
//
// Return type	: N/A
//
// Limitations  :
//
// Notes        :
//
// ***************************************************************************
int AnonPipe(int size, int count)
{
  int pipefd[2] = {0};

  int            i, sum, n;
  char *         buf;
  struct timeval begin, end;
  pid_t pid;
  int            status;


  buf = malloc(size);
  if (buf == NULL)
  {
    perror("malloc");
    return 1;
  }

  if (pipe(pipefd) == -1)
  {
    perror("pipe");
    return 1;
  }

  pid = fork();
  if (pid == 0)
  {
    sum = 0;
    for (i = 0; i < count; i++)
    {
      n = read(pipefd[0], buf, size);
      if (n == -1)
      {
        perror("read");
        return 1;
      }

      sum += n;
    }

    if (sum != count * size)
    {
      fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
      exit(-1);
    }
    exit(1);
  }
  else
  {
    gettimeofday(&begin, NULL);

    for (i = 0; i < count; i++)
    {
      if (write(pipefd[1], buf, size) != size)
      {
        perror("wirte");
        return 1;
      }
    }
    gettimeofday(&end, NULL);

    waitpid(pid, &status, WUNTRACED|WCONTINUED);
    double tm = timediff(&begin, &end);
    printf("Pipe:\n%3.0f MB/s %6.0f msg/s\n\n",
           (count * size * 1.0 / (tm * 1024 * 1024)),
           (count * 1.0 / tm));
  }
  return status;
}


// ***************************************************************************
//
// Unit Name    :
//
// \Brief       :
//
//
// \Param       :
//
// Return type	: N/A
//
// Limitations  :
//
// Notes        :
//
// ***************************************************************************
int NamedPipeFifo(int size, int count)
{
  int            fd;
  int            i, sum, n;
  char *         buf;
  struct timeval begin, end;
  pid_t pid;
  int            status;


  buf = malloc(size);
  if (buf == NULL)
  {
    perror("malloc");
    return 1;
  }

  unlink("./fifo-ipc");
  if (mkfifo("./fifo-ipc", 0700) == -1)
  {
    perror("mkfifo");
    return 1;
  }

  fd = open("./fifo-ipc", O_RDWR);
  if (fd == -1)
  {
    perror("open");
    return 1;
  }

  pid = fork();
  if (pid == 0)
  {
    sum = 0;
    for (i = 0; i < count; i++)
    {
      // 
      // block waiting for write
      //
      n = read(fd, buf, size);
      if (n == -1)
      {
        perror("read");
        kill(pid, SIGKILL);
      }
      sum += n;
    }

    if (sum != count * size)
    {
      fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
      exit(-1);
    }
    exit(1);

  }
  else
  {

    gettimeofday(&begin, NULL);

    for (i = 0; i < count; i++)
    {
      if (write(fd, buf, size) != size)
      {
        perror("Fifo write");
      }
    }
    gettimeofday(&end, NULL);

    waitpid(pid, &status, 0);
    double tm = timediff(&begin, &end);
    printf("Fifo:\n%3.0f MB/s %6.0f msg/s\n\n",
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
        printf("usage: ./ipc_bm <size> <count>\n");
        exit(-1);
    }

    size = atoi(argv[1]);
    count = atoi(argv[2]);
    
    PosixQ(size, count);

    ShrMem(size, count);

    NamedPipeFifo(size, count);

    AnonPipe(size, count);

    SockPair(size, count);

    TCPSocket(size, count);

    UDPSocket(size, count);

    exit(0);
}
