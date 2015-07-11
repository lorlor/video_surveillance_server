#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <pthread.h>

using namespace std;

queue<string> cache_q;
char buf[32];
int count = 0;
int cond = 1;
pthread_mutex_t mutex;
fstream file;

void *w_thr(void *arg)
{
for(;;){
	sprintf(buf, "./cache/cache_%d", count);
	cache_q.push(buf);
	pthread_mutex_lock(&mutex);

	file.open(buf, fstream::in | fstream::out);
	file << count << endl;
	file.close();

	pthread_mutex_unlock(&mutex);

	cout << "Count is " << count << endl;
	count++;
	if(count == 10)
		count = 0;
	sleep(1);
}
	return ((void *)1);
}

void *r_thr(void *arg)
{
	int ctrl = 1;
	string tmp;
	char ch;
	while(ctrl){
		if(cache_q.empty())
			ctrl = 0;

		tmp = cache_q.front();
		cache_q.pop();

		pthread_mutex_lock(&mutex);
		file.open(tmp.c_str(), fstream::in | fstream::out);
		file.read(&ch, 1);
		cout << "What is read : " << ch << endl;
		file.close();
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}

	return ((void *)1);
}

void handler(int a)
{
	cond = 0;
	cout << "Cond is " << cond << endl;
}

int main()
{
	signal(SIGINT, handler);
	pthread_t w_id, r_id;
	int w_ret, r_ret;
	pthread_mutex_init(&mutex, NULL);
	w_ret = pthread_create(&w_id, NULL, w_thr, NULL);
	if(w_ret != 0){
		perror("write thread");
		exit(-1);
	}
	r_ret = pthread_create(&r_id, NULL, r_thr, NULL);
	if(r_ret != 0){
		perror("read thread");
		exit(-1);
	}
/*
	r_ret = pthread_join(r_id, NULL);
	if(r_ret != 0){
		perror("r_th join");
		exit(-1);
	}

	w_ret = pthread_join(w_id, NULL);
	if(w_ret != 0){
		perror("w_th join");
		exit(-1);
	}
*/
	while(cond);
	return 0;
}
