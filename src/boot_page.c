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

#include "field_utils.h"

#include "common.h"
#include "pages.h"
//#include "boot_page.h"
#include "top_menu.h"
#include "fru.h"

#define BMC_MIN_VERSION_PP (2<<16 | 1)

#define TAG "BOOT_PAGE"

#define LABEL_WIDTH 25

#define COL1_W (LABEL_WIDTH-2)
#define COL2_OFF (COL1_W)
#define COL2_W (LABEL_WIDTH-3)
#define COL3_OFF (COL2_OFF+COL2_W)
#define COL3_W (LABEL_WIDTH+2)
#define COL4_OFF (COL3_OFF+COL3_W)
#define COL4_W (LABEL_WIDTH)


enum fields {
  BOOT_PRIORITY_LABEL=0,
  BOOT_SATA_PORT_LABEL,
  BOOT_SATA_PORT_SPN,
  POWER_POLICY_LABEL,
  POWER_POLICY_SPN,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct field_par fp[] = {
  [BOOT_PRIORITY_LABEL] = LABEL_PAR(0, 0, COL1_W, "SATA boot priority", PAGE_COLOR, PAGE_COLOR),
  [BOOT_SATA_PORT_LABEL] = LABEL_PAR(0, 2, COL1_W, "SATA boot port", PAGE_COLOR, PAGE_COLOR),
  [BOOT_SATA_PORT_SPN] = SPINNER_PAR(COL2_OFF, 2, COL2_W, "sata0:0\0sata0:1\0;", 0, BG_COLOR, BG_COLOR),
  [POWER_POLICY_LABEL] = LABEL_PAR(0, 4, COL1_W, "Power restore policy", PAGE_COLOR, PAGE_COLOR),
  [POWER_POLICY_SPN] = SPINNER_PAR(COL2_OFF, 4, COL2_W, "always off\0always on\0;", 0, BG_COLOR, BG_COLOR)
};


static struct {
  struct window_params wp;
  WINDOW *w;
  FIELD *fields[N_FIELDS+1];
  FORM *form;
  bool edit_mode;
  bool selected;
  FIELD *first_active;
  FIELD *last_active;
} boot_page;

void
init_boot_page(void) {
  int width, height;
  int i = 0;
  boot_page.edit_mode = false;
  boot_page.selected = false;
  boot_page.first_active = NULL;
  boot_page.last_active = NULL;
  boot_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);
  box(boot_page.wp.w, 0, 0);
  wbkgd(boot_page.wp.w, PAGE_COLOR);

  boot_page.wp.p = new_panel(boot_page.wp.w);

  uint32_t bmc_version = 0;
  bmc_version |= pages_params.bmc_version[0]<<16;
  bmc_version |= pages_params.bmc_version[1]<<8;
  bmc_version |= pages_params.bmc_version[2];

  flog("Check power policy spinner\n");
  if (bmc_version <= BMC_MIN_VERSION_PP) {
    fp[POWER_POLICY_SPN].ft = FT_LABEL;
    fp[POWER_POLICY_SPN].f = FT_LABEL;
    fp[POWER_POLICY_SPN].x = COL2_OFF;
    fp[POWER_POLICY_SPN].y = 4;
    fp[POWER_POLICY_SPN].w = COL2_W;
    fp[POWER_POLICY_SPN].txt = "N/A";
    fp[POWER_POLICY_SPN].regex = NULL;
    fp[POWER_POLICY_SPN].fore = PAGE_COLOR;
    fp[POWER_POLICY_SPN].back = PAGE_COLOR;
    fp[POWER_POLICY_SPN].static_size = true;
    fp[POWER_POLICY_SPN].max_growth = 0;
    fp[POWER_POLICY_SPN].autoskip = true;
    fp[POWER_POLICY_SPN].active = false;
    fp[POWER_POLICY_SPN].iarg = 0;
  }

  getmaxyx(boot_page.wp.w, height, width);
  (void)height;
  flog("Init boot page fields\n");
  for (i=0; i<N_FIELDS; i++) {
    boot_page.fields[i] = mk_field(&fp[i]);
    if (fp[i].ft != FT_LABEL) {
      if (boot_page.first_active == NULL) {
        boot_page.first_active = boot_page.fields[i];
      }
      boot_page.last_active = boot_page.fields[i];
    }
  }
  boot_page.fields[NULL_VAL] = NULL;
  
  boot_page.form = new_form(boot_page.fields);
  form_opts_off(boot_page.form, O_NL_OVERLOAD);
  form_opts_off(boot_page.form, O_BS_OVERLOAD);
  scale_form(boot_page.form, &height, &width);
  set_form_win(boot_page.form, boot_page.wp.w);
  boot_page.w = derwin(boot_page.wp.w, height, width, 2, 2);
  set_form_sub(boot_page.form, boot_page.w);
  post_form(boot_page.form);
  redrawwin(boot_page.wp.w);

  flog("Check port spinner\n");
  flog("fru.bootdevice: %s\n", fru.bootdevice);
  const char sata0port1[] = "sata0:1";
  if (strncmp((char *)fru.bootdevice, sata0port1, strlen(sata0port1))==0) {
    spinner_spin(boot_page.fields[BOOT_SATA_PORT_SPN]);
  }

  flog("Check power policy spinner\n");
  if (bmc_version > BMC_MIN_VERSION_PP) {
    if (fru.power_policy>PP_NUM) {
      fru.power_policy = 0;
    }
    //fix power policy here
    if (fru.power_policy >= PP_KEEP) {
      spinner_spin(boot_page.fields[POWER_POLICY_SPN]);
    }
  }
}

void
boot_save_bootdev(void) {
  flog("Save boot device\n");
  char *ptr = field_buffer(boot_page.fields[BOOT_SATA_PORT_SPN], 0);
  int i = 0;
  fmsg("PTR:\n");
  for (;i<16;i++) {
    fmsg("%02x ", ptr[i]);
    if (ptr[i] == ' ') {
      ptr[i] = '\0';
      break;
    }
  }
  fmsg("\n");
  flog("Obtained buffer: [%s]\n", ptr);
  fru_mrec_update_bootdevice(&fru, (uint8_t *)ptr);
}

void
boot_save_power_policy(void) {
  flog("Save power policy\n");
  uint32_t bmc_version = 0;
  bmc_version |= pages_params.bmc_version[0]<<16;
  bmc_version |= pages_params.bmc_version[1]<<8;
  bmc_version |= pages_params.bmc_version[2];
  if (bmc_version > BMC_MIN_VERSION_PP) {
    int i = spinner_current_index(boot_page.fields[POWER_POLICY_SPN]);
    if (i == PP_KEEP) {
      fwarn("PP_KEEP support is disabled");
      i = PP_ON;
    }
    fru_mrec_update_power_policy(&fru, i);
    fru_mrec_update_power_state(&fru);
    return;
  }
}

int
boot_page_process(int ch) {
  FIELD *f = current_field(boot_page.form);
  uint32_t bmc_version = 0;
  bmc_version |= pages_params.bmc_version[0]<<16;
  bmc_version |= pages_params.bmc_version[1]<<8;
  bmc_version |= pages_params.bmc_version[2];

  if (!boot_page.wp.hidden) {
    if (!boot_page.selected) {
      curs_set(0);
      switch (ch) {
      case KEY_DOWN:
        boot_page.selected = true;
        pages_params.use_arrows = false;
        boot_page.edit_mode = false;
        break;
      }
    } else {
      curs_set(1);
      switch (ch) {
      case KEY_DOWN:
        if (boot_page.edit_mode) {
          field_par_unset_line_bg(fp, boot_page.fields, N_FIELDS);
          boot_page.edit_mode = false;
        }
        if (f != boot_page.last_active) {
          form_driver(boot_page.form, REQ_NEXT_FIELD);
        }
        break;
      case KEY_UP:
        if (boot_page.edit_mode) {
          field_par_unset_line_bg(fp, boot_page.fields, N_FIELDS);
          boot_page.edit_mode = false;
        }
        if (f != boot_page.first_active) {
          form_driver(boot_page.form, REQ_PREV_FIELD);
        } else {
          boot_page.selected = false;
          pages_params.use_arrows = true;
        }
        break;
      case RKEY_ENTER:
        {
          if (f == boot_page.fields[BOOT_SATA_PORT_SPN]) {
            spinner_spin(f);
            boot_save_bootdev();
          } else if ((bmc_version > BMC_MIN_VERSION_PP) && (f == boot_page.fields[POWER_POLICY_SPN])) {
            spinner_spin(f);
            boot_save_power_policy();
          }
        }
        break;
      case RKEY_ESC:
        break;
      case KEY_LEFT:
        break;
      case KEY_RIGHT:
        break;
      case KEY_BACKSPACE:
      case 127:
        break;
      case KEY_DC:
        break;
      case -1:
        break;
      default:
        break;
      }
    }
    wnoutrefresh(boot_page.wp.w);
  }
  return 0;
}

void
deinit_boot_page(void) {
  int i;
  unpost_form(boot_page.form);
	free_form(boot_page.form);
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
