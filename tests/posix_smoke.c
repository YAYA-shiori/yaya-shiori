/*
 * POSIX smoke test for yaya (libaya5.so / libyaya.bundle)
 *
 * Usage: posix_smoke <path to library> <empty directory>
 *
 * dlopen() the built library and run SHIORI load / request / unload.
 * Loading an empty directory makes yaya start in shell mode, which EVALs
 * the request text and returns the result, so no dictionary is needed.
 *
 * The library path must contain '/' (otherwise dlopen searches the library path).
 * This file is ASCII only.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*load_fn)(char *h, long len);
typedef int (*unload_fn)(void);
typedef char *(*request_fn)(char *h, long *len);

static request_fn p_request;
static int failures;

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

int main(int argc, char **argv)
{
	void *lib;
	load_fn p_load;
	unload_fn p_unload;
	size_t n;
	char *path;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <library> <empty directory>\n", argv[0]);
		return 2;
	}

	lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (lib == NULL) {
		fprintf(stderr, "dlopen failed: %s\n", dlerror());
		return 1;
	}
	printf("OK  dlopen: %s\n", argv[1]);

	p_load = (load_fn)dlsym(lib, "load");
	p_unload = (unload_fn)dlsym(lib, "unload");
	p_request = (request_fn)dlsym(lib, "request");
	if (p_load == NULL || p_unload == NULL || p_request == NULL) {
		fprintf(stderr, "dlsym failed: %s\n", dlerror());
		return 1;
	}

	/* load() takes ownership of the buffer and frees it */
	n = strlen(argv[2]);
	path = (char *)malloc(n + 1);
	if (path == NULL) {
		fprintf(stderr, "out of memory\n");
		return 2;
	}
	memcpy(path, argv[2], n + 1);
	if (!p_load(path, (long)n)) {
		fprintf(stderr, "load failed\n");
		return 1;
	}
	printf("OK  load: %s\n", argv[2]);

	check("add/mul", "1+2*3", "7");
	check("string concat", "\"ab\"+\"cd\"", "abcd");
	check("REPLACE", "REPLACE(\"hello\",\"l\",\"L\")", "heLLo");
	check("while loop", "_s = 0\n_i = 0\nwhile _i < 5 {\n _s += _i\n _i++\n}\n_s", "10");

	p_unload();
	printf("OK  unload\n");

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
