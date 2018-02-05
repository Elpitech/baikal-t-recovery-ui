#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
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
#include <errno.h>

#include "common.h"
#include "pages.h"
#include "main_page.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "RECOVERY_PAGE"
//                           |                         |
#define EXT_REC_TXT_NOTFOUND "        Not found"
#define EXT_REC_TXT_FOUND    "          Start"
#define INT_REC_TXT_NOTFOUND "        Not found"
#define INT_REC_TXT_FOUND    "          Start"
#define ROM_NOTFOUND         "        Not found"
#define ROM_FOUND_USB        "     Start from USB"
#define ROM_FOUND_WEB        "     Start from WEB"
#define ROM_DOWNLOAD         "          Start"

#define LABEL_WIDTH 25
#define KB 1024
#define MB (1024*KB)

enum fields {
  EXT_RECOVERY_LABEL = 0,
  INT_RECOVERY_LABEL,
  ROM_UPDATE_LABEL,
  ROM_URL_LABEL,
  ROM_DOWNLOAD_LABEL,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct {
  struct window_params wp;
  WINDOW *sw;
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
  char url[ROM_URL_SIZE];
  bool edit_mode;
} recovery_page;

void
check_recovery(char *tar_path, char *recovery_path, char *recovery_mdev, char *recovery_line, enum fields label, bool int_recovery) {
  char *rtp = pages_params.ext_recovery_tar_path;
  char *rmdev = pages_params.ext_recovery_mdev;
  struct stat st;
  int ret = 0;
  FILE *f;
  if (int_recovery) {
    rtp = pages_params.int_recovery_tar_path;
    rmdev = pages_params.int_recovery_mdev;
  }

  ret = stat(tar_path, &st);
  if (int_recovery) {
    pages_params.int_recovery_valid = false;
  } else {
    pages_params.ext_recovery_valid = false;
  }
  if (ret < 0) {
    fdbg("Failed to stat %s: %i, errno: %s\n", tar_path, ret, strerror(errno));
    return;
  }
  ret = stat(recovery_path, &st);
  if (ret < 0) {
    fdbg("Failed to stat %s: %i, errno: %s\n", recovery_path, ret, strerror(errno));
    return;
  }
  ret = stat(recovery_mdev, &st);
  if (ret < 0) {
    fdbg("Failed to stat %s: %i, errno: %s\n", recovery_mdev, ret, strerror(errno));
    return;
  }

  ret = stat(recovery_line, &st);
  if (ret < 0) {
    fdbg("Failed to stat %s: %i, errno: %s\n", recovery_line, ret, strerror(errno));
    return;
  }

  fdbg("Recovery is found\n");
  f = fopen(tar_path, "r");
  if (f == NULL) {
    fdbg("Failed to open %s", tar_path);
    return;
  }
  memset(rtp, 0, RECOVERY_NAME_SIZE);
  fread(rtp, RECOVERY_NAME_SIZE, sizeof(uint8_t), f);
  fclose(f);
  f = fopen(recovery_mdev, "r");
  if (f == NULL) {
    fdbg("Failed to open %s", recovery_mdev);
    return;
  }
  memset(rmdev, 0, RECOVERY_NAME_SIZE);
  fread(rmdev, RECOVERY_NAME_SIZE, sizeof(uint8_t), f);
  fclose(f);

  fdbg("Recovery tar is reported at: %s, checking\n", rtp);
  ret = stat(rtp, &st);
  if (ret < 0) {
    fdbg("Failed to stat %s: %i, errno: %s\n", rtp, ret, strerror(errno));
    if (int_recovery) {
      set_field_buffer(recovery_page.fields[label], 0, INT_REC_TXT_NOTFOUND);
    } else {
      set_field_buffer(recovery_page.fields[label], 0, EXT_REC_TXT_NOTFOUND);
    }
    return;
  }
  if (int_recovery) {
    fdbg("Internal recovery seems to be present\n");
    pages_params.int_recovery_valid = true;
  } else {
    fdbg("External recovery seems to be present\n");
    pages_params.ext_recovery_valid = true;
  }
  if (!int_recovery) {
    f = fopen(recovery_line, "r");
    if (f == NULL) {
      fdbg("Failed to open %s", recovery_line);
      return;
    }
    char buf[256];
    memset(buf, 0, 256);
    fread(buf, 256, sizeof(uint8_t), f);
    fclose(f);
    set_field_buffer(recovery_page.fields[label], 0, buf);
  } else {
    set_field_buffer(recovery_page.fields[label], 0, INT_REC_TXT_FOUND);
  }
}

void
check_rom(const char *rom_path, enum fields label, const char *field_text, bool usb) {
  struct stat st;
  int ret = 0;
  FILE *f;
  char *ptr;
  if (usb) {
    pages_params.usb_rom_valid = false;
  } else {
    pages_params.web_rom_valid = false;
  }
  ret = stat(rom_path, &st);
  if (ret < 0) {
    flog("Failed to stat %s: %i, errno: %s\n", rom_path, ret, strerror(errno));
    if ((!pages_params.usb_rom_valid) && (!pages_params.web_rom_valid)) {
      set_field_buffer(recovery_page.fields[label], 0, ROM_NOTFOUND);
    }
    return;
  }
  f = fopen(rom_path, "r");
  ret = fscanf(f, "%ms", &ptr);
  fclose(f);
  if (ret!=1) {
    if ((!pages_params.usb_rom_valid) && (!pages_params.web_rom_valid)) {
      set_field_buffer(recovery_page.fields[label], 0, ROM_NOTFOUND);
    }
    return;
  }
  ret = stat(ptr, &st);
  if (ret < 0) {
    flog("Failed to stat %s: %i, errno: %s\n", ptr, ret, strerror(errno));
    if ((!pages_params.usb_rom_valid) && (!pages_params.web_rom_valid)) {
      set_field_buffer(recovery_page.fields[label], 0, ROM_NOTFOUND);
    }
    free(ptr);
    return;
  }
  flog("ROM update found, checking size\n");
  if ((16*MB)!=st.st_size) {
    ferr("ROM size is wrong: %lli instead of %i\n", st.st_size, 16*MB);
    if ((!pages_params.usb_rom_valid) && (!pages_params.web_rom_valid)) {
      set_field_buffer(recovery_page.fields[label], 0, ROM_NOTFOUND);
    }
    free(ptr);
    return;
  }
  if ((!pages_params.usb_rom_valid) && (!pages_params.web_rom_valid)) {
    if (strlen(ptr)>=ROM_URL_SIZE) {
      ferr("Path is too large\n");
      if ((!pages_params.usb_rom_valid) && (!pages_params.web_rom_valid)) {
        set_field_buffer(recovery_page.fields[label], 0, ROM_NOTFOUND);
      }
      free(ptr);
      return;
    }
    memset(pages_params.rom_path, 0, ROM_URL_SIZE);
    memcpy(pages_params.rom_path, ptr, strlen(ptr));
    set_field_buffer(recovery_page.fields[label], 0, field_text);
  }
  if (usb) {
    pages_params.usb_rom_valid = true;
  } else {
    pages_params.web_rom_valid = true;
  }
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
  recovery_page.edit_mode = false;
  memset(recovery_page.url, 0, ROM_URL_SIZE);
  recovery_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(recovery_page.wp.w, 0, 0);
  wbkgd(recovery_page.wp.w, PAGE_COLOR);

  recovery_page.wp.p = new_panel(recovery_page.wp.w);

  getmaxyx(recovery_page.wp.w, height, width);
  (void)height;

  mvwaddstr(recovery_page.wp.w, EXT_RECOVERY_LABEL*2+2, 2, "USB recovery");
  recovery_page.fields[EXT_RECOVERY_LABEL] = mk_button(LABEL_WIDTH, 0, EXT_RECOVERY_LABEL, EXT_REC_TXT_NOTFOUND, BG_COLOR);
  mvwaddstr(recovery_page.wp.w, INT_RECOVERY_LABEL*2+2, 2, "Restore backup");
  recovery_page.fields[INT_RECOVERY_LABEL] = mk_button(LABEL_WIDTH, 0, INT_RECOVERY_LABEL*2, INT_REC_TXT_NOTFOUND, BG_COLOR);
  mvwaddstr(recovery_page.wp.w, ROM_UPDATE_LABEL*2+2, 2, "Update ROM");
  recovery_page.fields[ROM_UPDATE_LABEL] = mk_button(LABEL_WIDTH, 0, ROM_UPDATE_LABEL*2, ROM_NOTFOUND, BG_COLOR);
  mvwaddstr(recovery_page.wp.w, ROM_URL_LABEL*2+2, 2, "ROM URL");
  recovery_page.fields[ROM_URL_LABEL] = mk_editable_field_regex_ex(LABEL_WIDTH, 0, ROM_URL_LABEL*2, recovery_page.url, ".*", BG_COLOR, false, false, ROM_URL_SIZE-1);
  mvwaddstr(recovery_page.wp.w, ROM_DOWNLOAD_LABEL*2+2, 2, "Download ROM");
  recovery_page.fields[ROM_DOWNLOAD_LABEL] = mk_button(LABEL_WIDTH, 0, ROM_DOWNLOAD_LABEL*2, ROM_DOWNLOAD, BG_COLOR);
  recovery_page.fields[NULL_VAL] = NULL;
  
  recovery_page.f = new_form(recovery_page.fields);
  scale_form(recovery_page.f, &height, &width);
  set_form_win(recovery_page.f, recovery_page.wp.w);
  recovery_page.sw = derwin(recovery_page.wp.w, height, LABEL_WIDTH, 2, LABEL_WIDTH);
  set_form_sub(recovery_page.f, recovery_page.sw);

  post_form(recovery_page.f);

  redrawwin(recovery_page.wp.w);

  check_int_recovery();
}

int
recovery_page_store(FILE *f) {
  char *url = field_buffer(recovery_page.fields[ROM_URL_LABEL], 0);
  return fprintf(f, "%s\n", url);
}

int
recovery_page_load(FILE *f) {
  char *ptr;
  int l = 0;
  int ret = fscanf(f, "%ms\n", &ptr);
  if (ptr==NULL) {
    return 0;
  }
  if (ret != 1) {
    return 0;
  }
  l = strlen(ptr);
  if (l>=ROM_URL_SIZE) {
    return -3;
  }
  memcpy(recovery_page.url, ptr, l);
  free(ptr);
  set_field_buffer(recovery_page.fields[ROM_URL_LABEL], 0, recovery_page.url);
  return 0;
}

static void
update_background(void) {
  if (recovery_page.edit_mode) {
    FIELD *f = current_field(recovery_page.f);
    if (f == recovery_page.fields[ROM_URL_LABEL]) {
      set_field_back(recovery_page.fields[ROM_URL_LABEL], EDIT_COLOR);
      set_field_fore(recovery_page.fields[ROM_URL_LABEL], EDIT_COLOR);
    }
    //wnoutrefresh(recovery_page.wp.w);
  }
}

int
recovery_page_process(int ch) {
  static uint64_t last_t = 0;
  uint64_t cur_t = time(NULL);
  if (!recovery_page.wp.hidden) {
    curs_set(1);
    if ((cur_t-last_t)>2) {
      last_t = cur_t;
      check_ext_recovery();
      check_int_recovery();
      check_rom(USB_UPDATE_ROM_PATH, ROM_UPDATE_LABEL, ROM_FOUND_USB, true);
      check_rom(WEB_UPDATE_ROM_PATH, ROM_UPDATE_LABEL, ROM_FOUND_WEB, false);
    }
    switch (ch) {
    case RKEY_ENTER://KEY_ENTER:
    {
      flog("Recovery: enter pressed\n");
      FIELD *f = current_field(recovery_page.f);
      if (f == recovery_page.fields[ROM_URL_LABEL]) {
        pages_params.use_arrows = false;
        recovery_page.edit_mode = true;
      } else if (f == recovery_page.fields[EXT_RECOVERY_LABEL]) {
        if (pages_params.ext_recovery_valid) {
          pages_params.start = START_EXT;
          flog("Set start external recovery flag\n");
        } else {
          flog("Recovery is not considered valid\n");
        }
      } else if (f == recovery_page.fields[INT_RECOVERY_LABEL]) {
        if (pages_params.int_recovery_valid) {
          pages_params.start = START_INT;
          flog("Set start internal recovery flag\n");
        } else {
          flog("Recovery is not considered valid\n");
        }
      } else if (f == recovery_page.fields[ROM_UPDATE_LABEL]) {
        if (pages_params.usb_rom_valid || pages_params.web_rom_valid) {
          pages_params.start = START_ROM_UP;
          flog("Set start rom update flag\n");
        } else {
          flog("ROM is not considered valid\n");
        }
      } else if (f == recovery_page.fields[ROM_DOWNLOAD_LABEL]) {
        char *buf = field_buffer(recovery_page.fields[ROM_URL_LABEL], 0);
        //int len = strlen(buf);
        memcpy(pages_params.rom_url, buf, ROM_URL_SIZE);
        pages_params.start = START_ROM_DOWN;
        flog("Set start rom download flag\n");
      }
    }
    break;
    case KEY_DOWN:
      if (!recovery_page.edit_mode) {
        form_driver(recovery_page.f, REQ_NEXT_FIELD);
      }
      break;
    case KEY_UP:
      if (!recovery_page.edit_mode) {
        form_driver(recovery_page.f, REQ_PREV_FIELD);
      }
      break;
    case KEY_BACKSPACE:
		case 127:
      form_driver(recovery_page.f, REQ_DEL_PREV);
      break;
    case KEY_DC:
      form_driver(recovery_page.f, REQ_DEL_CHAR);
			break;
    case RKEY_ESC:
      pages_params.use_arrows = true;
      recovery_page.edit_mode = false;
      set_field_back(recovery_page.fields[ROM_URL_LABEL], BG_COLOR);
      set_field_fore(recovery_page.fields[ROM_URL_LABEL], BG_COLOR);
      form_driver(recovery_page.f, ch);
      break;
    case KEY_LEFT:
      if (recovery_page.edit_mode) {
        form_driver(recovery_page.f, REQ_PREV_CHAR);
      }
      break;
    case KEY_RIGHT:
      if (recovery_page.edit_mode) {
        form_driver(recovery_page.f, REQ_NEXT_CHAR);
      }
      break;
    default:
      if (recovery_page.edit_mode) {
        form_driver(recovery_page.f, ch);
      }
      break;
    }
    wnoutrefresh(recovery_page.wp.w);
    update_background();
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
