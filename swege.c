/* Copyright 2020 Sergei Akhmatdinov					     */
/*									     */
/* Licensed under the Apache License, Version 2.0 (the "License");	     */
/* you may not use this file except in compliance with the License.	     */
/* You may obtain a copy of the License at				     */
/*									     */
/*     http://www.apache.org/licenses/LICENSE-2.0			     */
/*									     */
/* Unless required by applicable law or agreed to in writing, software	     */
/* distributed under the License is distributed on an "AS IS" BASIS,	     */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.	 */
/* See the License for the specific language governing permissions and			 */
/* limitations under the License. */

/* Includes */
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <mkdio.h>
#include "ini.h"

/* For copy_file() */
#ifdef __APPLE__
#include <copyfile.h>
#elif defined(__linux__)
#include <sys/sendfile.h>
#endif /* __APPLE__ */

/* Constants and Macros */
#define BUF_SIZE 3145728 /* Change if your stack is smaller */
#define TITLE_SIZE 50
#define D_NAME entry->d_name

#define MANIFESTF ".manifest"

#define INISECTION "swege"
#define INIFILE "swege.ini"

/* Simplify read_config() */
#define MATCH(n) strcmp(section, INISECTION) == 0 && strcmp(name, n) == 0

/* Typedefs */
typedef struct {
	char *site_title;

	char *src_dir;
	char *dst_dir;

	char *header_file;
	char *footer_file;
} Config;

/* Prototypes */
static void usage(void);
static void dir_err(const char *path);
static void file_err(const char *path);
static void copy_err(const char *path);

static int stream_to_file(const char *path, const char *new_path);
static int copy_file(const char *path, const char *new_path);
static void log_file(const char *path);
static void make_dir(const char *path);

static long file_exists(const char *src_path);
static int dir_exists(const char *src_path);
static int file_is_newer(const char *src_path);
static int path_in_manifest(const char *src_path);

static char *retrieve_title(FILE * in);
static char *mk_dst_path(const char *path);

static void find_files(const char *src_path);
static void render_md(char *path);
static int read_config(void *user, const char *section, const char *name,
		       const char *value);

/* Global variables */
static FILE *manifest;
static long manifest_time = 0;
static int head_foot_updated = 0;
static int files_procd = 0;
static Config config;

void
usage(void) 
{
	char *usage_str = "swege 1.1.0\n"
		"Please see https://github.com/sakhmatd/swege"
		" for detailed usage instructions.\n";
	fprintf(stderr, "%s", usage_str);
	exit(errno);
}

inline void
dir_err(const char *path) 
{
	fprintf(stderr, "Cannot open directory '%s': %s\n", path,
		strerror(errno));
	exit(errno);
}

inline void
file_err(const char *path) 
{
	fprintf(stderr, "Cannot access file '%s': %s\n", path,
		strerror(errno));
	exit(errno);
}

inline void
copy_err(const char *path)
{
	fprintf(stderr, "Failed copying file '%s': %s.\n",
			path, strerror(errno));

	exit(errno);
}

/* 70's style read-write loop
 * Used for adding headers and footers 
 * or as a fallback copy */
int
stream_to_file(const char *path, const char *new_path)
{
	int orig, new;

	orig = open(path, O_RDONLY);
        new = open(new_path, O_WRONLY | O_APPEND);
	
	char buf[BUF_SIZE] = {0};
        size_t bytes = 0;

        while ((bytes = read(orig, &buf, BUF_SIZE)) > 0) {
                if (write(new, &buf, bytes) < 0) {
                        return 0;
                }
	}

	return 1;
}

int
copy_file(const char *path, const char *new_path)
{
	int orig, new;
	int ret = 0;

	orig = open(path, O_RDONLY);
	if (orig < 0)
		file_err(path);

	/* TODO: Preserve permissions */
	new = open(new_path, O_CREAT | O_TRUNC | O_WRONLY, 0660);

	if (new < 0)
		file_err(new_path);

#ifdef __APPLE__
	ret = fcopyfile(orig, new, 0, COPYFILE_ALL);
#elif defined(__linux__) || __FreeBSD_version >= 1300038
	struct stat filestat;
	fstat(orig, &filestat);

	#ifdef __linux__
	/* copy_file_range requires _GNU_SOURCE, so sendfile instead */
	off_t bytes_written = 0;
	ret = sendfile(new, orig, &bytes_written, filestat.st_size);
	#elif defined(__FreeBSD__)
	/* copy_file_range added in 13-CURRENT, haven't tested yet */
	ret = copy_file_range(orig, 0, new, 0, filestat.st_size, 0);
	#endif /* __linux__ */

#else
	/* 70's style as a fallback */
	ret = stream_to_file(path, new_path);	

#endif /* __APPLE__ */

	if (ret < 0)
		copy_err(path);

	close(orig);
	close(new);
	files_procd++;
	return ret;
}

inline void
log_file(const char *src_path) 
{
	fprintf(manifest, "%s\n", src_path);
}

void
make_dir(const char *path)
{
	mode_t default_mode = umask(0);
	mkdir(path, 0755);
	umask(default_mode);
	files_procd++;
}

long
file_exists(const char *src_path) 
{
	struct stat file_stat;
	if (stat(src_path, &file_stat) == 0)
		return file_stat.st_mtime;

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
file_is_newer(const char *src_path) 
{
	struct stat src_stats;

	stat(src_path, &src_stats);

	if (src_stats.st_mtime > manifest_time)
		return 1;

	return 0;
}

int
path_in_manifest(const char *src_path) 
{
	char *line = NULL;
	char *line_cpy;
	size_t len;
	int read;

	/* Edge case if .manifest was deleted */
	if (manifest_time == 0) {
		return 0;
	}

	rewind(manifest);

	while ((read = getline(&line, &len, manifest)) != -1) {
		line_cpy = line;
		line_cpy[strcspn(line_cpy, "\n")] = 0;
		if (!strcmp(line_cpy, src_path)) {
			free(line);
			return 1;
		}
	}

	free(line);
	return 0;
}

/**
 * Grabs the first line of the md file.
 * Extract the title if explicitly given or use the first H1 found as the title.
 * Empty first line would result in no title for the page.
 */
char *
retrieve_title(FILE * in) 
{
	char *line = NULL;
	size_t linecap = 0;
	getline(&line, &linecap, in);

	if (!line) {
		fprintf(stderr, "Can't allocate memory: line\n");
		exit(errno);
	}

	char *ret = malloc(sizeof(char) * (TITLE_SIZE + 1));
	if (!ret) {
		fprintf(stderr, "Can't allocate memory: ret\n");
		exit(errno);
	}

	if (strstr(line, "title: ")) {
		char *piece = NULL;
		char *line_cpy = line;	/* strsep modifies argument */
		ret[0] = '\0';	/* Ensure ret is an empty string */

		while ((piece = strsep(&line_cpy, " "))) {
			if (strcmp("title:", piece)) {
				strncat(ret, piece,
					(TITLE_SIZE - strlen(ret)));
				strncat(ret, " ", (TITLE_SIZE - strlen(ret)));
			}
		}

		/* Remove trailing newline and whitespace */
		*strrchr(ret, ' ') = 0;
		ret[strcspn(ret, "\n")] = 0;
	} else if (strlen(line) >= 3 && line[0] == '#') {
		strncpy(ret, line + 2, (TITLE_SIZE + 1));	/* Copy line without the # */
		ret[TITLE_SIZE] = '\0';	/* Ensure termination */

		ret[strcspn(ret, "\n")] = 0;	/* Remove trailing newline */
		rewind(in);	/* Leave H1 in place for rendering */
	} else {
		free(line);
		free(ret);
		return NULL;
	}

	free(line);
	return ret;
}

char *
mk_dst_path(const char *path) 
{
	static char ret[PATH_MAX];
	size_t namelen = strlen(config.src_dir);

	path += namelen;
	snprintf(ret, PATH_MAX, "%s%s", config.dst_dir, path);

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

		if (!(entry = readdir(src)))
			break;

		/* Ignore emacs/vim autosave files */
		if (strstr(D_NAME, "~") || strstr(D_NAME, "#"))
			continue;

		if (strstr(D_NAME, ".md")) {
			snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
			if (path_in_manifest(path)) {
				if (file_is_newer(path) || head_foot_updated) {
					printf("%s\n", path);
					render_md(path);
				}
			} else {
				log_file(path);
				printf("%s\n", path);
				render_md(path);
			}
		} else if (entry->d_type & DT_DIR) {
			if (strcmp(D_NAME, "..") != 0
			    && strcmp(D_NAME, ".") != 0) {
				snprintf(path, PATH_MAX, "%s/%s", src_path,
					 D_NAME);
				if (path_in_manifest(path)) {
					snprintf(path, PATH_MAX, "%s/%s",
						 src_path, D_NAME);
					find_files(path);
				} else {
					make_dir(mk_dst_path(path));
					snprintf(path, PATH_MAX, "%s/%s",
						 src_path, D_NAME);
					log_file(path);
					printf("%s\n", path);
					find_files(path);
				}
			}
		} else if (!strstr(config.footer_file, D_NAME) &&
			   !strstr(config.header_file, D_NAME)) {
			snprintf(path, PATH_MAX, "%s/%s", src_path, D_NAME);
			if (path_in_manifest(path)) {
				if (file_is_newer(path)) {
					printf("%s\n", path);
					copy_file(path, mk_dst_path(path));
				}
			} else {
				log_file(path);
				printf("%s\n", path);
				copy_file(path, mk_dst_path(path));
			}
		}
	}

	if (closedir(src)) {
		fprintf(stderr, "Cannot close directory '%s': %s\n", src_path,
			strerror(errno));
		exit(errno);
	}
}

void
render_md(char *path) 
{
	FILE *fd = fopen(path, "r");
	if (!fd)
		file_err(path);

	path[strlen(path) - 3] = 0;	/* Remove the extention */
	snprintf(path, PATH_MAX, "%s.html", mk_dst_path(path));

	FILE *out = fopen(path, "a");

	if (!out)
		file_err(path);

        /* Append the header */
	stream_to_file(config.header_file, path);

	char *page_title = retrieve_title(fd);
	if (page_title) {
		fprintf(out, "<title>%s - %s</title>\n", page_title,
			config.site_title);
	} else {
		fprintf(out, "<title>%s</title>\n", config.site_title);
	}

	free(page_title);

	MMIOT *md = mkd_in(fd, 0);
	markdown(md, out, 0);

	fclose(fd);
	fclose(out);

        /* Append the footer */
	stream_to_file(config.footer_file, path);

	files_procd++;
}

int
read_config(void *user, const char *section, const char *name,
	    const char *value) 
{
	Config *pconfig = (Config *) user;

	if (MATCH("title")) {
		pconfig->site_title = strdup(value);
	} else if (MATCH("source")) {
		pconfig->src_dir = strdup(value);
	} else if (MATCH("destination")) {
		pconfig->dst_dir = strdup(value);
	} else if (MATCH("header")) {
		pconfig->header_file = strdup(value);
	} else if (MATCH("footer")) {
		pconfig->footer_file = strdup(value);
	} else {
		return 0;	/* unknown section/name, error */
	}

	return 1;
}

int
main(int argc, char *argv[]) 
{
	(void)argv;	/* Suppress compiler warning about unused argv */
	if (argc >= 2)
		usage();

	if (ini_parse(INIFILE, read_config, &config) < 0) {
		printf("Can't load 'swege.ini'\n");
		return 1;
	}

	/* Check if destination directory exists and create it if it doesn't. */
	mode_t default_mode = umask(0);
	if (!dir_exists(config.dst_dir))
		mkdir(config.dst_dir, 0755);
	umask(default_mode);

	/* mtime gets updated with fopen, so we need to save previous mtime */
	/* File exists returns current mtime of the file or 0 if it doesn't exist */
	manifest_time = file_exists(MANIFESTF);

	/* Force updating manifest if footer or header files are updated */
	head_foot_updated = (file_is_newer(config.footer_file) ||
			     file_is_newer(config.header_file));

	if (manifest_time) {
		manifest = fopen(MANIFESTF, "a+");
		futimens(fileno(manifest), NULL);	/* update manifest mtime */
	} else {
		manifest = fopen(MANIFESTF, "w+");
	}

	find_files(config.src_dir);

	if (files_procd)
		printf("%d files/directories processed.\n", files_procd);
	else
		printf("No changes or new files detected, site is up to date.\n");

	fclose(manifest);
}
