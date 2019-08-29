#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>

void sem_init(int sem_id, int sem_num, int init_valve)
{
    union semun sem_union;
    sem_union.val = init_valve;
    if (semctl(sem_id, sem_num, SETVAL, sem_union))
    {
        perror("semctl");
        exit(-1);
    }
}

void sem_release(int sem_id, int sem_num)
{
    struct sembuf sem_b;
    sem_b.sem_num = sem_num;
    sem_b.sem_op = 1;
    sem_b.sem_flg = 0;
    if (semop(sem_id, &sem_b, 1) == -1)
    {
        perror("sem_release");
        exit(-1);
    }
}

void sem_reserve(int sem_id, int sem_num)
{
    struct sembuf sem_b;
    sem_b.sem_num = sem_num;
    sem_b.sem_op = -1;
    sem_b.sem_flg = 0;
    if (semop(sem_id, &sem_b, 1) == -1)
    {
        perror("sem_reserve");
        exit(-1);
    }
}

double getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

int main(int argc, char const *argv[])
{
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
#define WRITE_SEM 0
#define READ_SEM 1

    pid_t pid;
    int sem_id;
    int shm_id;
    int count, i, size;
    struct timeval begin, end;

    if (argc != 3)
    {
        printf("usage: ./shm <size> <count>\n");
        return 1;
    }

    size = atoi(argv[1]);
    count = atoi(argv[2]);
    unsigned char *buf = malloc(size);

    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return -1;
    }
    else if (pid == 0) // parent
    {
        sem_id = semget(SEM_KEY, 2, 0600 | IPC_CREAT);
        if (sem_id == -1)
        {
            perror("parent: semget");
            return -1;
        }
        sem_init(sem_id, WRITE_SEM, 1);
        sem_init(sem_id, READ_SEM, 0);

        shm_id = shmget(SHM_KEY, size, IPC_CREAT | 0600);
        if (shm_id == -1)
        {
            perror("parent: shmget");
            return -1;
        }
        void *addr = shmat(shm_id, NULL, 0);
        if (addr == (void *)-1)
        {
            perror("parent: shmat");
            return -1;
        }

        for (i = 0; i < count; i++)
        {
            sem_reserve(sem_id, READ_SEM);
            memcpy(buf, addr, size);
            // printf(">>>>>>>>%d\n", *(int*)buf);
            sem_release(sem_id, WRITE_SEM);
        }

        if (shmdt(addr) == -1)
        {
            perror("child: shmdt");
            return -1;
        }
    }
    else // child
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
        void *addr = (int *)shmat(shm_id, NULL, 0);
        if (addr == (void *)-1)
        {
            perror("child: shmat");
            return -1;
        }

        gettimeofday(&begin, NULL);

        for (i = 0; i < count; i++)
        {
            sem_reserve(sem_id, WRITE_SEM);
            // *(int*)buf = i;
            memcpy(addr, buf, size);
            sem_release(sem_id, READ_SEM);
        }

        gettimeofday(&end, NULL);

        double tm = getdetlatimeofday(&begin, &end);
        printf("%fMB/s %fmsg/s %f\n",
               count * size * 1.0 / (tm * 1024 * 1024),
               count * 1.0 / tm, tm);

        sem_reserve(sem_id, WRITE_SEM);
        union semun dummy;
        semctl(sem_id, 0, IPC_RMID, dummy);
        if (shmdt(addr) == -1)
        {
            perror("shmdt");
            return -1;
        }
        shmctl(shm_id, IPC_RMID, 0);
    }

    return 0;
}
