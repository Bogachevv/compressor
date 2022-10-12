#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv){
	int prefix_len = 100;
	FILE *fin = stdin, *fout = stdout;
	if (argc > 1){
		prefix_len = atoi(argv[1]);
	}
	if (argc > 2){
		fin = fopen(argv[2], "rb");
	}
	if (argc > 3){
		fout = fopen(argv[3], "wb");
	}
	if (!fin || !fout){
		fprintf(stderr, "Can't open file\n");
		exit(1);
	}

	
	printf("Prefix len = %d\n", prefix_len);

	int ch;
	for (int i = 0; (i < prefix_len) && ((ch = fgetc(fin)) != EOF); ++i) fputc(ch, fout);

	fclose(fin);
	fclose(fout);
}
