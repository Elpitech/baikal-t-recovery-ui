#ifndef __COMMON_H__
#define __COMMON_H__

extern FILE *logf;
#define log(...) {fprintf (logf, "L["TAG"]: "__VA_ARGS__); fflush(logf); }
#define warn(...) {fprintf (logf, "W["TAG"]: "__VA_ARGS__); fflush(logf); }
#define err(...) {fprintf (logf, "E["TAG"]: "__VA_ARGS__); fflush(logf); }

#endif/*__COMMON_H__*/
