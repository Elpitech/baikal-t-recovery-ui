#ifndef __PAGES_H__
#define __PAGES_H__

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

#define RECOVERY_NAME_SIZE 256
struct pages {
  int exclusive;
  uint8_t recovery[RECOVERY_NAME_SIZE];
  bool recovery_valid;
  bool start_recovery;
};

extern struct pages pages_params;

#endif/*__PAGES_H__*/
