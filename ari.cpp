#include <cstdlib>
#include <cstdio>

#include "ari.h"

#define MAX_BYTE 255
#define WORD_SIZE 4294967295

typedef unsigned long long int64;


int64* build_table(char* ifile){
    auto table = new int64[MAX_BYTE + 1];
    for (int64 i = 0; i <= MAX_BYTE; ++i) table[i] = 0;

    FILE* ifp = fopen(ifile, "rb");
    int ch;
    while ((ch = fgetc(ifp)) != EOF){
        table[ch] += 1;
    }
    fclose(ifp);

    for (int64 i = 1; i <= MAX_BYTE; ++i) table[i] += table[i - 1];

    return table;
}

int get_char(const int64* table, int64 val, int64 l, int64 h){
    int64 tmp = ((val - l + 1) * table[MAX_BYTE] - 1);
    int i = 0;
    for (; (i <= MAX_BYTE) and (table[i] * (h - l) <= tmp); ++i){}
    return i;
}


void compress_ari(char *ifile, char *ofile) {
    auto table = build_table(ifile);
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    for (int i = 0; i <= MAX_BYTE; ++i){
        printf("%c\t%x\t%llu\n", i, i, table[i]);
    }

    int ch = get_char(table, WORD_SIZE / 4, 0, WORD_SIZE / 2);
    printf("DEB: %c\t%x\n", ch, ch);

    fclose(ifp);
    fclose(ofp);
}

void decompress_ari(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    /** PUT YOUR CODE HERE
      * implement an arithmetic encoding algorithm for decompression
      * don't forget to change header file `ari.h`
    */

    // This is an implementation of simple copying
    size_t n, m;
    unsigned char buff[8192];

    do {
        n = fread(buff, 1, sizeof buff, ifp);
        if (n)
            m = fwrite(buff, 1, n, ofp);
        else
            m = 0;
    } while ((n > 0) && (n == m));

    fclose(ifp);
    fclose(ofp);
}
