#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <mkdio.h>
#include "stack.h"
#include "config.h"

void find_files(const char *src_path);
void make_dirs(Stack *dir_list);
void render_md(Stack *md_list);
void usage(void);

static Stack dir_list;
static Stack md_list;

void find_files(const char *src_path)
{
        DIR *src = opendir(src_path);
        if (!src) {
                fprintf(stderr, "Cannot open directory '%s': %s", src_path, strerror(errno));
                exit(errno);
        }

        for (;;) {
                struct dirent *entry;
                char path[PATH_MAX];

                if(!(entry = readdir(src)))
                   break;

                if (strstr(entry->d_name, ".md") != NULL) {
                        snprintf(path, PATH_MAX, "%s/%s", src_path, entry->d_name);
                        push(&md_list, path);
                }

                if (entry->d_type & DT_DIR) {
                        if (strcmp(entry->d_name, "..") != 0 &&
                            strcmp(entry->d_name, ".") != 0  &&
                            strcmp(entry->d_name, assets_folder) != 0) {
                                snprintf(path, PATH_MAX, "%s/%s", src_path, entry->d_name);
                                push(&dir_list, path);
                                find_files(path);
                        }
                }
        }

        if (closedir(src)) {
                fprintf(stderr, "Cannot close directory '%s': %s", src_path, strerror(errno));
                exit(errno);
        }
}

void
make_dirs(Stack *dir_list) {
        const char *path;
        while (dir_list->head != NULL) {
                path = pop(dir_list);
                mkdir(path, DEFFILEMODE);
        }
}

int
main(int argc, char *argv[])
{
        if (argc < 3)
                usage();

        DIR *dst = opendir(argv[2]);

        const char *src_path = argv[1];

        find_files(src_path);

        if (!dst) {
                fprintf(stderr, "Cannot open directory '%s': %s", argv[2], strerror(errno));
                exit(errno);
        }

        FILE *fd = fopen("test.md", "r");
        FILE *out = fopen("out.html", "w");

        MMIOT *md = mkd_in(fd, 0);
        markdown(md, out, 0);

        fclose(fd);
        fclose(out);
}
