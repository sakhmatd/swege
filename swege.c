#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include <mkdio.h>
#include "stack.h"
#include "config.h"

#define D_NAME entry->d_name

void find_files(const char *src_path, const char *dst_path);
void make_dirs(Stack *dir_list);
void render_md(Stack *md_list);
void usage(void);

static Stack *dir_list;
static Stack *md_list;

static void
dir_err(const char *path)
{
        fprintf(stderr, "Cannot open directory '%s': %s\n", path, strerror(errno));
        exit(errno);
}

void find_files(const char *src_path, const char *dst_path)
{
        DIR *src = opendir(src_path);
        if (!src)
                dir_err(src_path);

        for (;;) {
                struct dirent *entry;
                char path[PATH_MAX];

                if(!(entry = readdir(src)))
                   break;

                if (strstr(D_NAME, ".md") != NULL) {
                        D_NAME[strlen(D_NAME) - 3] = 0; /* Remove the extention */
                        snprintf(path, PATH_MAX, "%s/%s.md", src_path, D_NAME);
                        push(md_list, path);
                        snprintf(path, PATH_MAX, "%s/%s.html", dst_path, D_NAME);
                        push(md_list, path);
                }

                if (entry->d_type & DT_DIR) {
                        if (strcmp(D_NAME, "..") != 0 &&
                            strcmp(D_NAME, ".") != 0  &&
                            strcmp(D_NAME, assets_folder) != 0) {
                                snprintf(path, PATH_MAX, "%s/%s", dst_path, D_NAME);
                                push(dir_list, path);
                                snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
                                find_files(path, dst_path);
                        }
                }
        }

        if (closedir(src)) {
                fprintf(stderr, "Cannot close directory '%s': %s\n", src_path, strerror(errno));
                exit(errno);
        }
}

void
make_dirs(Stack *dir_list)
{
        const char *path;

        while (dir_list->head != NULL) {
                path = pop(dir_list);
                mkdir(path, DEFFILEMODE);
        }
}

void
render_md(Stack *md_list)
{
        while (md_list->head != NULL) {
                const char *dst_path = pop(md_list);
                const char *path = pop(md_list);
                FILE *fd = fopen(path, "r");
                FILE *out = fopen(dst_path, "w");

                MMIOT *md = mkd_in(fd, 0);
                markdown(md, out, 0);

                fclose(fd);
                fclose(out);
        }
}

void
usage(void)
{
        fprintf(stderr, "Usage: swege src dst title\n");
        exit(1);
}

int
main(int argc, char *argv[])
{
        if (argc < 4)
                usage();

        const char *src_path = argv[1];
        const char *dst_path = argv[2];

        DIR *dptr;
        if ((dptr = opendir(src_path)) != NULL)
                closedir(dptr);
        else
                dir_err(src_path);

        if ((dptr = opendir(dst_path)) != NULL)
                closedir(dptr);
        else
                dir_err(dst_path);

        dir_list = new_stack();
        md_list = new_stack();

        find_files(src_path, dst_path);
        make_dirs(dir_list);
        render_md(md_list);

        free_stack(dir_list);
        free_stack(md_list);
}
