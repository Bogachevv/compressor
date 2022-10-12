#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv){
	if (argc <= 2){
		printf("Not enough args\n");
		exit(123);
	}
	FILE *first = fopen(argv[1], "rb");
	FILE *second = fopen(argv[2], "rb");

	int ch1, ch2;
	for (long long i = 1; ; ++i){
		if (feof(first) != feof(second)){
			printf("Files differ in %lld pos\n", i);
			return 1;
		}
		else if (feof(first) && feof(second)) return 0;
		ch1 = fgetc(first);
		ch2 = fgetc(second);
		if (ch1 == ch2) continue;
		if (ch1 != ch2){
			printf("Files differ in %lld pos\n", i);
			return 1;
		}
	}

}

