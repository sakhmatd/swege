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

#define BUF_SIZE 3145728
#define D_NAME entry->d_name

static void find_files(const char *src_path);
static void copy_files(void);
static void render_md(void);
static void usage(void);
static void dir_err(const char *path);
static void file_err(const char *path);
static void file_cpy(FILE *in, FILE *out);
static char* retrieve_title(FILE *in);
static char* mk_dst_path(const char *path);
static void find_files(const char *src_path);
static void render_md(void);
static void copy_files(void);

static Stack *md_stack;
static Stack *file_stack;

void
dir_err(const char *path)
{
        fprintf(stderr, "Cannot open directory '%s': %s\n", path, strerror(errno));
        usage();
}

void
file_err(const char *path)
{
        fprintf(stderr, "Cannot access file '%s': %s\n", path, strerror(errno));
        usage();
}

/**
 * Copies contents of one file to another.
 * Not the fastest way to do it, there is overhead in moving
 * from user to kern space, but portable.
 **/
void
file_cpy(FILE *in, FILE *out)
{
        char buf[BUF_SIZE];
        size_t bytes;

        rewind(in);
        while ((bytes = fread(buf, 1, sizeof(buf), in)) > 0)
                fwrite(buf, 1, bytes, out);
}

char*
retrieve_title(FILE *in)
{
        char *line = NULL;
        size_t linecap = 0;
        getline(&line, &linecap, in);

        if (line && strlen(line) >= 3 && line[0] == '#') {
                line += 2; /* Remove the '#' and the space after it */
                line[strlen(line) - 1] = 0; /* Remove trailing newline */
        } else {
                line = NULL;
        }

        rewind(in);
        return line;
}

char*
mk_dst_path(const char *path)
{
        static char ret[PATH_MAX];
        size_t namelen = strlen(src_dir);

        path += namelen;
        snprintf(ret, PATH_MAX, "%s%s", dst_dir, path);

        return ret;
}

void
find_files(const char *src_path)
{
        DIR *src = opendir(src_path);
        if (!src)
                dir_err(src_path);

        for (;;) {
                struct dirent *entry;
                char path[PATH_MAX];

                if(!(entry = readdir(src)))
                   break;

                if (strstr(D_NAME, ".md")) {
                        D_NAME[strlen(D_NAME) - 3] = 0; /* Remove the extention */
                        snprintf(path, PATH_MAX, "%s/%s.md", src_path, D_NAME);
                        push(md_stack, path);
                        snprintf(path, PATH_MAX, "%s/%s.html", mk_dst_path(src_path), D_NAME);
                        push(md_stack, path);
                } else if (entry->d_type & DT_DIR) {
                        if (strcmp(D_NAME, "..") != 0 &&
                            strcmp(D_NAME, ".") != 0) {
                                mode_t default_mode = umask(0);
                                snprintf(path, PATH_MAX, "%s/%s", mk_dst_path(src_path), D_NAME);
                                mkdir(path, 0755);
                                umask(default_mode);
                                snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
                                find_files(path);
                        }
                } else if (!strstr(footer_file, D_NAME) &&
                           !strstr(header_file, D_NAME) ) {
                        snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
                        push(file_stack, path);
                }
        }

        if (closedir(src)) {
                fprintf(stderr, "Cannot close directory '%s': %s\n", src_path, strerror(errno));
                exit(errno);
        }
}

void
render_md(void)
{
        FILE *header = fopen(header_file, "r");
        FILE *footer = fopen(footer_file, "r");

        while (!is_empty(md_stack)) {
                const char *dst_path = pop(md_stack);
                const char *path = pop(md_stack);
                const char *page_title;
                FILE *fd = fopen(path, "r");
                FILE *out = fopen(dst_path, "w");

                if (!fd)
                        file_err(path);
                if (!out)
                        file_err(dst_path);

                file_cpy(header, out);

                if ((page_title = retrieve_title(fd)))
                        fprintf(out, "<title>%s - %s<title>\n", page_title, site_title);
                else
                        fprintf(out, "<title>%s</title>\n", site_title);

                MMIOT *md = mkd_in(fd, 0);
                markdown(md, out, 0);

                file_cpy(footer, out);

                fclose(fd);
                fclose(out);
        }
}

void
copy_files(void)
{
        while (!is_empty(file_stack)) {
                const char *path = pop(file_stack);
                FILE *orig = fopen(path, "r");
                FILE *new = fopen(mk_dst_path(path), "w");

                if (!orig)
                        file_err(path);
                if (!new)
                        file_err(mk_dst_path(path));

                file_cpy(orig, new);
        }
}

void
usage(void)
{
        char *usage_str = "swege 0.2\n\
By default, swege reads markdown files from the 'src' directory and renders\n\
them as HTML to the 'site' directory, copying over any non-markdown files.\n";
        fprintf(stderr, "%s", usage_str);
        exit(errno);
}

int
main(int argc, char *argv[])
{
        if (argc >= 2)
                usage();

        md_stack = new_stack();
        file_stack = new_stack();

        /* Check if destination directory exists and create it if it doesn't. */
        mode_t default_mode = umask(0);
        DIR *dptr;
        if ((dptr = opendir(dst_dir)))
                closedir(dptr);
        else {
                mkdir(dst_dir, 0755);
        }
        umask(default_mode);

        find_files(src_dir);
        copy_files();
        render_md();

        free_stack(md_stack);
        free_stack(file_stack);
}
