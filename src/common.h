#ifndef __COMMON_H__
#define __COMMON_H__

#define PAGE_LABEL_COLOR COLOR_PAIR(1)
#define PAGE_COLOR COLOR_PAIR(2)
#define BG_COLOR COLOR_PAIR(3)
#define FIELD_COLOR COLOR_PAIR(4)
#define GREEN_COLOR COLOR_PAIR(5)
#define RED_COLOR COLOR_PAIR(6)

#define RKEY_ENTER 0x0a
#define RKEY_ESC   0x1b
#define RKEY_F10   0x112
#define RKEY_F6    0x10e

extern FILE *logfile;
#define log(...) {fprintf (logfile, "L["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define warn(...) {fprintf (logfile, "W["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define err(...) {fprintf (logfile, "E["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define msg(...) {fprintf (logfile, __VA_ARGS__); fflush(logfile); }


struct spinner_arg {
  int current_str;
  int n_str;
  char **strs;
};

#define INT_RECOVERY_PATH "/recovery/recovery.rc"
#define EXT_RECOVERY_PATH "/tmp/recovery.rc"
#define EXT_RECOVERY_TAR_PATH "/tmp/recovery-tar-path"
#define EXT_RECOVERY_MDEV "/tmp/recovery-mdev"

#endif/*__COMMON_H__*/
