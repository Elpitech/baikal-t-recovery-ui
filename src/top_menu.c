#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>

#include "top_menu.h"

#define N_ITEMS 2

static struct {
  WINDOW *w;
  ITEM *items[N_ITEMS+1];
  MENU *m;
} top_menu;

void
init_top_menu(void) {
  int width, height;
  top_menu.w = newwin(TOP_MENU_H, TOP_MENU_W, 0, 0);
  //box(top_menu.w, 0, 0);

  //getmaxyx(top_menu.w, height, width);
  //(void)height;

	/* mvwaddch(top_menu.w, 2, 0, ACS_LTEE);  */
	/* mvwhline(top_menu.w, 2, 1, ACS_HLINE, width - 2);  */
	/* mvwaddch(top_menu.w, 2, width - 1, ACS_RTEE);  */

  top_menu.items[0] = new_item("MAIN", "MAIN");
  top_menu.items[1] = new_item("BOOT", "BOOT");
  top_menu.items[2] = NULL;
  top_menu.m = new_menu((ITEM **)top_menu.items);
  menu_opts_off(top_menu.m, O_SHOWDESC);

  set_menu_win(top_menu.m, top_menu.w);
  set_menu_sub(top_menu.m, derwin(top_menu.w, 1, TOP_MENU_W-2, 1, 2));
	set_menu_format(top_menu.m, 1, N_ITEMS);
	set_menu_mark(top_menu.m, "");

  post_menu(top_menu.m);
  redrawwin(top_menu.w);
  //win_show(, label, 1);
}

int
top_menu_process(int ch) {
  switch (ch) {
  case KEY_LEFT:
    menu_driver(top_menu.m, REQ_LEFT_ITEM);
    touchwin(top_menu.w);
    wnoutrefresh(top_menu.w);
    break;
  case KEY_RIGHT:
    menu_driver(top_menu.m, REQ_RIGHT_ITEM);
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
