/*
 此DEMO用于展示共享内存
 共享内存使用，ftok,shmget,shmat,shmdt等
 进程级互斥变量使用 pthread_mutex_t使用，在初始化此变量时候采用动态初始化，将pthread_mutexattr_t属性初始化时候指定为pthread_mutex_t;
Note: 在shmget时候不需要加上IPC_EXCL，该变量会在shmkey存在的情况下返回失败,无法获得shmid，因此多进程同时使用该进程时候会出现Segment Fault
Note2: 互斥变量熟悉pthread_mutexattr_t也需要在共享内存进行分配，否则进程互斥锁无法使用
Problem:pthread_mutex_setpshared()函数居然用man 3查不到，回头看看怎么安装文档
*/
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<string.h>
#include<errno.h>
#include<pthread.h>

#define p(val) printf("step %d",val)
int main(){
	p(0);
	key_t key = ftok("./key_file_2",10);
	if(key == -1){
		printf("ftok failed:%s\n",strerror(errno));
		return -1;
	}	
	p(1);
	int shmid = shmget(key,1024*1024,IPC_CREAT|0666/*|IPC_EXCL*/);
	int isExist = 0;
	if(-1 == shmid && EEXIST != errno){
		if(EEXIST == errno)
			isExist = 1;
		else{
			printf("shmget failed:%s\n",strerror(errno));
			return -1;
		}
	}
	char * shmptr = (char*)shmat(shmid,NULL,0);
	if((int)*shmptr == -1){
		printf("shmat failed:%s",strerror(errno));
		return -1;	
	}
	p(2);
	pthread_mutex_t * mutex = (pthread_mutex_t*)(shmptr);
	//pthread_mutexattr_t * mutex_attr = (pthread_mutexattr_t*)(shmptr+sizeof(pthread_mutex_t));
	//pthread_mutexattr_t mutex_attr;
	int pret;
	//struct pthread_mutex_t mutex;
	if(!isExist){
		//pthread_mutex_t * mutex = (pthread_mutex_t*)(shmptr);
		pthread_mutexattr_t mutex_attr;
		pthread_mutexattr_init(&mutex_attr);
		pret = pthread_mutexattr_setpshared(&mutex_attr,PTHREAD_PROCESS_SHARED);
		//	pret = pthread_mutex_setpshared(mutex,PTHREAD_PROCESS_SHARED);
		if(0 != pret){
			printf("pthread_mutex_setpshared failed:%s\n",strerror(errno));
			return -1;
		}

		pret = pthread_mutex_init(mutex,&mutex_attr);
		if(0 != pret){
			printf("pthread_init failed:%s\n",strerror(errno));
			return -1;
		}
	}	
	int * dest = (int*)(shmptr+ sizeof(pthread_mutex_t) + sizeof(pthread_mutex_t));
	//	*dest = 0;
	while(1){
		pret = pthread_mutex_lock(mutex);
		if(0 != pret){
			printf("pthread_mutex_lock failed:%s\n",strerror(errno));
			return -1;
		}
		//*shmptr='a';
		//*shmptr = 0;
		//char * dest = shmptr + 1 + sizeof(pthread_mutex_t);
		*dest = *dest + 1;
		printf("get %d\n",*dest);	
		*dest = *dest - 1;
		pret = pthread_mutex_unlock(mutex);
		if(0 != pret){
			printf("pthread_mutex_unlock failed:%s\n",strerror(errno));
			return -1;
		}
	}
	int dret = shmdt(shmptr);
	if(-1 == dret){
		printf("shmdt failed:%s",strerror(errno));
	}
	return 0;
}
