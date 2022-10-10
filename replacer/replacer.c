#include <stdio.h>
#include <stdlib.h>

int main(){
	int tar = 0;
	int repl = 'A';
	int ch;
	while ((ch = getchar()) != EOF){
		if (ch == tar) ch = repl;
		putchar(ch);
	}
}
