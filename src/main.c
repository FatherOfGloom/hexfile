#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>

typedef int errno_t;

#define panic(...) do { fprintf(stderr, __VA_ARGS__); exit(0); } while (0)
#define todo() panic("panic at f:'%s' l:%d todo!", __FILE__, __LINE__)
#define nonnull(this) __extension__ ({ void* _ptr = (this); if (!_ptr) { panic("f:%s l:%d ERR: %s\n", __FILE__, __LINE__, "unwrap on a null value."); } _ptr; })

#define file_exists(file_path) (access(file_path, F_OK) == 0)

typedef struct File {
    FILE* handle;
} File;

errno_t file_size(File* file, size_t* size) {
    long saved = ftell(file->handle);
    if (saved < 0) return errno;
    if (fseek(file->handle, 0, SEEK_END) < 0) return errno;
    long result = ftell(file->handle);
    if (result < 0) return errno;
    if (fseek(file->handle, saved, SEEK_SET) < 0) return errno;
    *size = (size_t)result;
    return 0;
}

File file_open(const char* __restrict__ file_name, const char* __restrict__ mode) {
    return (File){.handle = nonnull(fopen(file_name, mode))};
}

errno_t file_close(File* file) {
    if (file->handle == NULL) return 0;
    errno_t result = fclose(file->handle);
    file->handle = NULL;
    return result;
}

typedef struct Slice {
    void* ptr;
    size_t len;
} Slice;

errno_t file_read_exact(File* file, Slice* buffer) {
    assert(buffer->ptr);
    assert(buffer->len);

    fread(buffer->ptr, buffer->len, 1, file->handle);

    if (ferror(file->handle)) return errno;

    return 0;
}

static const char* const usage = "Usage: hexfile [file-path]\n";

int main(int argc, char** argv) {
    if (argc != 2) {
        panic(usage);
    }

    char* file_path_cstr = argv[1];

    if (!file_exists(file_path_cstr)) {
        panic("Unable to find a file at path: '%s'", file_path_cstr);
    }

    File f = file_open(file_path_cstr, "rb");
    Slice buffer = {0};
    size_t size = 0;

    errno_t err = file_size(&f, &size);
    
    if (err != 0) {
        panic("Error at evaluating file size: '%s'", strerror(err));
    }

    if (size == 0) {
        return 0;
    }

    buffer.ptr = malloc(size);
    buffer.len = size;

    err = file_read_exact(&f, &buffer);

    if (err != 0) {
        panic("Error at reading file: '%s'", strerror(err));
    }

    printf("\t");

    for (size_t i = 0; i < 16; ++i) {
        printf(" %02llX", i);
    }

    printf("\n\n000000\t");

    char ascii_buffer[17] = {0};
    int ascii_buffer_idx = 0;
    int new_lines_count = 1;

    for (size_t i = 0; i < buffer.len; ++i) {
        uint8_t byte = *((uint8_t*)buffer.ptr + i);

        char mapped_ascii =
            (!isascii(byte) | (byte == '\r') | (byte == '\n') | (byte == '\0')) 
            ? '.' 
            : (char)byte;

        ascii_buffer[ascii_buffer_idx++] = mapped_ascii; 

        printf(" %02X", byte);

        if ((i + 1) % 16 == 0) {
            ascii_buffer_idx = 0;
            printf(" %s", ascii_buffer);
            printf("\n%06X", new_lines_count++);
            printf("\t");
        }
    }

    if (ascii_buffer_idx != 0) {
        ascii_buffer[ascii_buffer_idx] = 0; 
        int remaining_spaces = sizeof(ascii_buffer) - ascii_buffer_idx - 1; 
        remaining_spaces *= 3;
        for (int i = 0; i < remaining_spaces; ++i) {
            printf(" ");
        }

        printf(" %s\n", ascii_buffer);
    }

    file_close(&f);
    return 0;
}