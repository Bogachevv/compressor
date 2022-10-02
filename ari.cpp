#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <stdexcept>

#include "ari.h"

typedef unsigned char byte_t;
typedef unsigned long long int64;
#define MAX_BYTE 255
#define WORD_SIZE 65535

int64* build_table(const char* ifile){
    auto table = new int64[MAX_BYTE + 1];
    for (size_t i = 0; i <= MAX_BYTE; ++i){
        table[i] = 1;
    }

    int ch;
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    while ((ch = fgetc(ifp)) != EOF){
        table[ch] += 10;
    }
    fclose(ifp);

    for (;;){
        int64 sum = 0;
        for (size_t i = 0; i <= MAX_BYTE; ++i) sum += table[i];
        if (sum <= WORD_SIZE) break;
        for (size_t i = 0; i <= MAX_BYTE; ++i){
            table[i] = table[i] >> 1; // div 2
            table[i] = (table[i] == 0) ? 1 : table[i];
        }
    }

    for (size_t i = 1; i <= MAX_BYTE; ++i){
        table[i] += table[i - 1];
    }
    return table;
}

void bits_plus_follow(unsigned int bit, int64& bits_to_follow, FILE *ifp) {
    fprintf(ifp, "%d", bit);
    for (; bits_to_follow > 0; --bits_to_follow){
        fprintf(ifp, "%d", !bit);
    }
}

void compress_ari(char *ifile, char *ofile) {
    auto table = build_table(ifile);
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    int64 divider = table[MAX_BYTE];

    int64 l = 0, h = WORD_SIZE, i = 0;
    int64 first_qtr = (h + 1) / 4, half = first_qtr * 2;
    int64 third_qtr = first_qtr * 3, bits_to_follow = 0;

    int ch;
    while ((ch = fgetc(ifp)) != EOF){
       ++i;
       int64 rng = (h - l + 1);
       h = l + (table[ch    ] * rng) / divider - 1;
       l = l + (table[ch - 1] * rng) / divider;

       for (;;){
           if (h < half){
               bits_plus_follow(0, bits_to_follow, ofp);
           }
           else if (l >= half){
               bits_plus_follow(1, bits_to_follow, ofp);
               l -= half; h -= half;
           }
           else if ((l >= first_qtr) and (h <= third_qtr)){
               ++bits_to_follow;
               l -= first_qtr; h -= first_qtr;
           }
           else break;

           l += l; h += h + 1;
       }
    }

    ++bits_to_follow;
    if (l < first_qtr){
        bits_plus_follow(0, bits_to_follow, ifp);
    }else{
        bits_plus_follow(1, bits_to_follow, ifp);
    }

    printf("FILE LEN = %lld\n", i);
    delete[] table;
    fclose(ifp);
    fclose(ofp);
}

int64 read_int16(FILE* ifp){
    int64 res = 0;
    int ch;
    for (int64 i = 0; (i < 16) and ((ch = fgetc(ifp)) != EOF); ++i){
        ch -= '0';
        res += res + ch;
    }
    return res;
}

int64 read_bit(FILE* ifp){
    int ch = fgetc(ifp);
    if (ch == EOF){
        return 0;
//        throw std::runtime_error("Unexpected end of file");
    }
    return ch - '0';
}

int64 get_file_len(FILE* ifp){
    return 12;
}

unsigned char get_char(int64 value, const int64* table, int64 l, int64 h){
    int64 i = 0;
    int64 tmp = ((value - l + 1) * table[MAX_BYTE] - 1) / (h - l + 1);
    for (; (i <= MAX_BYTE) and (table[i] <= tmp); ++i){ }
    return i;
}

void decompress_ari(char *ifile, char *ofile) {
    auto path = "test_file.txt";
    auto table = build_table(path);
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    int64 divider = table[MAX_BYTE];
    int64 l = 0, h = WORD_SIZE;
    int64 first_qtr = (h + 1) / 4, half = first_qtr * 2;
    int64 third_qtr = first_qtr * 3;

    int64 value = read_int16(ifp);
    for (int64 i = 0; i < get_file_len(ifp); ++i){
        int64 ch = get_char(value, table, l, h);
        printf("CHAR = %c\t%lld\n", ch, ch);
        fputc((char)ch, ofp);
        int64 rng = (h - l + 1) / divider;
        h = l + table[ch    ] * rng - 1;
        l = l + table[ch - 1] * rng;

        for (;;){
            printf("value = %lld, l = %lld, h = %lld\n", value, l, h);
            if ((value > WORD_SIZE) or (l > WORD_SIZE) or (h > WORD_SIZE)){
                throw std::runtime_error("Bound error");
            }
            if (h <= half){

            }
            else if (l >= half){
                value -= half;
                l -= half; h -= half;
            }
            else if ((l >= first_qtr) and (h < third_qtr)){
                value -= first_qtr;
                l -= first_qtr; h -= first_qtr;
            }
            else break;

            l += l; h += h + 1;
            value += value + read_bit(ifp);
        }
//        printf("%c\t%d\n", ch, ch);
    }
    int64 ch = get_char(value, table, l, h);
    printf("CHAR = %c\t%lld\n", ch, ch);
    fputc((char)ch, ofp);

    fclose(ifp);
    fclose(ofp);
}
