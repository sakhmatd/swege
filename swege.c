/* Copyright 2020 Sergei Akhmatdinov                                         */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License");           */
/* you may not use this file except in compliance with the License.          */
/* You may obtain a copy of the License at                                   */
/*                                                                           */
/*     http://www.apache.org/licenses/LICENSE-2.0                            */
/*                                                                           */
/* Unless required by applicable law or agreed to in writing, software       */
/* distributed under the License is distributed on an "AS IS" BASIS,         */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  */
/* See the License for the specific language governing permissions and       */
/* limitations under the License. */

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
#include "config.h"

#define BUF_SIZE 3145728
#define D_NAME entry->d_name

static void find_files(const char *src_path);
static void usage(void);
static void dir_err(const char *path);
static void file_err(const char *path);
static void stream_file(FILE *in, FILE *out);
static char* retrieve_title(FILE *in);
static char* mk_dst_path(const char *path);
static void render_md(char *path);
static void copy_file(const char *path);
static void log_file(const char *path);
static void make_dir(const char *path);
static long file_exists(const char *src_path);
static int path_in_manifest(const char *src_path);
static int file_is_newer(const char *src_path);

static FILE *manifest;
static long manifest_time = 0;

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
 * Streams contents of one file to another.
 * Not the fastest way to do it, there is overhead in moving
 * from user to kern space, but portable.
 **/
void
stream_file(FILE *in, FILE *out)
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
                line[strcspn(line, "\n")] = 0; /* Remove trailing newline */
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
log_file(const char *src_path)
{
        fprintf(manifest, "%s\n", src_path);
}

long
file_exists(const char *src_path)
{
        struct stat file_stat;
        if (stat(src_path, &file_stat) == 0)
                return file_stat.st_mtim.tv_sec;

        return 0;
}

int
dir_exists(const char *src_path)
{
        DIR *dptr;
        if ((dptr = opendir(src_path))) {
                closedir(dptr);
                return 1;
        }

        return 0;
}

int
path_in_manifest(const char *src_path)
{
        char *line = NULL;
        size_t len;
        int read;

        /* Edge case if .manifest was deleted */
        if (manifest_time == 0) {
                return 0;
        }

        rewind(manifest);

        while ((read = getline(&line, &len, manifest)) != -1) {
                line[strcspn(line, "\n")] = 0;
                if (!strcmp(line, src_path))
                        return 1;
        }

        return 0;
}

int
file_is_newer(const char *src_path)
{
        struct stat src_stats;

        stat(src_path, &src_stats);

        if (src_stats.st_mtim.tv_sec > manifest_time)
                return 1;

        return 0;
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

                /* Ignore emacs/vim autosave files */
                if(strstr(D_NAME, "~") || strstr(D_NAME, "#"))
                        continue;

                if (strstr(D_NAME, ".md")) {
                        snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
                        if (path_in_manifest(path)) {
                                if (file_is_newer(path)) {
                                        printf("%s\n", path);
                                        render_md(path);
                                }
                        } else {
                                log_file(path);
                                printf("%s\n", path);
                                render_md(path);
                        }
                } else if (entry->d_type & DT_DIR) {
                        if (strcmp(D_NAME, "..") != 0 &&
                            strcmp(D_NAME, ".") != 0) {
                                snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
                                if (path_in_manifest(path)) {
                                        snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
                                        find_files(path);
                                } else {
                                        make_dir(mk_dst_path(path));
                                        snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
                                        log_file(path);
                                        printf("%s\n", path);
                                        find_files(path);
                                }
                        }
                } else if (!strstr(footer_file, D_NAME) &&
                           !strstr(header_file, D_NAME) ) {
                        snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
                        if (path_in_manifest(path)) {
                                if (file_is_newer(path)) {
                                        printf("%s\n", path);
                                        copy_file(path);
                                }
                        } else {
                                log_file(path);
                                printf("%s\n", path);
                                copy_file(path);
                        }
                }
        }

        if (closedir(src)) {
                fprintf(stderr, "Cannot close directory '%s': %s\n", src_path, strerror(errno));
                exit(errno);
        }
}

void
render_md(char *path)
{
        FILE *header = fopen(header_file, "r");
        FILE *footer = fopen(footer_file, "r");

        FILE *fd = fopen(path, "r");

        if (!fd)
                file_err(path);
        if (!header)
                file_err(header_file);

        if (!footer)
                file_err(footer_file);


        path[strlen(path) - 3] = 0; /* Remove the extention */
        snprintf(path, PATH_MAX, "%s.html", mk_dst_path(path));

        FILE *out = fopen(path, "w");

        if (!out)
                file_err(path);

        stream_file(header, out);

        const char *page_title;
        if ((page_title = retrieve_title(fd)))
                fprintf(out, "<title>%s - %s</title>\n", page_title, site_title);
        else
                fprintf(out, "<title>%s</title>\n", site_title);

        MMIOT *md = mkd_in(fd, 0);
        markdown(md, out, 0);

        stream_file(footer, out);

        fclose(header);
        fclose(footer);
        fclose(fd);
        fclose(out);
}

void
make_dir(const char *path)
{
        mode_t default_mode = umask(0);
        mkdir(path, 0755);
        umask(default_mode);
}

void
copy_file(const char *path)
{
        FILE *orig = fopen(path, "r");
        FILE *new = fopen(mk_dst_path(path), "w");

        if (!orig)
                file_err(path);
        if (!new)
                file_err(mk_dst_path(path));

        stream_file(orig, new);
}

void
usage(void)
{
        char *usage_str = "swege 0.4\n\
By default, swege reads markdown files from the 'src' directory and renders\n\
them as HTML to the 'site' directory, copying over any non-markdown files.\n";
        fprintf(stderr, "%s", usage_str);
        exit(errno);
}

int
main(int argc, char *argv[])
{
        (void)argv; /* Suppress compiler warning about unused argv */
        if (argc >= 2)
                usage();

        /* Check if destination directory exists and create it if it doesn't. */
        mode_t default_mode = umask(0);
        if (!dir_exists(dst_dir))
                mkdir(dst_dir, 0755);
        umask(default_mode);

        /* mtime gets updated with fopen, so we need to save previous mtime */
        /* File exists returns current mtime of the file or 0 if it doesn't exist */
        manifest_time = file_exists(".manifest");
        if (manifest_time) {
                manifest = fopen(".manifest", "a+");
        } else {
                manifest = fopen(".manifest", "w+");
        }

        find_files(src_dir);

        fclose(manifest);
}
