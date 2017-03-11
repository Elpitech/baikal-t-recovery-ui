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

#define LABEL_WIDTH 25

enum mac_fields {
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
  net_page.wp.w = newwin(LINES-TOP_MENU_H-1,2*COLS/3,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(net_page.wp.w, 0, 0);
  wbkgd(net_page.wp.w, PAGE_COLOR);

  net_page.wp.p = new_panel(net_page.wp.w);

  getmaxyx(net_page.wp.w, height, width);
  (void)height;

  //net_page.fields[MAC_LABEL] = mk_label(LABEL_WIDTH, 0, MAC_LABEL, "MAC address", PAGE_COLOR);
  mvwaddstr(net_page.wp.w, 2, 2, "MAC address");
  memset(net_page.mac_val, 0, LABEL_WIDTH);
  for (;i<6; i++) {
    sprintf(net_page.mac_val+3*i, "%02x", fru.mac[i]);
    net_page.fields[MAC0_VAL+i] = mk_editable_field_regex(2, 3*i, 0, net_page.mac_val+3*i, "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR);
  }

  net_page.fields[NULL_VAL] = NULL;
  
  net_page.f = new_form(net_page.fields);
  scale_form(net_page.f, &height, &width);
  set_form_win(net_page.f, net_page.wp.w);
  net_page.sw = derwin(net_page.wp.w, height, LABEL_WIDTH, 2, LABEL_WIDTH);
  set_form_sub(net_page.f, net_page.sw);

  post_form(net_page.f);

  redrawwin(net_page.wp.w);
	//wnoutrefresh(net_page.wp.w);
  //win_show(, label, 1);
}

void
net_save_mac(void) {
  uint8_t mac[6] = {0};
  int temp = 0;
  sscanf(field_buffer(net_page.fields[MAC0_VAL], 0), "%02x", &temp);
  mac[0] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[MAC1_VAL], 0), "%02x", &temp);
  mac[1] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[MAC2_VAL], 0), "%02x", &temp);
  mac[2] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[MAC3_VAL], 0), "%02x", &temp);
  mac[3] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[MAC4_VAL], 0), "%02x", &temp);
  mac[4] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[MAC5_VAL], 0), "%02x", &temp);
  mac[5] = temp&0xff;
  temp = 0;

  log("Save MAC: [%02x:%02x:%02x:%02x:%02x:%02x]\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  //memcpy(fru.mac, mac, 6);
  fru_mrec_update_mac(&fru, mac);
}

int
net_page_process(int ch) {
  if (!net_page.wp.hidden) {
    curs_set(1);
    switch (ch) {
    case KEY_DOWN:
      //if (pages_params.exclusive == P_NET) {
      form_driver(net_page.f, REQ_NEXT_FIELD);
      //form_driver(net_page.f, REQ_END_LINE);
      //}
      break;
    case KEY_UP:
      //if (pages_params.exclusive == P_NET) {
      form_driver(net_page.f, REQ_PREV_FIELD);
      //form_driver(net_page.f, REQ_END_LINE);
      //}
      break;
		case KEY_BACKSPACE:
		case 127:
      //if (pages_params.exclusive == P_NET) {
      form_driver(net_page.f, REQ_DEL_PREV);
      //}
      break;
    case KEY_DC:
      //if (pages_params.exclusive == P_NET) {
      form_driver(net_page.f, REQ_DEL_CHAR);
      //}
			break;
    case RKEY_ENTER://KEY_ENTER:
      //pages_params.exclusive = P_NET;
      //log("Set exclusive [%i]\n", pages_params.exclusive);
      net_save_mac();
      break;
    case RKEY_ESC:
      //if (pages_params.exclusive == P_NET) {
      //net_save_mac();
      //pages_params.exclusive = P_NONE;
      //log("Set exclusive [%i]\n", pages_params.exclusive);
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
