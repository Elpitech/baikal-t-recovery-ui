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

#define TAG "TOP_MENU"
#define N_ITEMS 4

#define LABEL_WIDTH 32
#define xstr(a) str(a)
#define str(a) #a

static const char name_main[] = "Main";
static const char alt_name_main[] = "MAIN";

static const char name_boot[] = "Boot";
static const char alt_name_boot[] = "BOOT";

static const char name_network[] = "Network";
static const char alt_name_network[] = "NETWORK";

static const char name_maintenance[] = "Maintenance";
static const char alt_name_maintenance[] = "MAINTENANCE";

static struct {
  WINDOW *w;
  WINDOW *bottom_w;
  
  WINDOW *sw;
  WINDOW *sw2;
  WINDOW *sw3;
  //WINDOW *sw4;
  ITEM *items[N_ITEMS+1];
  char *alt_names[N_ITEMS];
  char *names[N_ITEMS];
  MENU *m;
  //FIELD *help_fields[H_N_FIELDS+1];
	//FORM  *help_f;

} top_menu;

void
init_top_menu(struct window_params *main, struct window_params *boot, struct window_params *net, struct window_params *rec) {
  const char header[] = "T-Platforms Service Mode";
  top_menu.w = newwin(TOP_MENU_H, TOP_MENU_W, 0, 0);
  wbkgd(top_menu.w, BG_COLOR);

  top_menu.items[0] = new_item(name_main, alt_name_main);
  set_item_userptr(top_menu.items[0], main);
  top_menu.names[0] = (char *)name_main;
  top_menu.alt_names[0] = (char *)alt_name_main;
  

  top_menu.items[1] = new_item(name_boot, alt_name_boot);
  set_item_userptr(top_menu.items[1], boot);
  top_menu.names[1] = (char *)name_boot;
  top_menu.alt_names[1] = (char *)alt_name_boot;
  
  top_menu.items[2] = new_item(name_network, alt_name_network);
  set_item_userptr(top_menu.items[2], net);
  top_menu.names[2] = (char *)name_network;
  top_menu.alt_names[2] = (char *)alt_name_network;

  top_menu.items[3] = new_item(name_maintenance, alt_name_maintenance);
  set_item_userptr(top_menu.items[3], rec);
  top_menu.names[3] = (char *)name_maintenance;
  top_menu.alt_names[3] = (char *)alt_name_maintenance;

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

  mvwaddstr(top_menu.w, 0, TOP_MENU_W/2-strlen(header)/2, header);
//  mvwaddstr(top_menu.w, 0, TOP_MENU_W-strlen(xstr(REC_VERSION)), xstr(REC_VERSION));

  top_menu.bottom_w = newwin(1, TOP_MENU_W, LINES-1, 0);
  wbkgd(top_menu.bottom_w, BG_COLOR);
  
  const char nav_text[] = "Enter: select | Left/Right: select page | Up/Down: select option | F6: exit | F10: save and exit";
  int offset = (COLS-strlen(nav_text))/2;
  mvwaddstr(top_menu.bottom_w, 0, offset, nav_text);

  post_menu(top_menu.m);
  redrawwin(top_menu.w);
  redrawwin(top_menu.bottom_w);
  pages_params.use_arrows = true;
  //win_show(, label, 1);
}

void
top_menu_select_current(void) {
}

void
top_menu_unselect_all(void) {
  int i;
  for (i=0;i<(N_ITEMS);i++) {
    //set_item_name(top_menu.items[i], top_menu.names[i]);
  }
}

void
hide_all_panels_except(struct window_params *p) {
  int i = 0;
  struct window_params *t;
  for (;i<(N_ITEMS);i++) {
    t = (struct window_params *)item_userptr(top_menu.items[i]);
    if (t == p) {
      flog("Show panel %i\n", i);
      show_panel(t->p);
      t->hidden = false;
    } else {
      flog("Hide panel %i\n", i);
      hide_panel(t->p);
      t->hidden = true;
    }
  }
}

int
top_menu_store(FILE *f) {
  ITEM *cur = current_item(top_menu.m);
  int idx = item_index(cur);
  return fprintf(f, "%i\n", idx);
}

int
top_menu_load(FILE *f) {
  ITEM *cur;
  int idx = 0;
  int ret = fscanf(f, "%i\n", &idx);
  if (ret != 1) {
    return -1;
  }
  if (idx>=N_ITEMS) {
    idx = 0;
    return -2;
  }
  cur = top_menu.items[idx];
  set_current_item(top_menu.m, cur);
  hide_all_panels_except(item_userptr(cur));
  return 0;
}

int
top_menu_process(int ch) {
  ITEM *cur;
  if (pages_params.use_arrows) {
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
  }
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
  delwin(top_menu.bottom_w);
}
