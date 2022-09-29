#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <stdexcept>

#include "ari.h"

typedef unsigned char byte_t;
#define MAX_BYTE 255

int32_t* build_table(char* ifile){
    auto table = new int32_t[MAX_BYTE + 1];
    FILE* ifp = (FILE*)fopen(ifile, "rb");
    if (!ifp){
        throw std::runtime_error("Can't open file");
    }

    int ch;
    while ((ch = fgetc(ifp)) != EOF){
        ++table[(byte_t)ch];
    }

    for (int i = 1; i <= MAX_BYTE; ++i){
        table[i] += table[i - 1];
    }

    fclose(ifp);
    return table;
}

void bits_plus_follow(int32_t bit, int64_t& bits_to_follow, FILE *ofp){
    fprintf(ofp, "%u", bit);
    for (; bits_to_follow > 0; --bits_to_follow){
        fprintf(ofp, "%u", !bit);
    }
}

void compress_ari(char *ifile, char *ofile) {
    auto table = build_table(ifile);
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    int64_t l = 0, h = (unsigned int32_t)(-1);
    printf("%ld %ld\n", l, h);
    int64_t divider = (unsigned)table[MAX_BYTE];
    int64_t first_qtr = (h + 1) / 4;
    int64_t half = first_qtr * 2, third_qtr = first_qtr * 3;
    int64_t bits_to_follow = 0, i = 0;
    int ch;

    while ((ch = fgetc(ifp)) != EOF){
        int64_t rng = (h - l + 1) / divider;
        h = l + table[ch    ] * rng - 1;
        l = l + table[ch - 1] * rng;
        for (;;){
            if (h < half){
                bits_plus_follow(0, bits_to_follow, ofp);
            }
            else if (l >= half){
                bits_plus_follow(1, bits_to_follow, ofp);
                l -= half;
                h -= half;
            }
            else if ((l >= first_qtr) and (h < third_qtr)){
                ++bits_to_follow;
                l -= first_qtr;
                h -= first_qtr;
            } else break;
            l += l; h += h + 1;
        }
    }

    delete[] table;  // TODO: use smart_ptr to table
    fclose(ifp);
    fclose(ofp);
}

int32_t* get_table(char* ifile){

}

void decompress_ari(char *ifile, char *ofile) {
    auto table = build_table(ifile);
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    int64_t l = 0, h = (unsigned int32_t)(-1);
    printf("%ld %ld\n", l, h);
    int64_t divider = (unsigned)table[MAX_BYTE];
    int64_t first_qtr = (h + 1) / 4;
    int64_t half = first_qtr * 2, third_qtr = first_qtr * 3;
    int64_t bits_to_follow = 0, i = 0;
    int ch;

    delete[] table;
    fclose(ifp);
    fclose(ofp);
}
