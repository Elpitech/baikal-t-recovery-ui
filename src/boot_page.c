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
//#include "boot_page.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "BOOT_PAGE"

#define LABEL_WIDTH 32

enum fields {
  BOOT_DEVICE_LABEL = 0,
  BOOT_DEVICE_VAL,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct {
  struct window_params wp;
  WINDOW *sw;
  uint32_t shred;
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
} boot_page;

void
init_boot_page(void) {
  int width, height;
  time_t t;
  struct tm tm;
  boot_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(boot_page.wp.w, 0, 0);
  wbkgd(boot_page.wp.w, PAGE_COLOR);

  boot_page.wp.p = new_panel(boot_page.wp.w);

  getmaxyx(boot_page.wp.w, height, width);
  (void)height;

  boot_page.shred = read_shred();
  log("shred: 0x%08x\n", boot_page.shred);
  t = time(NULL);
  tm = *localtime(&t);

  boot_page.fields[BOOT_DEVICE_LABEL] = mk_label(LABEL_WIDTH, 0, BOOT_DEVICE_LABEL, "Bootdevice", PAGE_COLOR);
	boot_page.fields[BOOT_DEVICE_VAL] = mk_editable_field_regex(16, LABEL_WIDTH, BOOT_DEVICE_LABEL, fru.bootdevice, ".*", BG_COLOR);
  boot_page.fields[NULL_VAL] = NULL;
  
  boot_page.f = new_form(boot_page.fields);
  scale_form(boot_page.f, &height, &width);
  set_form_win(boot_page.f, boot_page.wp.w);
  boot_page.sw = derwin(boot_page.wp.w, height, width, 2, 2);
  set_form_sub(boot_page.f, boot_page.sw);

  post_form(boot_page.f);

  redrawwin(boot_page.wp.w);
	//wnoutrefresh(boot_page.wp.w);
  //win_show(, label, 1);
}

void
boot_save_bootdev(void) {
  char *ptr = field_buffer(boot_page.fields[BOOT_DEVICE_VAL], 0);
  //int len = strlen(ptr);
  //memcpy(fru.bootdevice, ptr, (len>FRU_STR_MAX?FRU_STR_MAX:len));
  fru_mrec_update_bootdevice(&fru, ptr);
}

int
boot_page_process(int ch) {
  if (!boot_page.wp.hidden) {
    switch (ch) {
    case KEY_RIGHT:
      if (pages_params.exclusive == P_BOOT) {
        form_driver(boot_page.f, REQ_NEXT_FIELD);
        //form_driver(net_page.f, REQ_END_LINE);
      }
      break;
    case KEY_LEFT:
      if (pages_params.exclusive == P_BOOT) {
        form_driver(boot_page.f, REQ_PREV_FIELD);
        //form_driver(net_page.f, REQ_END_LINE);
      }
      break;
		case KEY_BACKSPACE:
		case 127:
      if (pages_params.exclusive == P_BOOT) {
        form_driver(boot_page.f, REQ_DEL_PREV);
      }
      break;
    case KEY_DC:
      if (pages_params.exclusive == P_BOOT) {
        form_driver(boot_page.f, REQ_DEL_CHAR);
      }
			break;
    case RKEY_ENTER://KEY_ENTER:
      pages_params.exclusive = P_BOOT;
      log("Set exclusive [%i]\n", pages_params.exclusive);
      break;
    case RKEY_ESC:
      if (pages_params.exclusive == P_BOOT) {
        boot_save_bootdev();
        pages_params.exclusive = P_NONE;
        log("Set exclusive [%i]\n", pages_params.exclusive);
      }
      break;
    default:
      form_driver(boot_page.f, ch);
      break;
    }

    wnoutrefresh(boot_page.wp.w);
  }
  return 0;
}

void
deinit_boot_page(void) {
  int i;
  unpost_form(boot_page.f);
	free_form(boot_page.f);
  for (i=0; i<N_FIELDS; i++)
    free_field(boot_page.fields[i]);
  delwin(boot_page.wp.w);
}

struct window_params *
get_boot_page_wp(void) {
  return &boot_page.wp;
}
