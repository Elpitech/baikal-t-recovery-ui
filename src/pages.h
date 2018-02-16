#ifndef __PAGES_H__
#define __PAGES_H__

#include <sys/socket.h>
#include <netdb.h>

struct window_params {
  PANEL *p;
  WINDOW *w;
  bool hidden;
};

enum PAGES {
  P_NONE = 0,
  P_MAIN,
  P_BOOT,
  P_NET,
  P_RECOVERY
};

enum START {
  START_NONE=0,
  START_EXT,
  START_INT,
  START_ROM_UP,
  START_ROM_DOWN,
  START_IFUP,
  START_DHCP,
};

#define RECOVERY_NAME_SIZE 256
#define ROM_URL_SIZE 1024
struct pages {
  int exclusive;
  char int_recovery_tar_path[RECOVERY_NAME_SIZE];
  char int_recovery_mdev[RECOVERY_NAME_SIZE];
  bool int_recovery_valid;
  char ext_recovery_tar_path[RECOVERY_NAME_SIZE];
  char ext_recovery_mdev[RECOVERY_NAME_SIZE];
  char rom_url[ROM_URL_SIZE];
  char rom_path[ROM_URL_SIZE];
  char ip[NI_MAXHOST];
  char nm[NI_MAXHOST];
  char gw[NI_MAXHOST];
  char dns1[NI_MAXHOST];
  bool ext_recovery_valid;
  bool usb_rom_valid;
  bool web_rom_valid;
  enum START start;
  int bmc_version[3];
  uint8_t boot_reason[2];
  bool use_arrows;
};

extern struct pages pages_params;

#endif/*__PAGES_H__*/
