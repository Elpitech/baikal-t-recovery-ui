#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>
#include <form.h>

#include <time.h>
#include <errno.h>

#include "common.h"
#include "pages.h"
//#include "recovery_page.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "RECOVERY_PAGE"

#define LABEL_WIDTH 64

enum fields {
  RECOVERY_LABEL = 0,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct {
  struct window_params wp;
  WINDOW *sw;
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
} recovery_page;

void
init_recovery_page(void) {
  int width, height;
  recovery_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(recovery_page.wp.w, 0, 0);
  wbkgd(recovery_page.wp.w, PAGE_COLOR);

  recovery_page.wp.p = new_panel(recovery_page.wp.w);

  getmaxyx(recovery_page.wp.w, height, width);
  (void)height;

  recovery_page.fields[RECOVERY_LABEL] = mk_label(LABEL_WIDTH, 0, RECOVERY_LABEL, "No recovery images found", PAGE_COLOR);
  recovery_page.fields[NULL_VAL] = NULL;
  
  recovery_page.f = new_form(recovery_page.fields);
  scale_form(recovery_page.f, &height, &width);
  set_form_win(recovery_page.f, recovery_page.wp.w);
  recovery_page.sw = derwin(recovery_page.wp.w, height, width, 2, 2);
  set_form_sub(recovery_page.f, recovery_page.sw);

  post_form(recovery_page.f);

  redrawwin(recovery_page.wp.w);
	//wnoutrefresh(recovery_page.wp.w);
  //win_show(, label, 1);
}

int
recovery_page_process(int ch) {
  static uint64_t last_t = 0;
  uint64_t cur_t = time(NULL);
  struct stat st;
  int ret = 0;
  FILE *f;
  if (!recovery_page.wp.hidden) {
    if ((cur_t-last_t)>2) {
      last_t = cur_t;
      ret = stat("/dev/shm/recovery", &st);
      if(ret < 0) {
        log("Failed to stat /dev/shm/recovery: %i, errno: %s\n", ret, strerror(errno));
      } else {
        log("Recovery is found, opening\n");
        f = fopen("/dev/shm/recovery", "r");
        if (f == NULL) {
          warn("Failed to open /dev/shm/recovery");
        } else {
          memset(pages_params.recovery, 0, RECOVERY_NAME_SIZE);
          fread(pages_params.recovery, RECOVERY_NAME_SIZE, sizeof(uint8_t), f);
          fclose(f);
          log("Recovery is reported at: %s, checking\n", pages_params.recovery);
          ret = stat(pages_params.recovery, &st);
          if (ret < 0) {          
            warn("Failed to stat %s: %i, errno: %s\n", pages_params.recovery, ret, strerror(errno));
          } else {
            uint8_t buf[512];
            memset(buf, 0, 512);
            log("Recovery seems to be present\n");
            pages_params.recovery_valid = true;
            sprintf(buf, "Press enter to start recovery at %s", pages_params.recovery);
            set_field_buffer(recovery_page.fields[RECOVERY_LABEL], 0, buf);
          }
        }
      }
    }
    switch (ch) {
    case RKEY_ENTER://KEY_ENTER:
      log("Recovery: enter pressed\n");
      if (pages_params.recovery_valid) {
        pages_params.start_recovery = true;
        log("Set start recoery flag\n");
      } else {
        log("Recovery is not considered valid\n");
      }
      //pages_params.exclusive = P_RECOVERY;
      //log("Set exclusive [%i]\n", pages_params.exclusive);
      break;
      //case RKEY_ESC:
      //if (pages_params.exclusive == P_RECOVERY) {
      //pages_params.exclusive = P_NONE;
      //log("Set exclusive [%i]\n", pages_params.exclusive);
      //}
      //break;
      //default:
      //form_driver(recovery_page.f, ch);
    }

    wnoutrefresh(recovery_page.wp.w);
  }
  return 0;
}

void
deinit_recovery_page(void) {
  int i;
  unpost_form(recovery_page.f);
	free_form(recovery_page.f);
  for (i=0; i<N_FIELDS; i++)
    free_field(recovery_page.fields[i]);
  delwin(recovery_page.wp.w);
}

struct window_params *
get_recovery_page_wp(void) {
  return &recovery_page.wp;
}
