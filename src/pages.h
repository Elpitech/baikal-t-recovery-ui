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
  P_NET
};

struct pages {
  int exclusive;
};

extern struct pages pages_params;

#endif/*__PAGES_H__*/
