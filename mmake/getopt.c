#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]){

	int opt;

	while((opt = getopt(argc, argv, "abf:")) != -1){
		switch(opt){
			case 'f': 
				printf("Flagga f hittad!\n");
				break;
			case 'a':
				printf("Flagga a hittad!\n");
				break;
			case 'b':
				printf("Flagga b hittad!\n");
				break;
			case '?':
				printf("Okänd flagga! -%c\n", optopt);
				break;
		}
	}

	if (optind < argc) {
    	printf("Övriga argument:\n");
    	for (int i = optind; i < argc; i++) {
       	 	printf("  %s\n", argv[i]);
   		}
	}

	return 0;
}