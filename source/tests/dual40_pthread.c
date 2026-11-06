#include <pthread.h>
#include <stdint.h>
#include <assert.h>
static pthread_t thread;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wake=PTHREAD_COND_INITIALIZER,finished=PTHREAD_COND_INITIALIZER;
static void (*function)(void*);static void *argument;
static unsigned started,pending,done,available=1,fail_next,fault;
static void *worker(void *unused){(void)unused;pthread_mutex_lock(&mutex);for(;;){while(!pending)pthread_cond_wait(&wake,&mutex);void(*f)(void*)=function;void*a=argument;pthread_mutex_unlock(&mutex);f(a);pthread_mutex_lock(&mutex);pending=0;done=1;pthread_cond_signal(&finished);}return 0;}
int dc_submit(void(*f)(void*),void*a){if(!available||fault)return 0;pthread_mutex_lock(&mutex);if(!started){assert(!pthread_create(&thread,0,worker,0));started=1;}assert(!pending);function=f;argument=a;done=0;pending=1;pthread_cond_signal(&wake);pthread_mutex_unlock(&mutex);return 1;}
int dc_wait(void){pthread_mutex_lock(&mutex);while(!done)pthread_cond_wait(&finished,&mutex);pthread_mutex_unlock(&mutex);if(fail_next){fault=1;fail_next=0;return 0;}return 1;}
unsigned dc_status(void){return fault?3:available?2:1;}
unsigned dc_stage(void){return 3;}
unsigned dc_fault(void){return fault;}
void test_worker_available(unsigned on){available=on;}
void test_worker_fail(void){fail_next=1;}
