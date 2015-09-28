#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define NUMBER_OF_THREADS 4
#define MAXWORD 1024
#define MAX_ITERATIONS 100000000

char *buffer = NULL;
FILE *infile;
int producer_done = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;
pthread_cond_t condc = PTHREAD_COND_INITIALIZER;

typedef struct dict {
	char *word;
	int count;
	struct dict *next;
} dict_t;

dict_t *wd = NULL;

char *make_word( char *word ) {
	return strcpy( malloc( strlen( word )+1 ), word );
}

dict_t *make_dict(char *word) {
	dict_t *nd = (dict_t *) malloc( sizeof(dict_t) );
	nd->word = make_word( word );
	nd->count = 1;
//	pthread_mutex_t tlock = PTHREAD_MUTEX_INITIALIZER;
	nd->next = NULL;
	return nd;
}

void *insert_word( dict_t *d, char *word ) {
//pthread_mutex_lock(&mutex);
	//   Insert word into dict or increment count if already there
	//   return pointer to the updated dict

	dict_t *nd;
	dict_t *pd = NULL;				// prior to insertion point 
	dict_t *di = d;					// following insertion point

	// Search down list to find if present or point of insertion
	while(di && ( strcmp(word, di->word ) >= 0) ) { 
		if( strcmp( word, di->word ) == 0 ) { 
			di->count++;			// increment count 
			return d;				// return head 
		}
		pd = di;					// advance ptr pair
		di = di->next;
	}
	nd = make_dict(word);			// not found, make entry 
	nd->next = di;					// entry bigger than word or tail 

	if (pd) {
		pd->next = nd;
		return d;					// insert beyond head 
	}
	//pthread_mutex_unlock(&mutex);
	return nd;

}

void print_dict(dict_t *d) {
	while (d) {
		printf("[%d] %s\n", d->count, d->word);
		d = d->next;
	}
}

int get_word( char *buf, int n, FILE *infile) {
	int inword = 0;
	int c;  
	while( (c = fgetc(infile)) != EOF ) {
		if (inword && !isalpha(c)) {
			buf[inword] = '\0';	// terminate the word string
			return 1;
		} 
		if (isalpha(c)) {
			buf[inword++] = c;
		}
	}
	return 0;			// no more words
}

void *words() {
printf("Thread Created.\n");
	char wordbuf[MAXWORD];
//pthread_mutex_lock(&mutex);
	while( get_word( wordbuf, MAXWORD, infile ) ) {

		pthread_mutex_lock(&mutex);
//		get_word (wordbuf, MAXWORD, infile);
		wd = insert_word(wd, wordbuf); // add to dict
//printf("Added %s\n", wordbuf);
printf("*");
		pthread_mutex_unlock(&mutex);
	}
//pthread_mutex_unlock(&mutex);
//	return wd;
}

void* producer(void *ptr){
	printf("Producer created.\n");
	char wordbuf[MAXWORD];
	int status = 1;
	int first_run = 1;
	while((status = get_word( wordbuf, MAXWORD, infile ) ) == 1) {
		pthread_mutex_lock(&mutex);
		buffer = wordbuf;
		printf("Found: %s, now waiting to clear.\n", buffer);
		if(first_run == 1){
			// if there was a consumer created before the producer, its asleep, wake it as we have a entry rdy now
			pthread_cond_broadcast(&condc);
			first_run = 0;
		}
		
		while(buffer != NULL)
			pthread_cond_wait(&condp, &mutex);
	//	buffer = wordbuf;
		printf("Returned to producer, last word %s. WAITING.\n", wordbuf);
		pthread_cond_broadcast(&condc);
		pthread_mutex_unlock(&mutex);
//		sleep(1);
	}
//	buffer = NULL;
printf("SET PRODUCER_DONE TO 1.\n");
	producer_done = 1;
	pthread_mutex_unlock(&mutex);
	printf("Exited Producer.\n");
//	sleep(2);
	printf("Buffer contains: %s\n", buffer);
	pthread_cond_broadcast(&condc);
	pthread_exit(0);
}

void* consumer(void *ptr){
	printf("Consumer created.\n");
	for(;;){

		pthread_mutex_lock(&mutex);
		while (buffer == NULL && producer_done == 0){
			printf("BUFFER contains: %s. waiting\n", buffer);
			pthread_cond_wait(&condc, &mutex);
			printf("BUFFER now contains %s\n", buffer);
			
		}
		if (producer_done == 1){
			printf("Finishing consumer !!!!.\n");
			pthread_mutex_unlock(&mutex);
			pthread_exit(0);
		}
		wd = insert_word(wd, buffer);
		printf("Consumed %s. WAITING.\n", buffer);
		buffer = NULL;
		pthread_cond_signal(&condp);
		pthread_mutex_unlock(&mutex);

	}
	printf("Exited Consumer.\n");
	pthread_exit(0);
	
}



int main( int argc, char *argv[] ) {

	pthread_t consumer_threads[NUMBER_OF_THREADS];

	int status;

	dict_t *d = NULL;
	infile = stdin;
	if (argc >= 2) {
		infile = fopen (argv[1],"r");
	}
	if( !infile ) {
		printf("Unable to open %s\n",argv[1]);
		exit( EXIT_FAILURE );
	}

	pthread_t producer_thread;
	status = pthread_create(&producer_thread, NULL, producer, NULL);
	
	for (int i = 0; i < NUMBER_OF_THREADS; i++){
		status = pthread_create(&consumer_threads[i], NULL, consumer, NULL);
	}

	pthread_join(producer_thread, NULL);
	for (int j = 0; j < NUMBER_OF_THREADS; j++){
		pthread_join(consumer_threads[j], NULL);
	}

	print_dict( wd );
	fclose( infile );
}

