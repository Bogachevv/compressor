#include <cstdlib>
#include <cstdio>
#include <stdexcept>

#include "ari.h"

#define MAX_BYTE 255
//#define WORD_SIZE 4294967295
#define WORD_SIZE 65535

typedef unsigned long long int64;


int64* build_table(const char* ifile){
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


void bits_plus_follow(int bit, int64& bits_to_follow, FILE* ofp){
    fprintf(ofp, "%d", bit);
    for (; bits_to_follow > 0; --bits_to_follow){
        fprintf(ofp, "%d", !bit);
    }
}

int64 read_bit(FILE* ifp){
    int ch = fgetc(ifp);
    if (ch == EOF){
        return 0; // or throw exception?
    }
    return ch - '0';
}

int64 read_value(FILE* ifp, int64 bits_c){
    int64 res = 0;
    for (int64 i = 0; i < bits_c; ++i){
        res += res + read_bit(ifp);
    }
    return res;
}

int64 get_file_len(FILE* ifp){
    return 1040; //DEBUG
}

void compress_ari(char *ifile, char *ofile) {
    auto table = build_table(ifile);
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    for (int i = 0; i <= MAX_BYTE; ++i){
        printf("%c\t%x\t%llu\n", i, i, table[i]);
    }

    int64 l = 0, h = WORD_SIZE;
    const int64 first_qtr = (h + 1)/ 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    int64 bits_to_follow = 0, i = 0;

//    double val = 0.1875;
//    int ch = get_char(table, val * WORD_SIZE, 0, WORD_SIZE);
//    printf("%c", ch);
//    ch = get_char(table, val * WORD_SIZE, 0, WORD_SIZE / 2 - 1);
//    printf("%c", ch);
//    ch = get_char(table, val * WORD_SIZE, WORD_SIZE / 8, WORD_SIZE / 4 - 1);
//    printf("%c", ch);
//    ch = get_char(table, val * WORD_SIZE, WORD_SIZE / 16, WORD_SIZE / 4 - 1);
//    printf("%c\n", ch);
//
//    exit(0);

    int ch;
    while ((ch = fgetc(ifp)) != EOF){
        ++i;
        int64 range = (h - l + 1);
        h = l + (table[ch    ] * range) / table[MAX_BYTE] - 1;
        l = l + (table[ch - 1] * range) / table[MAX_BYTE];
        for (;;){
            printf("l = %lld(%f)\th = %lld(%f)\n", l, (double)l / WORD_SIZE, h, (double)h / WORD_SIZE);
            if ((l >= WORD_SIZE) or (h > WORD_SIZE)){
                fclose(ofp);
                throw std::runtime_error("Boundary error");
            }
            if (h < half){
                bits_plus_follow(0, bits_to_follow, ofp);
            }
            else if (l >= half){
                bits_plus_follow(1, bits_to_follow, ofp);
                l -= half; h -= half;
            }
            else if ((l >= first_qtr) and (h < third_qtr)){
                ++bits_to_follow;
                l -= first_qtr; h -= first_qtr;
            } else break;
            l += l; h += h + 1;
        }
    }

    ++bits_to_follow;
    if (l < first_qtr){
        bits_plus_follow(0, bits_to_follow, ofp);
    }else{
        bits_plus_follow(1, bits_to_follow, ofp);
    }

    printf("FILE LEN = %lld", i);

    delete[] table;
    fclose(ifp);
    fclose(ofp);
}

void decompress_ari(char *ifile, char *ofile) {
    const char* path = "input.txt";
    auto table = build_table(path);
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    int64 l = 0, h = WORD_SIZE;
    const int64 first_qtr = (h + 1)/ 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    int64 value = read_value(ifp, (WORD_SIZE == 65535) ? 16 : 32);

    for (int64 i = 0; i < get_file_len(ifp); ++i){
        int64 ch = get_char(table, value, l, h);
        fprintf(ofp, "%c", (char)ch);
        int64 range = (h - l + 1);
        h = l + (table[ch    ] * range) / table[MAX_BYTE] - 1;
        l = l + (table[ch - 1] * range) / table[MAX_BYTE];

        for (;;){
            printf("value = %lld\tl = %lld\th = %lld\n", value, l, h);
            if ((l >= WORD_SIZE) or (h > WORD_SIZE) or (value > WORD_SIZE)){
                fclose(ofp);
                throw std::runtime_error("Boundary error");
            }

            if (h < half){}
            else if (l >= half){
                value -= half;
                l-= half; h -= half;
            }
            else if ((l >= first_qtr) and (h < third_qtr)){
                value -= first_qtr;
                l -= first_qtr; h -= first_qtr;
            }
            else break;
            l += l; h += h + 1;
            value += value + read_bit(ifp);
        }

    }

    fclose(ifp);
    fclose(ofp);
}
