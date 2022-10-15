#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <sys/stat.h>
#include <cstdint>

#include "ppm.h"
//#include "settings.h"  // DELETE before send

#define MAX_BYTE 255
#define WORD_SIZE 4294967295
//#define WORD_SIZE 65535
#define MAX_PARITY 2147483648
#define MIN_PARITY 2
#define DELTA 2097152

namespace ppm{

    struct compressed_file{
        FILE* fd;
        uint8_t buf;
        int buf_size;
        uint64_t file_len;
        uint64_t** table;
        int model_shape;
        char* path;
        bool is_open;
        bool mode; //true if open in write mode
        uint64_t delta;
        int prev_ch;
#define READ_M false
#define WRITE_M true

        explicit compressed_file(char* path) :
                buf(0), file_len(0), buf_size(0), is_open(false),
                mode(false), fd(nullptr), delta(1), prev_ch(0), model_shape(1)
        {
            this->path = static_cast<char *>(malloc(strlen(path)));
            strcpy(this->path, path);
            table = nullptr;
        }

        void init_dynamic_table(){
            this->table = new uint64_t*[MAX_BYTE + 1];
            for (int i = 1; i <= MAX_BYTE; ++i) table[i] = nullptr;
        }

        void strict_table(uint64_t* table){
            for (int i = MAX_BYTE; i > 0; --i) table[i] -= table[i - 1]; // decomposition
            for (int i = 0; i <= MAX_BYTE; ++i)
                if (table[i] >= 2*MIN_PARITY) table[i] = table[i] / 2;
            for (int i = 1; i <= MAX_BYTE; ++i) table[i] += table[i - 1]; // composition
        }

        uint64_t* get_table(int ch){
            if (table[ch] == nullptr){
                const uint64_t initial_val = 2;
                table[ch] = new uint64_t[MAX_BYTE + 1];
                table[ch][0] = initial_val;
                for (int i = 1; i <= MAX_BYTE; ++i) table[ch][i] = table[ch][i - 1] + initial_val;
            }
            return table[ch];
        }

        void update_dynamic_table(int ch){
            auto table_ptr = get_table(prev_ch);
            for (int i = ch; i <= MAX_BYTE; ++i) table_ptr[i] += delta;
            if (table_ptr[MAX_BYTE] < MAX_PARITY) strict_table(table_ptr);
            prev_ch = ch;
        }

        void write_header(){
            fwrite(&file_len, sizeof(file_len), 1, fd);
        }

        void read_header(){
            fread(&file_len, 8, 1, fd);
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
            if (buf_size == 0) return;
            buf <<= sizeof(buf) * 8 - buf_size;
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
                }else{
                    buf = 0;
                }
                buf_size = sizeof(buf) * 8;
            }
            int bit = (buf & 0x80) >> 7;
            buf <<= 1;
            --buf_size;
            return bit;
        }

        void write_bit(int bit){
            if ((mode != WRITE_M) or (!is_open)){
                throw std::runtime_error("Can't write to file");
            }
            if (buf_size == sizeof(buf) * 8){
                flush();
            }
            ++buf_size;
            buf += buf + (bit ? 1 : 0);
        }

        int get_char(uint64_t val, uint64_t l, uint64_t h){
            auto table_ptr = get_table(prev_ch);
            uint64_t tmp = (val - l + 1) * table_ptr[MAX_BYTE];
            int i = 0;
            for (; (i <= MAX_BYTE) and (table_ptr[i] * (h - l + 1) < tmp); ++i){}
            return i;
        }

        ~compressed_file() {
            flush();
            fclose(fd);
            free(path);
            delete[] table;
        }

    };

    uint64_t get_val(uint64_t* table, int pos){
        if (pos >= 0){
            return table[pos];
        }
        return 0;
    }

    void bits_plus_follow(int bit, uint64_t& bits_to_follow, compressed_file& file){
        file.write_bit(bit);
        for (; bits_to_follow > 0; --bits_to_follow){
            file.write_bit(!bit);
        }
    }

    uint64_t read_value(compressed_file& compressed, uint64_t bits_c){
        uint64_t res = 0;
        for (int i = 0; i < bits_c; ++i){
            res += res + compressed.read_bit();
        }
        return res;
    }

    uint64_t get_file_size(char* path){
        struct stat st{};
        int rs = stat(path, &st);
        if (rs != 0){
            throw std::runtime_error("Can't read file size");
        }
        printf("FILE SIZE = %ld\n", st.st_size);
        return st.st_size;
    }

}

void compress_ppm(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    auto compressed = ppm::compressed_file(ofile);
    compressed.delta = DELTA;
    compressed.open_to_write();
    compressed.init_dynamic_table();
    compressed.file_len = ppm::get_file_size(ifile);
    printf("Delta: %d\n", DELTA);
    printf("File len = %ld\n", compressed.file_len);
    compressed.write_header();

    uint64_t l = 0, h = WORD_SIZE;
    const uint64_t first_qtr = (h + 1)/ 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    uint64_t bits_to_follow = 0, i = 0;

    int ch;
    while ((ch = fgetc(ifp)) != EOF){
        ++i;
        auto table_ptr = compressed.get_table(compressed.prev_ch);
        uint64_t range = (h - l + 1);
        h = l + (ppm::get_val(table_ptr, ch    ) * range) / table_ptr[MAX_BYTE] - 1;
        l = l + (ppm::get_val(table_ptr, ch - 1) * range) / table_ptr[MAX_BYTE];
        compressed.update_dynamic_table(ch);
        for (;;){
            if ((l >= WORD_SIZE) or (h > WORD_SIZE)){
                char err_msg[256];
                sprintf(err_msg,
                        "Compressor error: boundary error\n"
                        "l = %ld\th = %ld\tch = %d\n", l, h, ch);
                throw std::runtime_error(err_msg);
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

    if (i > 0){
        ++bits_to_follow;
        if (l < first_qtr){
            bits_plus_follow(0, bits_to_follow, compressed);
        }else{
            bits_plus_follow(1, bits_to_follow, compressed);
        }
    }

    compressed.flush();
    fclose(ifp);
}

void decompress_ppm(char *ifile, char *ofile) {
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    auto compressed = ppm::compressed_file(ifile);
    compressed.open_to_read();
    compressed.delta = DELTA;
    compressed.init_dynamic_table();
    uint64_t file_len = compressed.file_len;

    printf("File len = %ld\n", file_len);

    uint64_t l = 0, h = WORD_SIZE;
    const uint64_t first_qtr = (h + 1)/ 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    uint64_t value = read_value(compressed, (WORD_SIZE == 65535) ? 16 : 32);

    for (uint64_t i = 0; i < compressed.file_len; ++i){
        uint64_t ch = compressed.get_char(value, l, h);
        fprintf(ofp, "%c", (char)ch);
        auto table_ptr = compressed.get_table(compressed.prev_ch);
        uint64_t range = (h - l + 1);
        h = l + (ppm::get_val(table_ptr, (int)ch    ) * range) / table_ptr[MAX_BYTE] - 1;
        l = l + (ppm::get_val(table_ptr, (int)ch - 1) * range) / table_ptr[MAX_BYTE];
        compressed.update_dynamic_table((int)ch);
        for (;;){
            if ((l >= WORD_SIZE) or (h > WORD_SIZE) or (value < l) or (value > h)){
                fclose(ofp);
                printf("i = %ld: l = %ld\th = %ld\tval = %ld\tch = %lx\n",i, l, h, value, ch);
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
