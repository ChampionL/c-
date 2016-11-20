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
	pthread_mutexattr_t * mutex_attr = (pthread_mutexattr_t*)(shmptr+sizeof(pthread_mutex_t));
	//pthread_mutexattr_t mutex_attr;
	int pret;
	//struct pthread_mutex_t mutex;
	if(!isExist){
		//pthread_mutex_t * mutex = (pthread_mutex_t*)(shmptr);
		//pthread_mutexattr_t mutex_attr;
		pthread_mutexattr_init(mutex_attr);
		pret = pthread_mutexattr_setpshared(mutex_attr,PTHREAD_PROCESS_SHARED);
		//	pret = pthread_mutex_setpshared(mutex,PTHREAD_PROCESS_SHARED);
		if(0 != pret){
			printf("pthread_mutex_setpshared failed:%s\n",strerror(errno));
			return -1;
		}

		pret = pthread_mutex_init(mutex,mutex_attr);
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
