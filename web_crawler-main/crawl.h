typedef struct Crawl{
	char *searchTerm;
	char **urls;
	char curlErr[256];
	int num;
	int status;
}Crawl;
