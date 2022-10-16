#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <sys/stat.h>
#include <cstdint>

#include "ppm.h"
#include "settings.h"  // DELETE before send

#define MAX_BYTE 255
#define WORD_SIZE 4294967295
//#define WORD_SIZE 65535
//#define MAX_PARITY 8388608
//#define MIN_PARITY 2
//#define DELTA 1625
//#define SHAPE 5

namespace ppm{
    struct compressed_file{
        FILE* fd;
        uint8_t buf;
        int buf_size;
        uint64_t file_len;
        void **table;
        int model_shape;
        int *prev_ch_seq;
        char* path;
        bool is_open;
        bool mode; //true if open in write mode
        uint64_t delta;
        uint64_t available_memory;
#define READ_M false
#define WRITE_M true

        explicit compressed_file(char* path, int model_shape, uint64_t available_memory = UINT64_MAX) :
                buf(0), file_len(0), buf_size(0), is_open(false), mode(false), fd(nullptr), delta(1),
                model_shape(model_shape), available_memory(available_memory)
        {
            this->path = static_cast<char *>(malloc(strlen(path) + 1));
            strcpy(this->path, path);
            table = nullptr;
            prev_ch_seq = new int[model_shape];
            this->available_memory -= (model_shape) * sizeof(*prev_ch_seq);
            for (int i = 0; i < model_shape; ++i) prev_ch_seq[i] = 0;

        }

        void init_dynamic_table(uint64_t initial_val){
            if (model_shape == 0){
                uint64_t table_size = (MAX_BYTE + 1) * sizeof(uint16_t);
                if (this->available_memory < table_size) throw std::runtime_error("Compressed file: not enough memory");
                auto tmp_ptr = new uint16_t[MAX_BYTE + 1];
                this->available_memory -= table_size;
                tmp_ptr[0] = initial_val;
                for (int i = 1; i <= MAX_BYTE; ++i) tmp_ptr[i] = tmp_ptr[i - 1] + initial_val;
                this->table = (void**)tmp_ptr;
                return;
            }
            uint64_t table_size = (MAX_BYTE + 1) * sizeof(void*);
            if (this->available_memory < table_size) throw std::runtime_error("Compressed file: not enough memory");
            this->table = new void*[MAX_BYTE + 1];
            this->available_memory -= table_size;
            for (int i = 0; i <= MAX_BYTE; ++i) table[i] = nullptr;
        }

        void strict_table(uint16_t *table_ptr){
//            for (int i = MAX_BYTE; i > 0; --i) table_ptr[i] -= table_ptr[i - 1]; // decomposition
            for (int i = 0; i <= MAX_BYTE; ++i)
                if (table_ptr[i] >= 2 * MIN_PARITY) table_ptr[i] = table_ptr[i] / 2;
//            for (int i = 1; i <= MAX_BYTE; ++i) table_ptr[i] += table_ptr[i - 1]; // composition
        }

        uint16_t* get_table(){
            if (model_shape == 0) return (uint16_t*)table;
            void** table_ptr = table;
            for (int i = 0; i < model_shape - 1; ++i){
                if (table_ptr[prev_ch_seq[i]] == nullptr){
                    //init_pointer_table
                    uint64_t table_size = (MAX_BYTE + 1) * sizeof(void*);
                    if (this->available_memory < table_size)
                        throw std::runtime_error("Compressed file: not enough memory");
                    table_ptr[prev_ch_seq[i]] = new void*[MAX_BYTE+1];
                    this->available_memory -= table_size;
                    void** tmp_ptr = (void**)table_ptr[prev_ch_seq[i]];
                    for (int j = 0; j <= MAX_BYTE; ++j) tmp_ptr[j] = nullptr;
                }
                table_ptr = (void**)(table_ptr[prev_ch_seq[i]]);
            }

            if (table_ptr[prev_ch_seq[model_shape - 1]] == nullptr){
                //init_int64_table
                uint64_t table_size = (MAX_BYTE + 1) * sizeof(uint16_t);
                if (this->available_memory < table_size)
                    throw std::runtime_error("Compressed file: not enough memory");
                auto tmp_ptr = new uint16_t[MAX_BYTE+1];
                this->available_memory -= table_size;
                const int initial_val = 2;
                tmp_ptr[0] = initial_val;
                for (int j = 1; j <= MAX_BYTE; ++j) tmp_ptr[j] = tmp_ptr[j - 1] + initial_val;
                table_ptr[prev_ch_seq[model_shape - 1]] = (void**)tmp_ptr;
            }
            table_ptr = (void**)(table_ptr[prev_ch_seq[model_shape - 1]]);
            return (uint16_t*)table_ptr;
        }

        void load_table(uint64_t *table_ptr){
            auto base_table = get_table();
            table_ptr[0] = base_table[0];
            for (int i = 1; i <= MAX_BYTE; ++i) table_ptr[i] = table_ptr[i - 1] + base_table[i];
        }

        void add_prev_ch(int ch){
            for (int i = 0; i < model_shape - 1; ++i){
                prev_ch_seq[i] = prev_ch_seq[i + 1];
            }
            prev_ch_seq[model_shape - 1] = ch;
        }

        void update_dynamic_table(int ch){
            auto table_ptr = get_table();
            if ((uint64_t)table_ptr + delta >= UINT16_MAX) strict_table(table_ptr);
            table_ptr[ch] += delta;
//            for (int i = ch; i <= MAX_BYTE; ++i) table_ptr[i] += delta;
//            if (table_ptr[MAX_BYTE] >= MAX_PARITY) strict_table(table_ptr);
            if (model_shape > 0) add_prev_ch(ch);
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

        void free_table(void **table_ptr, int shape){
            if (shape == 0) {
                delete[] table_ptr;
                return;
            }
            for (int i = 0; i <= MAX_BYTE; ++i)
                if (table_ptr[i] != nullptr) free_table((void**)table_ptr[i], shape - 1);
            delete[] table_ptr;
        }

        ~compressed_file(){
            flush();
            fclose(fd);
            free(path);
            delete[] prev_ch_seq;
            free_table(table, model_shape);
        }

    };

    int get_char(uint64_t *table_ptr, uint64_t val, uint64_t l, uint64_t h){
        uint64_t tmp = (val - l + 1) * table_ptr[MAX_BYTE];
        int i = 0;
        for (; (i <= MAX_BYTE) and (table_ptr[i] * (h - l + 1) < tmp); ++i){}
        return i;
    }

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
    auto compressed = ppm::compressed_file(ofile, SHAPE, 2048*1024*1024ul);
    compressed.delta = DELTA;
    compressed.open_to_write();
    compressed.init_dynamic_table(2);
    compressed.file_len = ppm::get_file_size(ifile);
    printf("Delta: %d\n", DELTA);
    printf("File len = %ld\n", compressed.file_len);
    compressed.write_header();

    uint64_t l = 0, h = WORD_SIZE;
    const uint64_t first_qtr = (h + 1) / 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    uint64_t bits_to_follow = 0, i = 0;
    auto table = new uint64_t[MAX_BYTE + 1];

    int ch;
    while ((ch = fgetc(ifp)) != EOF){
        ++i;
        uint64_t range = (h - l + 1);
        compressed.load_table(table);
        h = l + (ppm::get_val(table, ch    ) * range) / table[MAX_BYTE] - 1;
        l = l + (ppm::get_val(table, ch - 1) * range) / table[MAX_BYTE];
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
    delete[] table;
}

void decompress_ppm(char *ifile, char *ofile) {
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    auto compressed = ppm::compressed_file(ifile, SHAPE);
    compressed.open_to_read();
    compressed.delta = DELTA;
    compressed.init_dynamic_table(2);
    uint64_t file_len = compressed.file_len;

    printf("File len = %ld\n", file_len);

    uint64_t l = 0, h = WORD_SIZE;
    const uint64_t first_qtr = (h + 1) / 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    uint64_t value = read_value(compressed, (WORD_SIZE == 65535) ? 16 : 32);
    auto table = new uint64_t[MAX_BYTE + 1];

    for (uint64_t i = 0; i < compressed.file_len; ++i){
        compressed.load_table(table);
        uint64_t ch = ppm::get_char(table, value, l, h);
        fprintf(ofp, "%c", (char)ch);
        uint64_t range = (h - l + 1);
        h = l + (ppm::get_val(table, (int)ch    ) * range) / table[MAX_BYTE] - 1;
        l = l + (ppm::get_val(table, (int)ch - 1) * range) / table[MAX_BYTE];
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
    delete[] table;
}
