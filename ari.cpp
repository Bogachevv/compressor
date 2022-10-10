#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <sys/stat.h>

#include "ari.h"

#define MAX_BYTE 255
#define WORD_SIZE 4294967295
//#define WORD_SIZE 65535
#define MAX_PARITY 65535
#define MIN_PARITY 32
#define DELTA 16

typedef unsigned long long int64;
typedef unsigned char byte_t;

void print_table(int64* table){
    for (int i = 0; i <= MAX_BYTE; ++i) {
        printf("table[%d] = %lld\n", i, table[i]);
    }
}

struct compressed_file{
    FILE* fd;
    byte_t buf;
    int buf_size;
    int64 file_len;
    int64* table;
    char* path;
    bool is_open;
    bool mode; //true if open in write mode
    bool static_table;
    int64 delta;
#define READ_M false
#define WRITE_M true

    explicit compressed_file(char* path, bool static_table = true) :
    buf(0), file_len(0), buf_size(0), is_open(false), mode(false), fd(nullptr), delta(1)
    {
        this->path = static_cast<char *>(malloc(strlen(path)));
        strcpy(this->path, path);
        this->static_table = static_table;
        table = new int64[MAX_BYTE + 1];
    }

    void init_dynamic_table(int64 initial_val){
        this->table = new int64[MAX_BYTE + 1];
        table[0] = initial_val;
        for (int i = 1; i <= MAX_BYTE; ++i) table[i] = table[i - 1] + initial_val;
    }

    void update_dynamic_table(int ch){
        if (table[MAX_BYTE] < MAX_PARITY){
            for (int i = ch; i <= MAX_BYTE; ++i) table[i] += delta;
            return;
        }

        for (int i = MAX_BYTE; i > 0; --i) table[i] -= table[i - 1]; // decomposition
        table[ch] += delta;
        for (int i = 0; i <= MAX_BYTE; ++i)
            if (table[i] >= 2*MIN_PARITY) table[i] = table[i] / 2;
        for (int i = 1; i <= MAX_BYTE; ++i) table[i] += table[i - 1]; // composition
    }

    void write_header(){
        fwrite(&file_len, sizeof(file_len), 1, fd);
        if (static_table) {
            auto proxy_table = new unsigned short[MAX_BYTE + 1];
            for (int i = 0; i <= MAX_BYTE; ++i) {
                proxy_table[i] = (unsigned short) table[i];
            }
            fwrite(proxy_table, sizeof(*proxy_table), MAX_BYTE + 1, fd);
            delete[] proxy_table;
        }
    }

    void read_header(){
        fread(&file_len, 8, 1, fd);
        if (static_table) {
            auto proxy_table = new unsigned short[MAX_BYTE + 1];
            fread(proxy_table, sizeof(*proxy_table), MAX_BYTE + 1, fd);
            for (int i = 0; i <= MAX_BYTE; ++i) {
                table[i] = (int64) proxy_table[i];
            }
            delete[] proxy_table;
        }
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
        buf <<= sizeof(buf) * 8 - buf_size;
//        printf("BUF: %d\n", buf);
        fwrite(&buf, sizeof(buf), 1, fd);
        buf = 0;
        buf_size = 0;
    }

    int read_bit(){
        if ((mode != READ_M) or (!is_open)){
            throw std::runtime_error("Can't read from file");
        }
        if (buf_size == 0){
            if (!feof(fd)){
                fread(&buf, sizeof(buf), 1, fd);
//                printf("BUF: %d\n", buf);
            }else{
                buf = 0;
            }
            buf_size = sizeof(buf) * 8;
        }
        int bit = (buf & 0x80) >> 7;
        buf <<= 1;
        --buf_size;
//        printf("%d", bit);
        return bit;
    }

    void write_bit(int bit){
        if ((mode != WRITE_M) or (!is_open)){
            throw std::runtime_error("Can't write to file");
        }
//        printf("%d", bit);
        if (buf_size == sizeof(buf) * 8){
            flush();
        }
        ++buf_size;
        buf += buf + (bit ? 1 : 0);
}

    ~compressed_file(){
        flush();
        fclose(fd);
        free(path);
        delete[] table;
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

    for (;;){
        int64 sum = 0;
        for (int i = 0; i <= MAX_BYTE; ++i) sum += table[i];
        if (sum <= MAX_PARITY) break;
        for (int i = 0; i <= MAX_BYTE; ++i){
            if (table[i] >= MIN_PARITY){
                table[i] >>= 1;
            }
        }
    }

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

int64 read_value(compressed_file& compressed, int64 bits_c){
    int64 res = 0;
    for (int i = 0; i < bits_c; ++i){
        res += res + compressed.read_bit();
    }
    return res;
}

int64 get_val(int64* table, int pos){
    if (pos >= 0){
        return table[pos];
    }
    return 0;
}

int64 get_file_size(char* path){
    struct stat st{};
    int rs = stat(path, &st);
    if (rs != 0){
        throw std::runtime_error("Can't read file size");
    }
    printf("FILE SIZE = %ld\n", st.st_size);
    return st.st_size;
}

void compress_ari(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    auto compressed = compressed_file(ofile, false);
//    compressed.table = build_table(ifile);
    compressed.delta = DELTA;
    compressed.open_to_write();
    compressed.init_dynamic_table(2);
    compressed.file_len = get_file_size(ifile);
    printf("File len = %lld\n", compressed.file_len);
    compressed.write_header();

    auto table = compressed.table;
    int64 l = 0, h = WORD_SIZE;
    const int64 first_qtr = (h + 1)/ 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    int64 bits_to_follow = 0, i = 0;

    int ch;
    while ((ch = fgetc(ifp)) != EOF){
        ++i;
        int64 range = (h - l + 1);
        h = l + (get_val(table, ch    ) * range) / table[MAX_BYTE] - 1;
        l = l + (get_val(table, ch - 1) * range) / table[MAX_BYTE];
        compressed.update_dynamic_table(ch);
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
    compressed.flush();

    for (int chr = 0; chr <= MAX_BYTE; ++chr){
        printf("%c\t%d\t%lld\n", (char)chr, chr, table[chr]);
    }

    fclose(ifp);
}

void decompress_ari(char *ifile, char *ofile) {
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    auto compressed = compressed_file(ifile, false);
    compressed.open_to_read();
    compressed.delta = DELTA;
    compressed.init_dynamic_table(2);
    int64 file_len = compressed.file_len;
    int64* table = compressed.table;

    printf("File len = %lld\n", file_len);

    int64 l = 0, h = WORD_SIZE;
    const int64 first_qtr = (h + 1)/ 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    int64 value = read_value(compressed, (WORD_SIZE == 65535) ? 16 : 32);

    for (int64 i = 0; i < compressed.file_len; ++i){
        int64 ch = get_char(table, value, l, h);
        fprintf(ofp, "%c", (char)ch);
        int64 range = (h - l + 1);
        h = l + (get_val(table, (int)ch    ) * range) / table[MAX_BYTE] - 1;
        l = l + (get_val(table, (int)ch - 1) * range) / table[MAX_BYTE];
        compressed.update_dynamic_table((int)ch);
        for (;;){
//            printf("value = %lld\tl = %lld\th = %lld\n", value, l, h);
//            if (value < l) value = l;
//            if (value >= h) value = h - 1;
            if ((l >= WORD_SIZE) or (h > WORD_SIZE) or (value < l) or (value >= h)){
                fclose(ofp);
                printf("i = %lld: l = %lld\th = %lld\tval = %lld\tch = %llx\n",i, l, h, value, ch);
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

    for (int i = 0; i <= MAX_BYTE; ++i){
        printf("%c\t%d\t%lld\n", (char)i, i, table[i]);
    }

    fclose(ofp);
}
