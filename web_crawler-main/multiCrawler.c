#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/tidybuffio.h>
#include "crawl.h"
#include "queue.h"
#include <pthread.h>



// libtidy -> sudo apt-get install tidy-dev
// libcurl -> sudo apt-get install libcurl4-openssl-dev
// compilation -> gcc crawler.c -o crawler -lcurl -ltidy

#define MAX 10
#define MAX_URL_LENGTH 512

// curl write callback, to fill tidy's input buffer
uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out){
 	uint r;
  	r = size * nmemb;
  	tidyBufAppend(out, in, r);
  	return r;
}

// finds urls on webpage
void find_urls(TidyDoc doc, TidyNode tnod, queue *q){
  	TidyNode child;
  	// loop through children
  	for(child = tidyGetChild(tnod); child; child = tidyGetNext(child) ) {
    		// get href children
    		TidyAttr hrefAttr = tidyAttrGetById(child, TidyAttr_HREF);
    		if (hrefAttr && strlen(tidyAttrValue(hrefAttr)) < MAX_URL_LENGTH) {
        		char *str = (char *) calloc(MAX_URL_LENGTH + 1, sizeof(char));
      			strncpy(str, (char *)(tidyAttrValue(hrefAttr)), MAX_URL_LENGTH);
      			// final check
    			if(str[0] == 'h' && str[1] == 't' && str[2] == 't' && str[3] == 'p' && str[4] == 's'){
    				enqueue(str, q);	
        		}
        		else free(str);
      		}
		find_urls(doc, child, q); // go one level deeper
	}
}

// process url and grab its links, returns -3 if the crawl is already full, -2 on curl error, 
// -1 on tidy buffer error, and 0 on completion
int process_url(char *url, Crawl *c, queue *q){
	if(c->num >= MAX) return -3;
	
	CURL *curl;
    	TidyDoc tdoc;
    	TidyBuffer docbuf = {0};
    	TidyBuffer tidy_errbuf = {0};
    	int err;
	
	// setup curl
    	curl = curl_easy_init();
    	curl_easy_setopt(curl, CURLOPT_URL, url);
    	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, c->curlErr);
    	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

	// setop tidy buffer
    	tdoc = tidyCreate();
    	tidyOptSetBool(tdoc, TidyForceOutput, yes);
    	tidyOptSetInt(tdoc, TidyWrapLen, 4096);
    	tidyBufInit(&docbuf);
	
	// perform curl
    	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
    	err = curl_easy_perform(curl);

	// check if curl was successful, if not return -1
    	if(!err) {
    		// parse document and cleanup
      		err = tidyParseBuffer(tdoc, &docbuf);
      		if(err < 0) return -1;
        	err = tidyCleanAndRepair(tdoc);
          	if(err < 0) return -1;
            	find_urls(tdoc, tidyGetRoot(tdoc), q);
      	}
    	else{
		return -2;
	}

    	// cleanup curl and tidy buffer
    	curl_easy_cleanup(curl);
    	tidyBufFree(&docbuf);
    	tidyRelease(tdoc);
	
	
	// update crawl data structures and return success
	c->urls[c->num] = url;
	c->num++;
	return 0;
}

// initializes webcrawl with a given searchterm(url)
Crawl *init_crawl(char *searchTerm){
	Crawl *c = (Crawl *)malloc(sizeof(Crawl *));
	
	c->searchTerm = (char*) malloc(513 * sizeof(char));
	strncpy(c->searchTerm, searchTerm, 512);

	c->urls = (char **)calloc(10, sizeof(char *));	
	
	c->num = 0;
	c->status = 10;
	return c;
}

// writes data from crawl to terminal
void write_to_terminal(Crawl *c){
	if(c->status == 10){
		printf("Crawl either in progress or not started...\n");
		return;
	}
	
	printf("Search Term  : %s\n", c->searchTerm);
	
	// check for errors
	if(c->status == -2){
		printf("Curl Error   : %s\n", c->curlErr);
	}
	else if(c->status == -1){
		printf("Tidy Buffer Error\n");
	}
	// print data
	else{
		printf("URL's(MAX 10): \n");
		for(int i = 0; i < c->num; i++){
			printf("%-2d           : %s\n", i + 1, c->urls[i]);
		}
	}
}

// free data
void free_crawl(Crawl *c){
	for(int i = 0; i < c->num; i++){
		free(c->urls[i]);
	}
	free(c->urls);
	free(c);
}

// loop for web crawl
void *loop(void *arg){
	Crawl* c = (Crawl *) arg;
	// initialize queue for handling urls
	queue *q = (queue *)malloc(sizeof(queue));
    	initQueue(q);
	enqueue(c->searchTerm, q);
	
	// initalize status flag
	int flag;
	
	// feedback loop, keep scanning webpages while we can find urls and we haven't exceeded the max number of urls
	while(c->num < MAX && isEmpty(q) != 1){
		flag = process_url(dequeue(q), c, q);
	}
	
	c->status = flag;
	
	// free queue and return flag
	while(isEmpty(q) != 1){
		free(dequeue(q));
	}
	free(q);
}

void main(void){
	// get urls from data.txt file
	char **strArr = (char **)calloc(100,sizeof(char *));
	FILE *fp = fopen("data.txt", "r");
	int num = 0;
	char c = fgetc(fp);
	// iterate through characters
	while(c != EOF && num < 100){
		char *str = (char *)malloc(513*sizeof(char));
		int j = 0;
		// iterate until c == '\n' or j >= 512
		while(c != 10 && j < 512){
			str[j] = c;
			j++;
			c = fgetc(fp);
		}
		// add url to array
		strArr[num] = str;
		num++;		
		c = fgetc(fp);
	}
	fclose(fp);
	
	// initalize crawlers
	Crawl **cArr = (Crawl **)calloc(100, sizeof(Crawl *));
	for(int i = 0; i < num; i++){
		cArr[i] = init_crawl(strArr[i]);
	}
	
	// create threads
	pthread_t threads[num];
	for(int i = 0; i < num; i++){
		pthread_create(&threads[i], NULL, loop, (void *)cArr[i]);
	}
	// join threads
	for(int i = 0; i < num; i++){
		pthread_join(threads[i], NULL);
	}

	system("clear"); // helps when printing data
	
	// print data
	for(int i = 0; i < num; i++){
		write_to_terminal(cArr[i]);
		printf("\n");
	}

	// free crawls and stuff
	for(int i = 0; i < num; i++){
		free(strArr[i]);
		free_crawl(cArr[i]);
	}
	free(strArr);
	free(cArr);
}