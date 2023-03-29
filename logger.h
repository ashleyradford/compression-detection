/**
 * @file
 *
 * Defines logger functions for DEBUG flag
 *
 * Source Matthew Malensek: https://www.cs.usfca.edu/~mmalensek/cs326/schedule/code/week04/debug.c.html
 */

#ifndef DEBUG
#define DEBUG 0
#endif

#define LOGP(str) \
    do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): %s", __FILE__, \
            __LINE__, __func__, str); } while (0)

#define LOG(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
            __LINE__, __func__, __VA_ARGS__); } while (0)
