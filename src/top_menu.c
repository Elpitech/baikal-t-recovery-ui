#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>

#include "common.h"
#include "top_menu.h"
#include "pages.h"

#define TAG "TOP_MENU"
#define N_ITEMS 4

static struct {
  WINDOW *w;
  WINDOW *sw;
  ITEM *items[N_ITEMS+1];
  MENU *m;
} top_menu;

void
init_top_menu(struct window_params *main, struct window_params *boot, struct window_params *net, struct window_params *rec) {
  int width, height;
  top_menu.w = newwin(TOP_MENU_H, TOP_MENU_W, 0, 0);
  wbkgd(top_menu.w, BG_COLOR);
  //box(top_menu.w, 0, 0);

  //getmaxyx(top_menu.w, height, width);
  //(void)height;

	/* mvwaddch(top_menu.w, 2, 0, ACS_LTEE);  */
	/* mvwhline(top_menu.w, 2, 1, ACS_HLINE, width - 2);  */
	/* mvwaddch(top_menu.w, 2, width - 1, ACS_RTEE);  */

  top_menu.items[0] = new_item("Main", "MAIN");
  set_item_userptr(top_menu.items[0], main);
  
  top_menu.items[1] = new_item("Boot", "BOOT");
  set_item_userptr(top_menu.items[1], boot);
  
  top_menu.items[2] = new_item("Network", "NET");
  set_item_userptr(top_menu.items[2], net);

  top_menu.items[3] = new_item("Recovery", "REC");
  set_item_userptr(top_menu.items[3], rec);

  top_menu.items[4] = NULL;
  top_menu.m = new_menu((ITEM **)top_menu.items);
  menu_opts_off(top_menu.m, O_SHOWDESC);

  set_menu_win(top_menu.m, top_menu.w);
  top_menu.sw = derwin(top_menu.w, 1, TOP_MENU_W-2, 1, 2);
  wbkgd(top_menu.sw, BG_COLOR);
  set_menu_sub(top_menu.m, top_menu.sw);
	set_menu_format(top_menu.m, 1, N_ITEMS);
	set_menu_mark(top_menu.m, "");
  set_menu_fore(top_menu.m, A_REVERSE);
  set_menu_back(top_menu.m, BG_COLOR);

  post_menu(top_menu.m);
  redrawwin(top_menu.w);
  //win_show(, label, 1);
}

void
hide_all_panels_except(struct window_params *p) {
  int i = 0;
  struct window_params *t;
  for (;i<(N_ITEMS);i++) {
    t = (struct window_params *)item_userptr(top_menu.items[i]);
    if (t == p) {
      log("Show panel %i\n", i);
      show_panel(t->p);
      t->hidden = false;
    } else {
      log("Hide panel %i\n", i);
      hide_panel(t->p);
      t->hidden = true;
    }
  }
}

int
top_menu_process(int ch) {
  ITEM *cur;
  switch (ch) {
  case KEY_LEFT:
    menu_driver(top_menu.m, REQ_LEFT_ITEM);
    cur = current_item(top_menu.m);
    hide_all_panels_except(item_userptr(cur));
    touchwin(top_menu.w);
    wnoutrefresh(top_menu.w);
    break;
  case KEY_RIGHT:
    menu_driver(top_menu.m, REQ_RIGHT_ITEM);
    cur = current_item(top_menu.m);
    hide_all_panels_except(item_userptr(cur));
    touchwin(top_menu.w);
    wnoutrefresh(top_menu.w);
    break;
  }
  wnoutrefresh(top_menu.w);
  return 0;
}

void
deinit_top_menu(void) {
  int i = 0;
  unpost_menu(top_menu.m);
  free_menu(top_menu.m);
  for(i = 0; i < N_ITEMS; i++)
    free_item(top_menu.items[i]);
  delwin(top_menu.w);
}
