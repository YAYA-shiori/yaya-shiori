/*
 * POSIX smoke test for yaya (libaya5.so / libyaya.bundle)
 *
 * Usage: posix_smoke <path to library> <empty directory>
 *
 * dlopen() the built library and run SHIORI load / request / unload.
 * Loading an empty directory makes yaya start in shell mode, which EVALs
 * the request text and returns the result, so no dictionary is needed.
 *
 * The directory may have a non-ASCII (UTF-8) name; the tests also create files
 * with Japanese names in it, and check character set conversions.
 *
 * The library path must contain '/' (otherwise dlopen searches the library path).
 * This file is ASCII only: non-ASCII text is written as UTF-8 \x escapes.
 * The expected values of the common cases are the results of yaya.exe on Windows.
 */
#include <dlfcn.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int (*load_fn)(char *h, long len);
typedef int (*unload_fn)(void);
typedef char *(*request_fn)(char *h, long *len);
typedef long (*multi_load_fn)(char *h, long len);
typedef int (*multi_unload_fn)(long id);

static request_fn p_request;
static int failures;

struct test_case {
	const char *label;
	const char *expr;
	const char *expected;
};

/*
 * Cases that behave the same on Windows.
 * "\xE6\x97\xA5\xE6\x9C\xAC" is "nihon" (2 kanji), "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt" is "nihongo.txt",
 * "\xE3\x82\xB5\xE3\x83\x96" is "sabu" (a subdirectory), "\xEF\xBD\x9E" is FULLWIDTH TILDE.
 */
static const struct test_case common_cases[] = {
	{ "add/mul", "1+2*3", "7" },
	{ "string concat", "\"ab\"+\"cd\"", "abcd" },
	{ "REPLACE", "REPLACE(\"hello\",\"l\",\"L\")", "heLLo" },
	{ "while loop", "_s = 0\n_i = 0\nwhile _i < 5 {\n _s += _i\n _i++\n}\n_s", "10" },
	{ "utf8 concat",
	  "\"\xE3\x81\x82\xE3\x81\x84\"+\"\xE3\x81\x86\"",
	  "\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86" },
	{ "STRLEN",
	  "STRLEN(\"\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\")",
	  "3" },
	{ "enc sjis",
	  "STRENCODE(\"\xE6\x97\xA5\xE6\x9C\xAC\",\"Shift_JIS\")",
	  "%93%FA%96%7B" },
	{ "enc eucjp",
	  "STRENCODE(\"\xE6\x97\xA5\xE6\x9C\xAC\",\"EUC-JP\")",
	  "%C6%FC%CB%DC" },
	{ "enc jis",
	  "STRENCODE(\"\xE6\x97\xA5\xE6\x9C\xAC\",\"ISO-2022-JP\")",
	  "%1B%24BF%7CK%5C%1B%28B" },
	{ "enc tilde",
	  "STRENCODE(\"\xEF\xBD\x9E\",\"Shift_JIS\")",
	  "%81%60" },
	{ "enc big5",
	  "STRENCODE(\"\xE4\xB8\xAD\xE6\x96\x87\",\"BIG5\")",
	  "%A4%A4%A4%E5" },
	{ "enc gb",
	  "STRENCODE(\"\xE4\xB8\xAD\xE6\x96\x87\",\"GB2312\")",
	  "%D6%D0%CE%C4" },
	{ "enc kr",
	  "STRENCODE(\"\xED\x95\x9C\xEA\xB8\x80\",\"EUC-KR\")",
	  "%C7%D1%B1%DB" },
	{ "enc unconv",
	  "STRENCODE(\"A\xED\x95\x9C" "B\",\"Shift_JIS\")",
	  "A%3FB" },
	{ "dec sjis",
	  "STRDECODE(\"%93%FA%96%7B\",\"Shift_JIS\")",
	  "\xE6\x97\xA5\xE6\x9C\xAC" },
	{ "dec tilde",
	  "STRDECODE(\"%81%60\",\"Shift_JIS\")",
	  "\xEF\xBD\x9E" },
	{ "dec jis",
	  "STRDECODE(STRENCODE(\"\xE6\x97\xA5\xE6\x9C\xAC\",\"ISO-2022-JP\"),\"ISO-2022-JP\")",
	  "\xE6\x97\xA5\xE6\x9C\xAC" },
	{ "fwrite sjis",
	  "_d = FCHARSET(\"Shift_JIS\")\n_d = FOPEN(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\",\"w\")\n_d = FWRITE2(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\",\"\xE6\x97\xA5\xE6\x9C\xAC\")\n_d = FCLOSE(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")\nFSIZE(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")",
	  "4" },
	{ "fread sjis",
	  "_d = FCHARSET(\"Shift_JIS\")\n_d = FOPEN(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\",\"r\")\n_s = FREAD(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")\n_d = FCLOSE(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")\n_s",
	  "\xE6\x97\xA5\xE6\x9C\xAC" },
	{ "MKDIR",
	  "MKDIR(\"\xE3\x82\xB5\xE3\x83\x96\")",
	  "1" },
	{ "FENUM dir",
	  "ASEARCH(\"\\\xE3\x82\xB5\xE3\x83\x96\",SPLIT(FENUM(\".\"), \",\")) >= 0",
	  "1" },
	{ "FENUM file",
	  "ASEARCH(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\", SPLIT(FENUM(\".\"), \",\")) >= 0",
	  "1" },
	{ "FCOPY to dir",
	  "FCOPY(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\",\"\xE3\x82\xB5\xE3\x83\x96\")",
	  "1" },
	{ "FCOPY result",
	  "FSIZE(\"\xE3\x82\xB5\xE3\x83\x96/\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")",
	  "4" },
	{ "FCOPY self",
	  "FCOPY(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\",\".\")",
	  "0" },
	{ "FCOPY self kept",
	  "FSIZE(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")",
	  "4" },
	{ "FMOVE exists",
	  "FMOVE(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\",\"\xE3\x82\xB5\xE3\x83\x96\")",
	  "0" },
	{ "FMOVE to dir",
	  "_d = FDEL(\"\xE3\x82\xB5\xE3\x83\x96/\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")\nFMOVE(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\",\"\xE3\x82\xB5\xE3\x83\x96\")",
	  "1" },
	{ "FMOVE result",
	  "FSIZE(\"\xE3\x82\xB5\xE3\x83\x96/\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")",
	  "4" },
	{ "FMOVE back",
	  "FMOVE(\"\xE3\x82\xB5\xE3\x83\x96/\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\",\".\")",
	  "1" },
	{ "FMOVE back result",
	  "FSIZE(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")",
	  "4" },
	{ "FDEL dir",
	  "FDEL(\"\xE3\x82\xB5\xE3\x83\x96\")",
	  "0" },
	{ "RMDIR file",
	  "RMDIR(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")",
	  "0" },
	{ "RMDIR not empty",
	  "_d = FCOPY(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\",\"\xE3\x82\xB5\xE3\x83\x96\")\nRMDIR(\"\xE3\x82\xB5\xE3\x83\x96\")",
	  "0" },
	{ "FDEL file",
	  "FDEL(\"\xE3\x82\xB5\xE3\x83\x96/\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")",
	  "1" },
	{ "RMDIR dir",
	  "RMDIR(\"\xE3\x82\xB5\xE3\x83\x96\")",
	  "1" },
	{ "FATTRIB file",
	  "_a = FATTRIB(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")\n_a[2] + _a[3] * 10 + _a[6] * 100",
	  "0" },
	{ "FATTRIB dir",
	  "_a = FATTRIB(\".\")\n_a[2]",
	  "1" },
	{ "FATTRIB ctime",
	  "_a = FATTRIB(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")\n_a[9] > 0 && _a[10] > 0",
	  "1" },
	{ "GETTICKCOUNT",
	  "_a = GETTICKCOUNT()\n_b = GETTICKCOUNT()\n_a > 0 && _b >= _a",
	  "1" },
};

/* Cases for POSIX only (files made by setup_dir(), processes, signals) */
static const struct test_case posix_cases[] = {
	{ "FATTRIB normal", "_a = FATTRIB(\"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt\")\n_a[4]", "1" },
	{ "FATTRIB hidden", "_a = FATTRIB(\".hidden\")\n_a[3] * 10 + _a[4]", "10" },
	{ "FENUM symlink to dir", "ASEARCH(\"\\link\", SPLIT(FENUM(\".\"), \",\")) >= 0", "1" },
	{ "EXECUTE_WAIT exit code", "EXECUTE_WAIT(\"sh\", \"-c 'exit 3'\")", "3" },
	{ "EXECUTE_WAIT full path", "EXECUTE_WAIT(\"/bin/sh\", \"-c 'exit 4'\")", "4" },
	{ "EXECUTE_WAIT dir and UTF-8 args", "EXECUTE_WAIT(\"sh\", \"-c 'test -f \xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt'\", \".\")", "0" },
};

/* Send one request and compare the response with the expected string. */
static void check(const char *label, const char *expr, const char *expected)
{
	size_t n = strlen(expr);
	char *h = (char *)malloc(n + 1);
	long len = (long)n;
	char *res;

	if (h == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(2);
	}
	memcpy(h, expr, n + 1);

	/* yaya does not free the request buffer on success, so it is not freed here either */
	res = p_request(h, &len);

	if (res == NULL) {
		printf("NG  %s: request returned NULL (expected \"%s\")\n", label, expected);
		failures++;
		return;
	}
	if ((size_t)len == strlen(expected) && memcmp(res, expected, (size_t)len) == 0) {
		printf("OK  %s: \"%s\"\n", label, expected);
	}
	else {
		printf("NG  %s: got \"%.*s\" (expected \"%s\")\n", label, (int)len, res, expected);
		failures++;
	}
	free(res);
}

static void check_cases(const struct test_case *cases, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++) {
		check(cases[i].label, cases[i].expr, cases[i].expected);
	}
}

static void ok_or_ng(const char *label, int ok)
{
	printf("%s  %s\n", ok ? "OK" : "NG", label);
	if (!ok) {
		failures++;
	}
}

static char *join_path(const char *dir, const char *name)
{
	size_t n = strlen(dir) + strlen(name) + 2;
	char *p = (char *)malloc(n);
	if (p == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(2);
	}
	snprintf(p, n, "%s/%s", dir, name);
	return p;
}

static void write_file(const char *path, const char *text)
{
	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		perror(path);
		exit(2);
	}
	fputs(text, fp);
	fclose(fp);
}

/* Files used by posix_cases: a hidden file, and a symbolic link to a directory */
static void setup_dir(const char *dir)
{
	char *p;

	p = join_path(dir, ".hidden");
	write_file(p, "x");
	free(p);

	p = join_path(dir, "real");
	if (mkdir(p, 0755) != 0) {
		perror(p);
		exit(2);
	}
	free(p);

	p = join_path(dir, "link");
	if (symlink("real", p) != 0) {
		perror(p);
		exit(2);
	}
	free(p);
}

/* The file written with FCHARSET("Shift_JIS") must contain Shift_JIS bytes */
static void check_sjis_file(const char *dir)
{
	static const char expected[] = "\x93\xFA\x96\x7B";
	char buf[16];
	size_t n = 0;
	char *p = join_path(dir, "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.txt");
	FILE *fp = fopen(p, "rb");

	if (fp != NULL) {
		n = fread(buf, 1, sizeof(buf), fp);
		fclose(fp);
	}
	free(p);
	ok_or_ng("file written in Shift_JIS", fp != NULL && n == 4 && memcmp(buf, expected, 4) == 0);
}

static char *dup_buffer(const char *s)
{
	size_t n = strlen(s);
	char *p = (char *)malloc(n + 1);
	if (p == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(2);
	}
	memcpy(p, s, n + 1);
	return p;
}

int main(int argc, char **argv)
{
	void *lib;
	load_fn p_load;
	unload_fn p_unload;
	multi_load_fn p_multi_load;
	multi_unload_fn p_multi_unload;
	char *path;
	char *missing;
	long id;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <library> <empty directory>\n", argv[0]);
		return 2;
	}

	setup_dir(argv[2]);

	lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (lib == NULL) {
		fprintf(stderr, "dlopen failed: %s\n", dlerror());
		return 1;
	}
	printf("OK  dlopen: %s\n", argv[1]);

	p_load = (load_fn)dlsym(lib, "load");
	p_unload = (unload_fn)dlsym(lib, "unload");
	p_request = (request_fn)dlsym(lib, "request");
	p_multi_load = (multi_load_fn)dlsym(lib, "multi_load");
	p_multi_unload = (multi_unload_fn)dlsym(lib, "multi_unload");
	if (p_load == NULL || p_unload == NULL || p_request == NULL || p_multi_load == NULL || p_multi_unload == NULL) {
		fprintf(stderr, "dlsym failed: %s\n", dlerror());
		return 1;
	}

	/* Internal functions must not be exported (built with -fvisibility=hidden) */
	ok_or_ng("sqlite3_open is not exported", dlsym(lib, "sqlite3_open") == NULL);
	ok_or_ng("json_parse_string is not exported", dlsym(lib, "json_parse_string") == NULL);
	ok_or_ng("gumbo_parse is not exported", dlsym(lib, "gumbo_parse") == NULL);

	/* load() takes ownership of the buffer and frees it */
	path = dup_buffer(argv[2]);
	if (!p_load(path, (long)strlen(argv[2]))) {
		fprintf(stderr, "load failed\n");
		return 1;
	}
	printf("OK  load: %s\n", argv[2]);

	check_cases(common_cases, sizeof(common_cases) / sizeof(common_cases[0]));
	check_sjis_file(argv[2]);
	check_cases(posix_cases, sizeof(posix_cases) / sizeof(posix_cases[0]));

	/* The exit code must be returned even if the host ignores SIGCHLD */
	signal(SIGCHLD, SIG_IGN);
	check("EXECUTE_WAIT with SIGCHLD ignored", "EXECUTE_WAIT(\"sh\", \"-c 'exit 3'\")", "3");
	signal(SIGCHLD, SIG_DFL);

	p_unload();
	printf("OK  unload\n");

	/* A directory that does not exist must not terminate the host */
	missing = join_path(argv[2], "missing/");
	id = p_multi_load(missing, (long)strlen(missing));
	ok_or_ng("multi_load with a missing directory", id > 0);
	ok_or_ng("multi_unload out of range", p_multi_unload(id + 1) == 0);
	ok_or_ng("multi_unload", id <= 0 || p_multi_unload(id) == 1);

	if (dlclose(lib) != 0) {
		fprintf(stderr, "dlclose failed: %s\n", dlerror());
		return 1;
	}
	printf("OK  dlclose\n");

	if (failures != 0) {
		printf("%d check(s) failed\n", failures);
		return 1;
	}
	printf("smoke test passed\n");
	return 0;
}
