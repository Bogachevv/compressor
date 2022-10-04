#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <string.h>

#include "ari.h"

#define MAX_BYTE 255
//#define WORD_SIZE 4294967295
#define WORD_SIZE 65535

typedef unsigned long long int64;
typedef unsigned char byte_t;

struct compressed_file{
    FILE* fd;
    byte_t buf;
    int buf_size;
    int64 file_len;
    int64* table;
    char* path;
    bool is_open;
    bool mode; //true if open in write mode
#define READ_M false
#define WRITE_M true

    explicit compressed_file(char* path) : buf(0), file_len(0), buf_size(0), is_open(false), mode(false), fd(nullptr){
        this->path = static_cast<char *>(malloc(strlen(path)));
        strcpy(this->path, path);
        table = new int64[MAX_BYTE + 1];
    }

    int64 read_int(int bytes_c){
        int64 res = 0;
        fread(&res, bytes_c, 1, fd);
        return res;
    }

    void write_header(){
        fwrite(&file_len, sizeof(file_len), 1, fd);
        for (int i = 0; i <= MAX_BYTE; ++i){
            fwrite(&(table[i]), sizeof(*table), 1, fd);
        }
    }

    void read_header(){
        fread(&file_len, 8, 1, fd);
        fread(table, 8, MAX_BYTE + 1, fd);
    }

    void open_to_read(){
        fd = fopen(path, "rb");
        if (!fd){
            throw std::runtime_error("Can't open file");
        }
        is_open = true;
        mode = READ_M;
        read_header();
    }

    void open_to_write(){
        fd = fopen(path, "wb");
        if (!fd){
            throw std::runtime_error("Can't open file");
        }
        is_open = true;
        mode = WRITE_M;
    }

    void flush(){
//        buf <<= sizeof(buf) * 8 - buf_size;
//        fwrite(&buf, sizeof(buf), 1, fd);
//        buf = 0;
//        buf_size = 0;
    }

    int read_bit(){
//        if ((mode != READ_M) or (!is_open)){
//            throw std::runtime_error("Can't read from file");
//        }
//        if (buf_size == 0){
//            if (!feof(fd)){
//                fread(&buf, sizeof(buf), 1, fd);
//            }else{
//                buf = 0;
//            }
//
//            buf_size = sizeof(buf) * 8;
//        }
//        int bit = (buf & 0x80) >> 7;
//        int bit = buf & 1;
//        buf >>= 1;
//        --buf_size;
        bool bit;
        if (!feof(fd))
            fread(&bit, sizeof(bool), 1, fd);
        else
            bit = false;
        printf("%d", bit);
        return bit;
    }

    void write_bit(int bit){
        printf("%d", bit);
//        if ((mode != WRITE_M) or (!is_open)){
//            throw std::runtime_error("Can't read from file");
//        }
//        if (buf_size == sizeof(buf_size) * 8){
//            flush();
//        }
//        buf <<= 1;
//        buf += bit;
//        ++buf_size;
        bool bt = (bit == 1);
        fwrite(&bt, sizeof(bool), 1, fd);
    }

    ~compressed_file(){
        flush();
        fclose(fd);
        free(path);
//        delete[] table;
    }


};


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


void bits_plus_follow(int bit, int64& bits_to_follow, compressed_file& file){
    file.write_bit(bit);
    for (; bits_to_follow > 0; --bits_to_follow){
        file.write_bit(!bit);
    }
}

int64 read_bit(FILE* ifp){
    int ch = fgetc(ifp);
    if (ch == EOF){
        return 0; // or throw exception?
    }
    return ch - '0';
}

int64 read_value(compressed_file& compressed, int64 bits_c){
    int64 res = 0;
    for (int i = 0; i < bits_c; ++i){
        res += res + compressed.read_bit();
    }
    return res;
}

int64 get_file_len(FILE* ifp){
    return 1040; //DEBUG
}

void compress_ari(char *ifile, char *ofile) {
    auto table = build_table(ifile);
    FILE *ifp = (FILE *)fopen(ifile, "rb");

    auto compressed = compressed_file(ofile);
    compressed.open_to_write();
    compressed.table = table;
    compressed.file_len = table[MAX_BYTE];
    compressed.write_header();

    int64 l = 0, h = WORD_SIZE;
    const int64 first_qtr = (h + 1)/ 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    int64 bits_to_follow = 0, i = 0;

    int ch;
    while ((ch = fgetc(ifp)) != EOF){
        ++i;
        int64 range = (h - l + 1);
        h = l + (table[ch    ] * range) / table[MAX_BYTE] - 1;
        l = l + (table[ch - 1] * range) / table[MAX_BYTE];
        for (;;){
//            printf("l = %lld(%f)\th = %lld(%f)\n", l, (double)l / WORD_SIZE, h, (double)h / WORD_SIZE);
            if ((l >= WORD_SIZE) or (h > WORD_SIZE)){
                throw std::runtime_error("Boundary error");
            }
            if (h < half){
                bits_plus_follow(0, bits_to_follow, compressed);
            }
            else if (l >= half){
                bits_plus_follow(1, bits_to_follow, compressed);
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
        bits_plus_follow(0, bits_to_follow, compressed);
    }else{
        bits_plus_follow(1, bits_to_follow, compressed);
    }

    printf("\nFILE LEN = %lld", i);
//    compressed.file_len = i;
//    compressed.write_header(true);

    delete[] table;
    fclose(ifp);
}

void decompress_ari(char *ifile, char *ofile) {
//    const char* path = "input.txt";
//    auto table = build_table(path);
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    auto compressed = compressed_file(ifile);
    compressed.open_to_read();
    int64 file_len = compressed.file_len;
    int64* table = compressed.table;

    printf("File len = %lld\n", file_len);
    for (int i = 0; i <= MAX_BYTE; ++i){
        printf("%c\t%x\t%llu\n", i, i, table[i]);
    }

    int64 l = 0, h = WORD_SIZE;
    const int64 first_qtr = (h + 1)/ 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    int64 value = read_value(compressed, (WORD_SIZE == 65535) ? 16 : 32);
//    printf("value = %lld\n", value);

    for (int64 i = 0; i < compressed.file_len; ++i){
        int64 ch = get_char(table, value, l, h);
        fprintf(ofp, "%c", (char)ch);
        int64 range = (h - l + 1);
        h = l + (table[ch    ] * range) / table[MAX_BYTE] - 1;
        l = l + (table[ch - 1] * range) / table[MAX_BYTE];

        for (;;){
//            printf("value = %lld\tl = %lld\th = %lld\n", value, l, h);
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
            value += value + compressed.read_bit();
        }

    }

    fclose(ofp);
}
