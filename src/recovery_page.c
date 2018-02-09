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
#include "field_utils.h"
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
#define COL1_W (LABEL_WIDTH-2)
#define COL2_OFF (COL1_W)
#define COL2_W (LABEL_WIDTH-1)
#define COL3_OFF (COL2_OFF+COL2_W)
#define COL3_W (LABEL_WIDTH+2)
#define COL4_OFF (COL3_OFF+COL3_W)
#define COL4_W (LABEL_WIDTH)

#define KB 1024
#define MB (1024*KB)

enum fields {
  EXT_RECOVERY_LABEL = 0,
  EXT_RECOVERY_BTN,
  INT_RECOVERY_LABEL,
  INT_RECOVERY_BTN,
  ROM_UPDATE_LABEL,
  ROM_UPDATE_BTN,
  ROM_URL_LABEL,
  ROM_URL_VAL,
  ROM_DOWNLOAD_LABEL,
  ROM_DOWNLOAD_BTN,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct field_par fp[] = {
  [EXT_RECOVERY_LABEL] = LABEL_PAR(0, 0, COL1_W, "USB recovery", PAGE_COLOR, PAGE_COLOR),
  [EXT_RECOVERY_BTN] = BUTTON_PAR(COL2_OFF, 0, COL2_W, EXT_REC_TXT_NOTFOUND, BG_COLOR, BG_COLOR),
  
  [INT_RECOVERY_LABEL] = LABEL_PAR(0, 2, COL1_W, "Restore backup", PAGE_COLOR, PAGE_COLOR),
  [INT_RECOVERY_BTN] = BUTTON_PAR(COL2_OFF, 2, COL2_W, INT_REC_TXT_NOTFOUND, BG_COLOR, BG_COLOR),

  [ROM_UPDATE_LABEL] = LABEL_PAR(0, 4, COL1_W, "Update ROM", PAGE_COLOR, PAGE_COLOR),
  [ROM_UPDATE_BTN] = BUTTON_PAR(COL2_OFF, 4, COL2_W, ROM_NOTFOUND, BG_COLOR, BG_COLOR),
  
  [ROM_URL_LABEL] = LABEL_PAR(0, 6, COL1_W, "ROM URL", PAGE_COLOR, PAGE_COLOR),
  [ROM_URL_VAL] = LINE_PAR(COL2_OFF, 6, COL2_W, "", ".*", BG_COLOR, BG_COLOR, false, ROM_URL_SIZE-1, false),
  
  [ROM_DOWNLOAD_LABEL] = LABEL_PAR(0, 8, COL1_W, "Download ROM", PAGE_COLOR, PAGE_COLOR),
  [ROM_DOWNLOAD_BTN] = BUTTON_PAR(COL2_OFF, 8, COL2_W, ROM_DOWNLOAD, BG_COLOR, BG_COLOR),
};

static struct {
  struct window_params wp;
  WINDOW *w;
  char url[ROM_URL_SIZE];
  FIELD *fields[N_FIELDS+1];
  FORM *form;
  bool edit_mode;
  bool selected;
  FIELD *first_active;
  FIELD *last_active;
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
  check_recovery(EXT_RECOVERY_TAR_PATH, EXT_RECOVERY_PATH, EXT_RECOVERY_MDEV, EXT_RECOVERY_LINE, EXT_RECOVERY_BTN, false);
}

void
check_int_recovery(void) {
  check_recovery(INT_RECOVERY_TAR_PATH, INT_RECOVERY_PATH, INT_RECOVERY_MDEV, INT_RECOVERY_LINE, INT_RECOVERY_BTN, true);
}

void
init_recovery_page(void) {
  int width, height;
  int i = 0;
  recovery_page.edit_mode = false;
  recovery_page.selected = false;
  recovery_page.first_active = NULL;
  recovery_page.last_active = NULL;
  recovery_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);
  box(recovery_page.wp.w, 0, 0);
  wbkgd(recovery_page.wp.w, PAGE_COLOR);

  recovery_page.wp.p = new_panel(recovery_page.wp.w);

  getmaxyx(recovery_page.wp.w, height, width);
  (void)height;

  for (i=0; i<N_FIELDS; i++) {
    recovery_page.fields[i] = mk_field(&fp[i]);
    if (fp[i].ft != FT_LABEL) {
      if (recovery_page.first_active == NULL) {
        recovery_page.first_active = recovery_page.fields[i];
      }
      recovery_page.last_active = recovery_page.fields[i];
    }
  }
  recovery_page.fields[NULL_VAL] = NULL;
  
  recovery_page.form = new_form(recovery_page.fields);
  form_opts_off(recovery_page.form, O_NL_OVERLOAD);
  form_opts_off(recovery_page.form, O_BS_OVERLOAD);
  scale_form(recovery_page.form, &height, &width);
  set_form_win(recovery_page.form, recovery_page.wp.w);
  recovery_page.w = derwin(recovery_page.wp.w, height, width, 2, 2);
  set_form_sub(recovery_page.form, recovery_page.w);

  post_form(recovery_page.form);

  redrawwin(recovery_page.wp.w);

  check_int_recovery();
}

int
recovery_page_store(FILE *f) {
  char *url = field_buffer(recovery_page.fields[ROM_URL_VAL], 0);
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
  set_field_buffer(recovery_page.fields[ROM_URL_VAL], 0, recovery_page.url);
  return 0;
}

int
recovery_page_process(int ch) {
  static uint64_t last_t = 0;
  uint64_t cur_t = time(NULL);
  if (!recovery_page.wp.hidden) {
    if ((cur_t-last_t)>2) {
      last_t = cur_t;
      check_ext_recovery();
      check_int_recovery();
      check_rom(USB_UPDATE_ROM_PATH, ROM_UPDATE_BTN, ROM_FOUND_USB, true);
      check_rom(WEB_UPDATE_ROM_PATH, ROM_UPDATE_BTN, ROM_FOUND_WEB, false);
    }
    FIELD *f = current_field(recovery_page.form);
    if (!recovery_page.selected) {
      curs_set(0);
      switch (ch) {
      case KEY_DOWN:
        recovery_page.selected = true;
        pages_params.use_arrows = false;
        recovery_page.edit_mode = false;
        break;
      }
    } else {
      curs_set(1);
      switch (ch) {
      case KEY_DOWN:
        if (recovery_page.edit_mode) {
          field_par_unset_line_bg(fp, recovery_page.fields, N_FIELDS);
          recovery_page.edit_mode = false;
        }
        if (f != recovery_page.last_active) {
          form_driver(recovery_page.form, REQ_NEXT_FIELD);
        }
        break;
      case KEY_UP:
        if (recovery_page.edit_mode) {
          field_par_unset_line_bg(fp, recovery_page.fields, N_FIELDS);
          recovery_page.edit_mode = false;
        }
        if (f != recovery_page.first_active) {
          form_driver(recovery_page.form, REQ_PREV_FIELD);
        } else {
          recovery_page.selected = false;
          pages_params.use_arrows = true;
        }
        break;
      case RKEY_ENTER:
      {
        flog("enter pressed\n");
        FIELD *f = current_field(recovery_page.form);
        if (f == recovery_page.fields[EXT_RECOVERY_BTN]) {
          if (pages_params.ext_recovery_valid) {
            pages_params.start = START_EXT;
            flog("Set start external recovery flag\n");
          } else {
            flog("Recovery is not considered valid\n");
          }
        } else if (f == recovery_page.fields[INT_RECOVERY_BTN]) {
          if (pages_params.int_recovery_valid) {
            pages_params.start = START_INT;
            flog("Set start internal recovery flag\n");
          } else {
            flog("Recovery is not considered valid\n");
          }
        } else if (f == recovery_page.fields[ROM_UPDATE_BTN]) {
          if (pages_params.usb_rom_valid || pages_params.web_rom_valid) {
            pages_params.start = START_ROM_UP;
            flog("Set start rom update flag\n");
          } else {
            flog("ROM is not considered valid\n");
          }
        } else if (f == recovery_page.fields[ROM_DOWNLOAD_BTN]) {
          char *buf = field_buffer(recovery_page.fields[ROM_URL_VAL], 0);
          //int len = strlen(buf);
          memcpy(pages_params.rom_url, buf, ROM_URL_SIZE);
          pages_params.start = START_ROM_DOWN;
          flog("Set start rom download flag\n");
        }
      }
      break;
      case RKEY_ESC:
        break;
      case KEY_LEFT:
        if (field_opts(f) & O_EDIT) {
          recovery_page.edit_mode = true;
          field_par_set_line_bg(fp, f, recovery_page.fields, N_FIELDS);
          form_driver(recovery_page.form, REQ_PREV_CHAR);
        }
        break;
      case KEY_RIGHT:
        if (field_opts(f) & O_EDIT) {
          recovery_page.edit_mode = true;
          field_par_set_line_bg(fp, f, recovery_page.fields, N_FIELDS);
          form_driver(recovery_page.form, REQ_NEXT_CHAR);
        }
        break;
      case KEY_BACKSPACE:
      case 127:
        recovery_page.edit_mode = true;
        field_par_set_line_bg(fp, f, recovery_page.fields, N_FIELDS);
        form_driver(recovery_page.form, REQ_DEL_PREV);
        break;
      case KEY_DC:
        recovery_page.edit_mode = true;
        field_par_set_line_bg(fp, f, recovery_page.fields, N_FIELDS);
        form_driver(recovery_page.form, REQ_DEL_CHAR);
        break;
      case -1:
        break;
      default:
        recovery_page.edit_mode = true;
        field_par_set_line_bg(fp, f, recovery_page.fields, N_FIELDS);
        form_driver(recovery_page.form, ch);
        break;
      }
    }
  }
  return 0;
}

void
deinit_recovery_page(void) {
  int i;
  unpost_form(recovery_page.form);
	free_form(recovery_page.form);
  for (i=0; i<N_FIELDS; i++)
    free_field(recovery_page.fields[i]);
  delwin(recovery_page.wp.w);
}

struct window_params *
get_recovery_page_wp(void) {
  return &recovery_page.wp;
}
