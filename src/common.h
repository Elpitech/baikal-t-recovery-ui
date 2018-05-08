#ifndef __COMMON_H__
#define __COMMON_H__

#define PAGE_LABEL_COLOR COLOR_PAIR(1)
#define PAGE_COLOR COLOR_PAIR(2)
#define BG_COLOR COLOR_PAIR(3)
#define FIELD_COLOR COLOR_PAIR(4)
#define GREEN_COLOR COLOR_PAIR(5)
#define RED_COLOR COLOR_PAIR(6)
#define RED_EDIT_COLOR COLOR_PAIR(7)
#define EDIT_COLOR COLOR_PAIR(8)

#define RKEY_ENTER 0x0a
#define RKEY_ESC   0x1b
#define RKEY_F10   0x112
#define RKEY_F6    0x10e

extern FILE *logfile;
#define flog(...) {fprintf (logfile, "L["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define fwarn(...) {fprintf (logfile, "W["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define ferr(...) {fprintf (logfile, "E["TAG"]: "__VA_ARGS__); fflush(logfile); }
#define fmsg(...) {fprintf (logfile, __VA_ARGS__); fflush(logfile); }
#ifdef DEBUG
#define fdbg fmsg
#else
#define fdbg(...)
#endif

#define RECSTAT_PATH             "/var/run/recoveryui/recovery.stat"

#define VAR_PREFIX               "/var/run/recoveryui/"
#define INT_RECOVERY_PATH        "/var/run/recoveryui/int/recovery.rc"
#define INT_RECOVERY_TAR_PATH    "/var/run/recoveryui/int/recovery-tar-path"
#define INT_RECOVERY_MDEV        "/var/run/recoveryui/int/recovery-mdev"
#define INT_RECOVERY_LINE        "/var/run/recoveryui/int/line"
#define EXT_RECOVERY_PATH        "/var/run/recoveryui/ext/recovery.rc"
#define EXT_RECOVERY_TAR_PATH    "/var/run/recoveryui/ext/recovery-tar-path"
#define EXT_RECOVERY_MDEV        "/var/run/recoveryui/ext/recovery-mdev"
#define EXT_RECOVERY_LINE        "/var/run/recoveryui/ext/line"
#define USB_UPDATE_ROM_PATH      "/var/run/recoveryui/update-rom-path"
#define WEB_UPDATE_ROM_PATH      "/var/run/recoveryui/web-update-rom-path"
#define ROM_UPDATE_SCRIPT_PATH   "/usr/bin/btflash.sh"
#define ROM_DOWNLOAD_SCRIPT_PATH "/usr/bin/getrom.sh"
#define STATE_PATH               "/var/run/recoveryui/recovery.state"
#define NETCONF_SCRIPT_PATH      "/usr/bin/netconf.sh"

#endif/*__COMMON_H__*/
