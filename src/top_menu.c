#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>
#include <form.h>

#include "common.h"
#include "top_menu.h"
#include "pages.h"

#define TAG "TOP_MENU"
#define N_ITEMS 4

#define LABEL_WIDTH 32
#define xstr(a) str(a)
#define str(a) #a

enum fields {
  HEADER_LABEL = 0,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

enum bottom_fields {
  BOTTOM_LABEL = 0,
  B_NULL_VAL,
  B_N_FIELDS=B_NULL_VAL
};

enum help_fields {
  NAV_LABEL = 0,
  NAV_TEXT1,
  NAV_TEXT2,
  NAV_TEXT3,
  H_NULL_VAL,
  H_N_FIELDS=H_NULL_VAL
};

static struct {
  WINDOW *w;
  WINDOW *bottom_w;
  WINDOW *help_w;
  
  WINDOW *sw;
  WINDOW *sw2;
  WINDOW *sw3;
  WINDOW *sw4;
  ITEM *items[N_ITEMS+1];
  MENU *m;
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
  FIELD *bottom_fields[B_N_FIELDS+1];
	FORM  *bottom_f;
  FIELD *help_fields[H_N_FIELDS+1];
	FORM  *help_f;

} top_menu;

void
init_top_menu(struct window_params *main, struct window_params *boot, struct window_params *net, struct window_params *rec) {
  int width, height;
  const char header[] = "T-Platforms mITX recovery";
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

  top_menu.items[3] = new_item("Maintenance", "MAINT");
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

  top_menu.fields[HEADER_LABEL] = mk_label(strlen(header), TOP_MENU_W/2-strlen(header)/2, 0, header, BG_COLOR);
  top_menu.fields[NULL_VAL] = NULL;
  top_menu.f = new_form(top_menu.fields);
  scale_form(top_menu.f, &height, &width);
  set_form_win(top_menu.f, top_menu.w);
  top_menu.sw2 = derwin(top_menu.w, 1, width, 0, 0);
  set_form_sub(top_menu.f, top_menu.sw2);
  post_form(top_menu.f);

  top_menu.bottom_w = newwin(1, TOP_MENU_W, LINES-1, 0);
  wbkgd(top_menu.bottom_w, BG_COLOR);
  
  top_menu.bottom_fields[BOTTOM_LABEL] = mk_label(strlen(xstr(REC_VERSION)), 0, 0, xstr(REC_VERSION), BG_COLOR);
  top_menu.bottom_fields[B_NULL_VAL] = NULL;
  top_menu.bottom_f = new_form(top_menu.bottom_fields);
  scale_form(top_menu.bottom_f, &height, &width);
  set_form_win(top_menu.bottom_f, top_menu.bottom_w);
  top_menu.sw3 = derwin(top_menu.bottom_w, 1, width, 0, 0);
  set_form_sub(top_menu.bottom_f, top_menu.sw3);
  post_form(top_menu.bottom_f);

  const int hwin_w = COLS-2*COLS/3;
  const int hwin_h = LINES-TOP_MENU_H-1;
  const char nav[] = "Navigation";
  const int nav_l = strlen(nav);
  const char nav_text1[] = "Enter     : edit values";
  const char nav_text2[] = "left/right: choose page";
  const char nav_text3[] = "up/down   : choose option";
  top_menu.help_w = newwin(hwin_h, hwin_w, TOP_MENU_H, 2*COLS/3);
  box(top_menu.help_w, 0, 0);
  mvwhline(top_menu.help_w, hwin_h/2-1, 1, 0, hwin_w-2);
  wbkgd(top_menu.help_w, PAGE_COLOR);
  //mvwhline(top_menu.w, 2, 1, ACS_HLINE, width - 2);
  top_menu.help_fields[NAV_LABEL] = mk_label(nav_l, hwin_w/2-nav_l/2-2, 0, nav, PAGE_COLOR);
  top_menu.help_fields[NAV_TEXT1] = mk_label(hwin_w-3, 0, NAV_TEXT1, nav_text1, PAGE_COLOR);
  top_menu.help_fields[NAV_TEXT2] = mk_label(hwin_w-3, 0, NAV_TEXT2, nav_text2, PAGE_COLOR);
  top_menu.help_fields[NAV_TEXT3] = mk_label(hwin_w-3, 0, NAV_TEXT3, nav_text3, PAGE_COLOR);
  
  top_menu.help_fields[H_NULL_VAL] = NULL;
  top_menu.help_f = new_form(top_menu.help_fields);
  scale_form(top_menu.help_f, &height, &width);
  set_form_win(top_menu.help_f, top_menu.help_w);
  top_menu.sw4 = derwin(top_menu.help_w, (hwin_h)/2-1, width, (hwin_h)/2, 2);
  set_form_sub(top_menu.help_f, top_menu.sw4);
  post_form(top_menu.help_f);

  post_menu(top_menu.m);
  redrawwin(top_menu.w);
  redrawwin(top_menu.bottom_w);
  redrawwin(top_menu.help_w);
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
  wnoutrefresh(top_menu.bottom_w);
  wnoutrefresh(top_menu.help_w);
  return 0;
}

void
deinit_top_menu(void) {
  int i = 0;
  unpost_form(top_menu.f);
	free_form(top_menu.f);
  for (i=0; i<N_FIELDS; i++)
    free_field(top_menu.fields[i]);
  unpost_form(top_menu.bottom_f);
	free_form(top_menu.bottom_f);
  for (i=0; i<B_N_FIELDS; i++)
    free_field(top_menu.bottom_fields[i]);

  unpost_menu(top_menu.m);
  free_menu(top_menu.m);
  for(i = 0; i < N_ITEMS; i++)
    free_item(top_menu.items[i]);
  delwin(top_menu.w);
  delwin(top_menu.bottom_w);
}
