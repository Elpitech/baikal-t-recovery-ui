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
#include "main_page.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "RECOVERY_PAGE"
#define EXT_REC_TXT_NOTFOUND "External recovery image not found"
#define EXT_REC_TXT_FOUND "Press enter to start recovery from USB"
#define INT_REC_TXT_NOTFOUND "Internal recovery image not found"
#define INT_REC_TXT_FOUND "Press enter to start recovery from disk"

#define LABEL_WIDTH 40

enum fields {
  EXT_RECOVERY_LABEL = 0,
  INT_RECOVERY_LABEL,
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
check_recovery(char *tar_path, char *recovery_path, char *recovery_mdev, char *recovery_line, enum fields label, bool int_recovery) {
  char *rtp = pages_params.ext_recovery_tar_path;
  char *rmdev = pages_params.ext_recovery_mdev
  struct stat st;
  int ret = 0;
  FILE *f;
  ret = stat(tar_path, &st);
  pages_params.recovery_valid = false;
  if (ret < 0) {
    log("Failed to stat %s: %i, errno: %s\n", tar_path, ret, strerror(errno));
    return;
  }
  ret = stat(recovery_path, &st);
  if (ret < 0) {
    log("Failed to stat %s: %i, errno: %s\n", recovery_path, ret, strerror(errno));
    return;
  }
  ret = stat(recovery_mdev, &st);
  if (ret < 0) {
    log("Failed to stat %s: %i, errno: %s\n", recovery_mdev, ret, strerror(errno));
    return;
  }

  ret = stat(recovery_line, &st);
  if (ret < 0) {
    log("Failed to stat %s: %i, errno: %s\n", recovery_line, ret, strerror(errno));
    return;
  }

  log("Recovery is found\n");
  f = fopen(tar_path, "r");
  if (f == NULL) {
    warn("Failed to open %s", tar_path);
    return;
  }
  if (int_recovery) {
    rtp = pages_params.int_recovery_tar_path;
    rmdev = pages_params.int_recovery_mdev;
  }
  memset(rtp, 0, RECOVERY_NAME_SIZE);
  fread(rtp, RECOVERY_NAME_SIZE, sizeof(uint8_t), f);
  fclose(f);
  f = fopen(recovery_mdev, "r");
  if (f == NULL) {
    warn("Failed to open %s", recovery_mdev);
    return;
  }
  memset(rmdev, 0, RECOVERY_NAME_SIZE);
  fread(rmdev, RECOVERY_NAME_SIZE, sizeof(uint8_t), f);
  fclose(f);

  log("Recovery tar is reported at: %s, checking\n", pages_params.recovery_tar_path);
  ret = stat(pages_params.recovery_tar_path, &st);
  if (ret < 0) {
    warn("Failed to stat %s: %i, errno: %s\n", pages_params.recovery_tar_path, ret, strerror(errno));
    if (int_recovery) {
      set_field_buffer(recovery_page.fields[label], 0, INT_REC_TXT_NOTFOUND);
    } else {
      set_field_buffer(recovery_page.fields[label], 0, EXT_REC_TXT_NOTFOUND);
    }
    return;
  }
  log("Recovery seems to be present\n");
  pages_params.recovery_valid = true;
  f = fopen(recovery_line, "r");
  if (f == NULL) {
    warn("Failed to open %s", recovery_line);
    return;
  }
  char buf[256];
  memset(buf, 0, 256);
  fread(buf, 256, sizeof(uint8_t), f);
  fclose(f);

  set_field_buffer(recovery_page.fields[label], 0, buf);
}

void
check_ext_recovery(void) {
  check_recovery(EXT_RECOVERY_TAR_PATH, EXT_RECOVERY_PATH, EXT_RECOVERY_MDEV, EXT_RECOVERY_LINE, EXT_RECOVERY_LABEL, false);
}

void
check_int_recovery(void) {
  check_recovery(INT_RECOVERY_TAR_PATH, INT_RECOVERY_PATH, INT_RECOVERY_MDEV, INT_RECOVERY_LINE, INT_RECOVERY_LABEL, true);
}

void
init_recovery_page(void) {
  int width, height;
  recovery_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(recovery_page.wp.w, 0, 0);
  wbkgd(recovery_page.wp.w, PAGE_COLOR);

  recovery_page.wp.p = new_panel(recovery_page.wp.w);

  getmaxyx(recovery_page.wp.w, height, width);
  (void)height;

  recovery_page.fields[EXT_RECOVERY_LABEL] = mk_label(LABEL_WIDTH, 0, EXT_RECOVERY_LABEL, EXT_REC_TXT_NOTFOUND, PAGE_COLOR);
  recovery_page.fields[INT_RECOVERY_LABEL] = mk_label(LABEL_WIDTH, 0, INT_RECOVERY_LABEL, INT_REC_TXT_NOTFOUND, PAGE_COLOR);
  recovery_page.fields[NULL_VAL] = NULL;
  
  recovery_page.f = new_form(recovery_page.fields);
  scale_form(recovery_page.f, &height, &width);
  set_form_win(recovery_page.f, recovery_page.wp.w);
  recovery_page.sw = derwin(recovery_page.wp.w, height, width, 2, 2);
  set_form_sub(recovery_page.f, recovery_page.sw);

  post_form(recovery_page.f);

  redrawwin(recovery_page.wp.w);

  check_int_recovery();
	//wnoutrefresh(recovery_page.wp.w);
  //win_show(, label, 1);
}

int
recovery_page_process(int ch) {
  static uint64_t last_t = 0;
  uint64_t cur_t = time(NULL);
  if (!recovery_page.wp.hidden) {
    if ((cur_t-last_t)>2) {
      last_t = cur_t;
      check_ext_recovery();
    }
    switch (ch) {
    case RKEY_ENTER://KEY_ENTER:
    {
      log("Recovery: enter pressed\n");
      FIELD *f = current_field(recovery_page.f);
      if (f == recovery_page.fields[EXT_RECOVERY_LABEL]) {
        if (pages_params.ext_recovery_valid) {
          pages_params.start_ext_recovery = true;
          pages_params.start_int_recovery = false;
          log("Set start external recovery flag\n");
        } else {
          log("Recovery is not considered valid\n");
        }
      } else if (f == recovery_page.fields[INT_RECOVERY_LABEL]) {
        if (pages_params.int_recovery_valid) {
          pages_params.start_int_recovery = true;
          pages_params.start_ext_recovery = false;
          log("Set start internal recovery flag\n");
        } else {
          log("Recovery is not considered valid\n");
        }
      }
    }
    break;
    case KEY_DOWN:
      form_driver(recovery_page.f, REQ_NEXT_FIELD);
      break;
    case KEY_UP:
      form_driver(recovery_page.f, REQ_PREV_FIELD);
      break;
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
