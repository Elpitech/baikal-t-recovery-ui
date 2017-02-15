#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>
#include <form.h>

#include <time.h>

#include "common.h"
#include "pages.h"
//#include "net_page.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "NET_PAGE"

#define LABEL_WIDTH 32

enum fields {
  MAC_LABEL = 0,
  MAC0_VAL,
  MAC1_VAL,
  MAC2_VAL,
  MAC3_VAL,
  MAC4_VAL,
  MAC5_VAL,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct {
  struct window_params wp;
  WINDOW *w;
  WINDOW *sw;
  uint32_t shred;
  uint8_t mac_val[LABEL_WIDTH];
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
} net_page;

void
init_net_page(void) {
  int width, height;
  int i = 0;
  net_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(net_page.wp.w, 0, 0);
  wbkgd(net_page.wp.w, PAGE_COLOR);

  net_page.wp.p = new_panel(net_page.wp.w);

  getmaxyx(net_page.wp.w, height, width);
  (void)height;

  net_page.fields[MAC_LABEL] = mk_label(LABEL_WIDTH, 0, MAC_LABEL, "MAC address", PAGE_COLOR);
  memset(net_page.mac_val, 0, LABEL_WIDTH);
  for (;i<6; i++) {
    sprintf(net_page.mac_val+3*i, "%02x", fru.mac[i]);
    net_page.fields[MAC0_VAL+i] = mk_editable_field_regex(2, LABEL_WIDTH+3*i, MAC_LABEL, net_page.mac_val+3*i, "[0-9a-zA-Z][0-9a-zA-Z]", PAGE_COLOR);
  }

  net_page.fields[NULL_VAL] = NULL;
  
  net_page.f = new_form(net_page.fields);
  scale_form(net_page.f, &height, &width);
  set_form_win(net_page.f, net_page.wp.w);
  net_page.sw = derwin(net_page.wp.w, height, width, 2, 2);
  set_form_sub(net_page.f, net_page.sw);

  post_form(net_page.f);

  redrawwin(net_page.wp.w);
	//wnoutrefresh(net_page.wp.w);
  //win_show(, label, 1);
}

int
net_page_process(int ch) {
  if (!net_page.wp.hidden) {
    switch (ch) {
    case KEY_DOWN:
      //if (page_params.exclusive == P_NET) {
        form_driver(net_page.f, REQ_NEXT_FIELD);
        form_driver(net_page.f, REQ_END_LINE);
        //}
      break;
    case KEY_UP:
      //if (page_params.exclusive == P_NET) {
        form_driver(net_page.f, REQ_PREV_FIELD);
        form_driver(net_page.f, REQ_END_LINE);
        //}
      break;
    default:
      form_driver(net_page.f, ch);
      break;
    }
    wnoutrefresh(net_page.wp.w);
  }
  return 0;
}

void
deinit_net_page(void) {
  int i;
  unpost_form(net_page.f);
	free_form(net_page.f);
  for (i=0; i<N_FIELDS; i++)
    free_field(net_page.fields[i]);
  delwin(net_page.wp.w);
}

struct window_params *
get_net_page_wp(void) {
  return &net_page.wp;
}
