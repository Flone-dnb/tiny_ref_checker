#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __linux__
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#else
#error "unsupported OS"
#endif

unsigned int processed_src_count;
unsigned int processed_ref_count;

bool
find_name(const char* code, unsigned long code_len, const char* name, unsigned long name_len) {
    unsigned long name_i = 0;
    for (unsigned long i = 0; i < code_len; i++) {
        if (code[i] == name[name_i]) {
            if (name_i == 0 && i > 0 && code[i - 1] != ' ') {
                continue;
            }

            if (name_i == 0 && (i >= 5)) {
                // Make sure this is not a @ref.
                char text[6] = {0};
                memcpy(text, code + (i - 5), 5);
                if (strcmp(text, "@ref ") == 0) {
                    continue;
                }
            }

            name_i += 1;
            if (name_i == name_len) {
                // Found a match, make sure it's a function or a variable.
                if (i + 1 >= code_len) {
                    return false;
                }
                if (code[i + 1] == '(' || code[i + 1] == ';') {
                    return true;
                }
                name_i = 0;
            }
        } else {
            name_i = 0;
        }
    }

    return false;
}

int
check_code(const char* code, unsigned long code_len, const char* file_name, const char* path_to_file) {
    for (unsigned long i = 0; i < code_len; i++) {
        if (code[i] == '@' && (i + 6 < code_len) && code[i + 1] == 'r' && code[i + 2] == 'e'
            && code[i + 3] == 'f' && code[i + 4] == ' ') {
            if (code[i + 5] == ' ') {
                printf("error, found \"@ref  \" with more than 1 space after \"@ref\"");
                return 1;
            }

            unsigned long name_len = 0;
            for (unsigned long k = i + 5; k < code_len; k++) {
                if ((code[k] >= 'A' && code[k] <= 'Z') || (code[k] >= 'a' && code[k] <= 'z')
                    || (code[k] >= '0' && code[k] <= '9') || code[k] == '_') {
                    name_len += 1;
                    continue;
                }
                break;
            }

            char* name = malloc(sizeof(char) * (name_len + 1));
            name[name_len] = 0;
            memcpy(name, code + (i + 5), name_len);

            if (!find_name(code, code_len, name, name_len)) {
                printf("unable to find a referenced name \"%s\" in the file \"%s\" (%s)", name, file_name,
                       path_to_file);
                free(name);
                return 1;
            }
            processed_ref_count += 1;

            free(name);
        }
    }

    return 0;
}

int
process_file(const char* path_to_file, unsigned long path_len, const char* filename) {
    bool is_src_file = false;

    if (path_len >= 2) {
        char ext[3] = {0};
        memcpy(ext, path_to_file + (path_len - 2), 2);
        if (strcmp(ext, ".h") == 0 || strcmp(ext, ".c") == 0) {
            is_src_file = true;
        }
    }

    if (path_len >= 4) {
        char ext[5] = {0};
        memcpy(ext, path_to_file + (path_len - 4), 4);
        if (strcmp(ext, ".hpp") == 0 || strcmp(ext, ".cpp") == 0) {
            is_src_file = true;
        }
    }

    if (!is_src_file) {
        return 0;
    }

    // Open file.
    FILE* f = fopen(path_to_file, "rb");
    if (f == NULL) {
        printf("unable read the file \"%s\" (file exist?)", path_to_file);
        fclose(f);
        return 1;
    }

    // Get file size.
    unsigned long file_size = 0;
    fseek(f, 0, SEEK_END);
    {
        long size = ftell(f);
        if (size < 0) {
            printf("failed to get file size \"%s\"", path_to_file);
            fclose(f);
            return 1;
        }
        file_size = (unsigned long)size;
    }
    rewind(f);

    // Read content.
    char* file_content = malloc(sizeof(char) * (file_size + 1));
    const size_t bytes_read = fread(file_content, 1, file_size, f);
    file_content[file_size] = 0;
    if (bytes_read != file_size) {
        printf("failed to read the file \"%s\"", path_to_file);
        fclose(f);
        free(file_content);
        return 1;
    }
    fclose(f);

    int result = check_code(file_content, file_size, filename, path_to_file);

    free(file_content);
    processed_src_count += 1;
    return result;
}

int
process_directory(const char* path_to_dir) {
    DIR* dir = opendir(path_to_dir);
    if (dir == NULL) {
        printf("unable to open the directory \"%s\" (does path exist?)", path_to_dir);
        closedir(dir);
        return 1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' || entry->d_name[1] == '.') {
            continue;
        }

        int path_size = snprintf(NULL, 0, "%s/%s", path_to_dir, entry->d_name);
        if (path_size <= 0) {
            printf("snprintf error");
            closedir(dir);
            return 1;
        }

        char* path_to_entry = malloc(sizeof(char) * ((unsigned long)path_size + 2));
        path_to_entry[path_size + 1] = 0;
        snprintf(path_to_entry, (unsigned long)path_size + 1, "%s/%s", path_to_dir, entry->d_name);

        int result = 0;
        if (entry->d_type == DT_DIR) {
            result = process_directory(path_to_entry);
        } else {
            result = process_file(path_to_entry, (unsigned long)path_size, entry->d_name);
        }

        free(path_to_entry);
        if (result != 0) {
            closedir(dir);
            return result;
        }
    }

    closedir(dir);

    return 0;
}

int
main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("expected 1 argument");
        return 1;
    }
    const char* path_to_src = argv[1];

    const size_t path_len = strlen(path_to_src);
    if (path_to_src[path_len - 1] == '/') {
        printf("the specified path should not have a trailing forward slash");
        return 1;
    }

    processed_src_count = 0;
    processed_ref_count = 0;
    int result = process_directory(path_to_src);
    if (result != 0) {
        fflush(stdout);
        return result;
    }

    printf("checked %u source file(s) and %u ref(s)", processed_src_count, processed_ref_count);
    return 0;
}
