#ifndef __COMMON_H__
#define __COMMON_H__

extern FILE *logfile;
#define log(...) {fprintf (logfile, "L["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define warn(...) {fprintf (logfile, "W["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define err(...) {fprintf (logfile, "E["TAG"]: "__VA_ARGS__); fflush(logfile); }

#endif/*__COMMON_H__*/
