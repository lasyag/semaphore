#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <pthread.h>

#define Block_size 1000

struct structure
{
	int freelist[100];
	int list1[100];
	int list2[100];
};

struct structure shm;

sem_t *sem_free, *sem_list1, *sem_list2, *count_free, *count_list1, *count_list2, *l1_con;

void *thread1()
{
	while(1)
	{
		sem_wait(l1_con);
		sem_wait(count_free);
		sem_wait(sem_free);
		int i,*b;
		for(i=0;i<100;i++)
		{
			if(shm.freelist[i]!=0)
			{
				b=shm.freelist[i];
				//printf("Thread 1 Block unlinked from freelist %d\n", *b);
				shm.freelist[i]=0;
				break;
			}
		}
		sem_post(sem_free);
		*b = (rand() % 32) + 18;
		printf("Producing information in block b = %d\n", *b);
		sem_wait(sem_list1);
		for(i=0;i<100;i++)
		{
			if(shm.list1[i]==0)
			{
				shm.list1[i]=b;
				//printf("Thread 1 Block in list1 %d\n", b);
				break;
			}
		}
		sem_post(sem_list1);
		sem_post(count_list1);
		usleep(100);
	}
}

void *thread2()
{
		while(1)
	{
		sem_wait(count_list1);
		sem_wait(sem_list1);
		int i,*x, *y;
		for(i=0;i<100;i++)
		{
			if(shm.list1[i]!=0)
			{
				x=shm.list1[i];
				//printf("Thread 2 Block unlinked from list1 %d\n", *x);
				shm.list1[i]=0;
				break;
			}
		}
		sem_post(sem_list1);
		sem_wait(count_free);
		sem_wait(sem_free);
		for(i=0;i<100;i++)
		{
			if(shm.freelist[i]!=0)
			{
				y=shm.freelist[i];
				//printf("Thread 2 Block unlinked from freelist %d\n", *y);
				shm.freelist[i]=0;
				break;
			}
		}
		sem_post(sem_free);
		sem_post(l1_con);
		*y = *x + (rand() % 10);
		printf("Used information in block x = %d and produced information in block y = %d\n", *x, *y);
		*x = 0;
		sem_wait(sem_free);
		for(i=0;i<100;i++)
		{
			if(shm.freelist[i]==0)
			{
				shm.freelist[i]=x;
				//printf("Thread 2 Block inserted in freelist:  %d\n", x);
				break;
			}
		}
		sem_post(sem_free);
		sem_post(count_free);
		sem_wait(sem_list2);
		for(i=0;i<100;i++)
		{
			if(shm.list2[i]==0)
			{
				shm.list2[i]=y;
				//printf("Thread 2 Block inserted in list2:  %d\n", y);
				break;
			}
		}
		sem_post(sem_list2);
		sem_post(count_list2);
		usleep(100);
	}
}

void *thread3()
{
	while(1)
	{	
		sem_wait(count_list2);
		sem_wait(sem_list2);
		int i,*c;
		for(i=0;i<100;i++)
		{
			if(shm.list2[i]!=0)
			{
				c=shm.list2[i];
				//printf("Thread 3 Block unlinked from list2 %d\n", *c);
				shm.list2[i]=0;
				break;
			}
		}
		sem_post(sem_list2);
		printf("Used information in block c = %d\n", *c);
		*c = 0;
		sem_wait(sem_free);
		for(i=0;i<100;i++)
		{
			if(shm.freelist[i]==0)
			{
				shm.freelist[i]=c;
				//printf("Thread 3 Block inserted in freelist:  %d\n", c);
				break;
			}
		}
		sem_post(sem_free);
		sem_post(count_free);
		usleep(100);
	}
	
}


int main()
{
	int i, ret_val;
	pthread_t thread[3];
	int *mem_block;

	sem_free = sem_open ("sem1", O_CREAT | O_EXCL, 0644, 1);
	sem_list1 = sem_open ("sem2", O_CREAT | O_EXCL, 0644, 1);
	sem_list2 = sem_open ("sem3", O_CREAT | O_EXCL, 0644, 1);
	count_free = sem_open ("sem4", O_CREAT | O_EXCL, 0644, 100);
	count_list1 = sem_open ("sem5", O_CREAT | O_EXCL, 0644, 0);
	count_list2 = sem_open ("sem6", O_CREAT | O_EXCL, 0644, 0);
	l1_con = sem_open("sem7", O_CREAT | O_EXCL, 0644, 99);

	sem_unlink ("sem1");
	sem_unlink ("sem2");
	sem_unlink ("sem3");
	sem_unlink ("sem4");
	sem_unlink ("sem5");
	sem_unlink ("sem6");
	sem_unlink ("sem7");

	mem_block = (int *) malloc (400);

	for(i=0;i<100;i++)
	{
		shm.list1[i]=0;
		shm.list2[i]=0;	
	}	

	for(i=0;i<100;i++)
	{
		mem_block[i] = 0;
	}

	for(i=0;i<100;i++)
	{
		shm.freelist[i] = &mem_block[i];
		//printf("MEM BLOCK ADDR %d VALUE %d\n", &mem_block[i],mem_block[i]);
	}

	ret_val = pthread_create(&thread[0], NULL, thread1, NULL);
	if(ret_val)
	{
		printf("Error code %d\n", ret_val);
		exit(-1);
	}

	ret_val = pthread_create(&thread[1], NULL, thread2, NULL);
	if(ret_val)
	{
		printf("Error code %d\n", ret_val);
		exit(-1);
	}

	ret_val = pthread_create(&thread[2], NULL, thread3, NULL);
	if(ret_val)
	{
		printf("Error code %d\n", ret_val);
		exit(-1);
	}
	
	pthread_exit(NULL);
}