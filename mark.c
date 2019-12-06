#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <mkdio.h>
#include "stack.h"

void find_files(DIR *src, DIR *dst);
void usage(void);

static Stack dir_list;
static Stack md_list;

void find_files(DIR *src, DIR *dst)
{
        
}

int
main(int argc, char *argv[])
{
        if (argc < 3)
                usage();

        DIR *src = opendir(argv[1]);
        DIR *dst = opendir(argv[2]);

        if (!src) {
                fprintf(stderr, "Cannot open directory '%s': %s", argv[1], strerror(errno));
                exit(errno);
        }

        if (!dst) {
                fprintf(stderr, "Cannot open directory '%s': %s", argv[2], strerror(errno));
                exit(errno);
        }

        find_files(src, dst);

        FILE *fd = fopen("test.md", "r");
        FILE *out = fopen("out.html", "w");

        MMIOT *md = mkd_in(fd, 0);
        markdown(md, out, 0);

        fclose(fd);
        fclose(out);
}
