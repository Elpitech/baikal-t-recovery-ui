#include <stdlib.h>
#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>
#include <form.h>

#include <time.h>

#include "main_page.h"

#include "common.h"
#include "pages.h"
//#include "boot_page.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "BOOT_PAGE"

#define LABEL_WIDTH 25

enum fields {
  BOOT_DEVICE_VAL = 0,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static const char sata0port0[] = "sata0:0";
static const char sata0port1[] = "sata0:1";

static const char *sata_devs[] = {
  sata0port0,
  sata0port1
};

static struct {
  struct window_params wp;
  WINDOW *sw;
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
} boot_page;

void
init_boot_page(void) {
  int width, height;
  boot_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(boot_page.wp.w, 0, 0);
  wbkgd(boot_page.wp.w, PAGE_COLOR);

  boot_page.wp.p = new_panel(boot_page.wp.w);

  getmaxyx(boot_page.wp.w, height, width);
  (void)height;

  log("sata boot device: %s\n", fru.bootdevice);

  mvwaddstr(boot_page.wp.w, 2, 2, "SATA boot priority");
  mvwaddstr(boot_page.wp.w, 4, 2, "Boot partition");
  if (strncmp((char *)fru.bootdevice, sata0port1, strlen(sata0port1))==0) {
   	boot_page.fields[BOOT_DEVICE_VAL] = mk_spinner(strlen(sata0port1), 0, 2, (char**)sata_devs, 2, 1, BG_COLOR);
  } else {
    boot_page.fields[BOOT_DEVICE_VAL] = mk_spinner(strlen(sata0port0), 0, 2, (char**)sata_devs, 2, 0, BG_COLOR);
  }
  boot_page.fields[NULL_VAL] = NULL;
  
  boot_page.f = new_form(boot_page.fields);
  scale_form(boot_page.f, &height, &width);
  set_form_win(boot_page.f, boot_page.wp.w);
  boot_page.sw = derwin(boot_page.wp.w, height, LABEL_WIDTH, 2, LABEL_WIDTH);
  set_form_sub(boot_page.f, boot_page.sw);

  post_form(boot_page.f);

  redrawwin(boot_page.wp.w);
}

void
boot_save_bootdev(void) {
  log("Save boot device\n");
  char *ptr = field_buffer(boot_page.fields[BOOT_DEVICE_VAL], 0);
  int i = 0;
  msg("PTR:\n");
  for (;i<16;i++) {
    msg("%02x ", ptr[i]);
    if (ptr[i] == ' ') {
      ptr[i] = '\0';
      break;
    }
  }
  msg("\n");
  log("Obtained buffer: [%s]\n", ptr);
  //int len = strlen(ptr);
  //memcpy(fru.bootdevice, ptr, (len>FRU_STR_MAX?FRU_STR_MAX:len));
  fru_mrec_update_bootdevice(&fru, (uint8_t *)ptr);
}

int
boot_page_process(int ch) {
  if (!boot_page.wp.hidden) {
    curs_set(1);
    switch (ch) {
    case KEY_DOWN:
      //if (pages_params.exclusive == P_BOOT) {
      form_driver(boot_page.f, REQ_NEXT_FIELD);
      //form_driver(net_page.f, REQ_END_LINE);
      //}
      break;
    case KEY_UP:
      //if (pages_params.exclusive == P_BOOT) {
      form_driver(boot_page.f, REQ_PREV_FIELD);
      //form_driver(net_page.f, REQ_END_LINE);
      //}
      break;
		case KEY_BACKSPACE:
		case 127:
      //if (pages_params.exclusive == P_BOOT) {
      form_driver(boot_page.f, REQ_DEL_PREV);
      //}
      break;
    case KEY_DC:
      //if (pages_params.exclusive == P_BOOT) {
      form_driver(boot_page.f, REQ_DEL_CHAR);
      //}
			break;
    case RKEY_ENTER://KEY_ENTER:
    {
      FIELD *f = current_field(boot_page.f);
      if (f == boot_page.fields[BOOT_DEVICE_VAL]) {
        spinner_spin(f);
        boot_save_bootdev();
      }
    }
    break;
    /* case RKEY_ESC: */
    /*   if (pages_params.exclusive == P_BOOT) { */
    /*     form_driver(boot_page.f, REQ_VALIDATION); */
    /*     boot_save_bootdev(); */
    /*     pages_params.exclusive = P_NONE; */
    /*     log("Set exclusive [%i]\n", pages_params.exclusive); */
    /*   } */
    /*   break; */
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
  for (i=0; i<N_FIELDS; i++) {
    void *p = field_userptr(boot_page.fields[i]);
    if (p != NULL) {
      free(p);
    }
    free_field(boot_page.fields[i]);
  }
  delwin(boot_page.wp.w);
}

struct window_params *
get_boot_page_wp(void) {
  return &boot_page.wp;
}
