#ifndef __COMMON_H__
#define __COMMON_H__

extern FILE *logf;
#define log(...) fprintf (logf, "L["TAG"]: "__VA_ARGS__)
#define warn(...) fprintf (logf, "W["TAG"]: "__VA_ARGS__)
#define err(...) fprintf (logf, "E["TAG"]: "__VA_ARGS__)

#endif/*__COMMON_H__*/
