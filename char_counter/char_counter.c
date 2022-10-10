#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


int main(){
	uint64_t parity[256];
	for (int i = 0; i < 256; ++i) parity[i] = 0;
	int ch;
	while ((ch = getchar()) != EOF){
		++parity[ch];
	}

	for (int i = 0; i < 256; ++i){
		//printf("%3d %3x: %ld\n", i, i, parity[i]);
		printf("%3d %ld\n", i, parity[i]);
	}

}
