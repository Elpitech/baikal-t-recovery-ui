#ifndef __COMMON_H__
#define __COMMON_H__

#define PAGE_LABEL_COLOR COLOR_PAIR(1)
#define PAGE_COLOR COLOR_PAIR(2)

extern FILE *logfile;
#define log(...) {fprintf (logfile, "L["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define warn(...) {fprintf (logfile, "W["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define err(...) {fprintf (logfile, "E["TAG"]: "__VA_ARGS__); fflush(logfile); }

void label(WINDOW *win, int starty, int startx, wchar_t *string, int color);

#endif/*__COMMON_H__*/
