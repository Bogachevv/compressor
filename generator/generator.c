#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv){
	int iter_c = 10;
	if (argc > 1){
		iter_c = atoi(argv[1]);
	}
	srand(time(NULL));
	int zero_c = 0;
	for (int i = 0; i < iter_c; ++i){
		int r = rand() % 256;
		if (r <= 64) r = 0;
		fputc(r, stdout);
	}
}
