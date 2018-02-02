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

#include "common.h"
#include "pages.h"
#include "main_page.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "MAIN_PAGE"

#define SHRED_GPIO_BASE 32
#define SHRED_NGPIO 8

#define LABEL_WIDTH 25

#define SYS_TEMP_PATH "/sys/bus/platform/drivers/pvt-hwmon/1f200000.pvt/temp1_input"
#define SYS_COREV_PATH "/sys/bus/platform/drivers/pvt-hwmon/1f200000.pvt/in1_input"

enum BMC_BOOTREASON {
  BR_UNKNOWN=0,
  BR_NORMAL,
  BR_BOOTCONF_FAIL,
  BR_NOT_TESTED,
  BR_TEST_FAIL
};

enum fields_col1 {
  BOOTREASON_VAL=0,
  TESTOK_VAL,
  TIMESTAT_VAL,
  TIME_VAL,
  DATE_VAL,
  TEMP_VAL,
  VOLTAGE_VAL,
  SHRED_VAL,
  BMC_PROTO_VAL,
  RFS_VAL,
  KERNEL_VAL,
  
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

enum fields_col2 {
  BRD_MFG_DATE_VAL = 0,
  BRD_MFG_NAME_VAL,
  BRD_PRODUCT_NAME_VAL,
  BRD_SERIAL_VAL,
  BRD_PN_VAL,
  BRD_FRU_ID_VAL,
  
  PRODUCT_MFG_VAL,
  PART_MODEL_NUMBER_VAL,

  PRODUCT_VERSION_VAL,
  PRODUCT_SERIAL_VAL,
  PRODUCT_FRU_ID_VAL,
  
  COL2_NULL_VAL,
  COL2_N_FIELDS=COL2_NULL_VAL
};

static struct {
  struct window_params wp;
  WINDOW *w;
  WINDOW *sw;
  uint32_t shred;
  char shred_label[20];
  char time_label[20];
  char temp_label[20];
  char voltage_label[20];
  char mfg_date_label[20];
  char date_label[20];
  char bmc_version_label[20];
  char rfs_version[20];
  char kernel_release[20];
  FIELD *fields_col1[N_FIELDS+1];
	FORM  *f1;
  FIELD *fields_col2[COL2_N_FIELDS+1];
	FORM  *f2;

} main_page;

FIELD *mk_spinner(int w, int x, int y, char **strings, int n_str, int default_n, chtype c) {
  FIELD *f = new_field(1, w, y, x, 0, 0);
  struct spinner_arg *s = (struct spinner_arg *)malloc(sizeof(struct spinner_arg));
  s->current_str = default_n;
  s->n_str = n_str;
  s->strs = strings;
  set_field_userptr(f, (void *)s);
  field_opts_off(f, O_EDIT);
  set_field_buffer(f, 0, strings[default_n]);
  set_field_fore(f, c);
  set_field_back(f, c);
  return f;  
}

void
spinner_spin(FIELD *f) {
  void *p = field_userptr(f);
  if (p==NULL) {
    return;
  }
  struct spinner_arg *s = (struct spinner_arg *)p;
  s->current_str++;
  s->current_str%= s->n_str;
  set_field_buffer(f, 0, s->strs[s->current_str]);
}

FIELD *
mk_button(int w, int x, int y, char *string, chtype c) {
  FIELD *f = new_field(1, w, y, x, 0, 0);
  field_opts_off(f, O_EDIT);
  set_field_buffer(f, 0, string);
  set_field_fore(f, c);
  set_field_back(f, c);
  return f;
}

FIELD *
mk_label(int w, int x, int y, char *string, chtype c) {
  FIELD *f = new_field(1, w, y, x, 0, 0);
  field_opts_off(f, O_EDIT);
  field_opts_off(f, O_ACTIVE);
  set_field_buffer(f, 0, string);
  set_field_fore(f, c);
  set_field_back(f, c);
  return f;
}

FIELD *
mk_label_colored(int w, int x, int y, char *string, chtype fore, chtype back) {
  FIELD *f = new_field(1, w, y, x, 0, 0);
  field_opts_off(f, O_EDIT);
  set_field_buffer(f, 0, string);
  set_field_fore(f, fore);
  set_field_back(f, back);
  return f;
}

/* void */
/* mk_st_label(WINDOW *w, int x, int y, char *string chtype c) { */
  
/* } */

FIELD *
mk_label2(int w, int h, int x, int y, char *string, chtype c) {
  FIELD *f = new_field(h, w, y, x, 0, 0);
  field_opts_off(f, O_EDIT);
  set_field_buffer(f, 0, string);
  set_field_fore(f, c);
  set_field_back(f, c);
  return f;
}

FIELD *
mk_editable_field_regex_ex(int w, int x, int y, char *string, char *regex, chtype c, bool autoskip, bool static_size, int max_growth) {
  FIELD *f = new_field(1, w, y, x, 0, 0);
  set_max_field(f, w);
  set_field_type(f, TYPE_REGEXP, regex);
  uint32_t default_opts = O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE | O_BLANK;
  if (autoskip) {
    default_opts |= O_AUTOSKIP;
  }
  set_field_opts(f, default_opts);
  if (!static_size) {
    field_opts_off(f, O_STATIC);
    set_max_field(f, max_growth);
  }
  if (!autoskip) {
    field_opts_off(f, O_AUTOSKIP);
  }
  set_field_buffer(f, 0, string);
  set_field_fore(f, c);
  set_field_back(f, c);
  return f;
}

FIELD *
mk_editable_field_regex(int w, int x, int y, char *string, char *regex, chtype c) {
  return mk_editable_field_regex_ex(w, x, y, string, regex, c, true, true, 0);
}

uint32_t read_shred(void) {
#ifdef REAL_DEVICES
  uint32_t val = 0;
  int ret = 0;
  uint32_t shred_val = 0;
  int i = 0;
  char buf[64] = {0};
  for (i=0;i<SHRED_NGPIO;i++) {
    log("Opening /sys/class/gpio/export\n");
    FILE *export = fopen("/sys/class/gpio/export", "w");
    if (export == NULL) {
      err("Failed to open /sys/class/gpio/export\n");
      return val;
    }
    ret = sprintf(buf, "%i", i+SHRED_GPIO_BASE);
    log("Exporting pin %s[len: %i]\n", buf, ret);
    fwrite(buf, sizeof(char), ret, export);
    fclose(export);
    sprintf(buf, "/sys/class/gpio/gpio%i/value", i+SHRED_GPIO_BASE);
    log("Opening %s\n", buf);
    FILE *gpio = fopen(buf, "r");
    if (gpio == NULL) {
      err("Failed to open %s\n", buf);
      fclose(export);
      return shred_val;
    }
    ret = fscanf(gpio, "%i", &val);
    log("gpio[%i] read returned %i, value: %i\n", i+SHRED_GPIO_BASE, ret, val);
    shred_val |= (val!=0?(1<<i):0);
    fclose(gpio);
  }
  return shred_val;
#else
  return FAKE_SHRED;
#endif
}

void read_bmc_version(void) {
  struct stat st;
  int ret;
  int reason;
  pages_params.bmc_version[0] = 0;
  pages_params.bmc_version[1] = 0;
  pages_params.bmc_version[2] = 0;
  pages_params.boot_reason[0] = 0;
  pages_params.boot_reason[1] = 0;
  FILE *version = fopen("/sys/bus/i2c/drivers/mitx2-bmc/version", "r");
  if (version == NULL) {
    err("Failed to open /sys/bus/i2c/drivers/mitx2-bmc/version\n");
    return;
  }
  ret = fscanf(version, "%i.%i.%i", &pages_params.bmc_version[0], &pages_params.bmc_version[1], &pages_params.bmc_version[2]);
  fclose(version);
  log("BMC version read returned %i, value: %i.%i.%i\n", ret, pages_params.bmc_version[0], pages_params.bmc_version[1], pages_params.bmc_version[2]);
  if (pages_params.bmc_version[0] >= 2) {
    ret = stat("/sys/bus/i2c/drivers/mitx2-bmc/bootreason", &st);
    if (ret<0) {
      log("Failed to stat bmc sysfs bootreason, assuming ancient BMC\n");
      return;
    }
    FILE *bootreason = fopen("/sys/bus/i2c/drivers/mitx2-bmc/bootreason", "r");
    if (bootreason == NULL) {
      err("Failed to open /sys/bus/i2c/drivers/mitx2-bmc/bootreason\n");
      return;
    }
    ret = fscanf(bootreason, "%i", &reason);
    fclose(bootreason);
    pages_params.boot_reason[0] = reason & 0xff;
    pages_params.boot_reason[1] = (reason>>8) & 0xff;
    log("Bootreason: %i/%i\n", pages_params.boot_reason[0], pages_params.boot_reason[1]);
  }
}

int
read_pvt(void) {
  FILE * pvt_temp_file = fopen(SYS_TEMP_PATH, "r");
  if (pvt_temp_file == NULL) {
    err("Failed to open %s\n", SYS_TEMP_PATH);
    sprintf(main_page.temp_label, "%.3f", 0.0f);
    return -1;
  }
  int pvt_temp = 0;
  fscanf(pvt_temp_file, "%i", &pvt_temp);
  fclose(pvt_temp_file);
  sprintf(main_page.temp_label, "%.3f Deg.C", ((float)pvt_temp)/1000.0f);

  FILE * pvt_core_voltage_file = fopen(SYS_COREV_PATH, "r");
  if (pvt_core_voltage_file == NULL) {
    err("Failed to open %s\n", SYS_COREV_PATH);
    sprintf(main_page.voltage_label, "%.3f", 0.0f);
    return -2;
  }
  int pvt_corev = 0;
  fscanf(pvt_core_voltage_file, "%i", &pvt_corev);
  fclose(pvt_core_voltage_file);
  sprintf(main_page.voltage_label, "%.3f V", ((float)pvt_corev)/1000.0f);
  return 0;
}

void
init_main_page(void) {
  int width, height;
  time_t t;
  struct tm tm;
  int y = 0;
  read_bmc_version();
  main_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(main_page.wp.w, 0, 0);
  wbkgd(main_page.wp.w, PAGE_COLOR);

  main_page.wp.p = new_panel(main_page.wp.w);

  getmaxyx(main_page.wp.w, height, width);
  (void)height;

  main_page.shred = read_shred();
  log("shred: 0x%08x\n", main_page.shred);
  t = time(NULL);
  tm = *localtime(&t);
  read_pvt();

  mvwaddstr(main_page.wp.w, y+2, 2, "Boot status");

  if (pages_params.boot_reason[0] == BR_NORMAL) {
    main_page.fields_col1[BOOTREASON_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "NORMAL", GREEN_COLOR);
  } else if (pages_params.boot_reason[0] == BR_BOOTCONF_FAIL) {
    main_page.fields_col1[BOOTREASON_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "BOOTCONF FAIL", RED_COLOR);
  } else if (pages_params.boot_reason[0] == BR_TEST_FAIL) {
    main_page.fields_col1[BOOTREASON_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "HW TEST FAIL", RED_COLOR);
  } else if (pages_params.boot_reason[0] == BR_NOT_TESTED) {
    main_page.fields_col1[BOOTREASON_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "NOT TESTED", RED_COLOR);
  } else {
    main_page.fields_col1[BOOTREASON_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "UNKNOWN", RED_COLOR);
  }
  y+=2;

  log("Test ok: %i\n", fru.test_ok);
  mvwaddstr(main_page.wp.w, y+2, 2, "MFG HW test status");
  if (fru.test_ok==1) {
    main_page.fields_col1[TESTOK_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "PASSED", GREEN_COLOR);
    log("mfg hw test status field len: %i\n", strlen("PASSED"));
  } else if (fru.test_ok==2) {
    main_page.fields_col1[TESTOK_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "FAILED", RED_COLOR);
    log("mfg hw test status field len: %i\n", strlen("FAILED"));
  } else {
    main_page.fields_col1[TESTOK_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "UNKNOWN", PAGE_COLOR);
    log("mfg hw test status field len: %i\n", strlen("UNKNOWN"));
  }
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "RTC status");
  if ((tm.tm_year + 1900)==1970) {
    main_page.fields_col1[TIMESTAT_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "FAIL", RED_COLOR);
  } else {
    main_page.fields_col1[TIMESTAT_VAL] = mk_label(LABEL_WIDTH-3, 0, y, "OK", GREEN_COLOR);
  }
  y+=2;
  
  mvwaddstr(main_page.wp.w, y+2, 2, "Time");
  sprintf(main_page.time_label, "%02i:%02i:%02i UTC", tm.tm_hour, tm.tm_min, tm.tm_sec);
  log("time field len: %i\n", strlen(main_page.time_label));
	main_page.fields_col1[TIME_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.time_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Date");
  sprintf(main_page.date_label, "%04i-%02i-%02i", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
  log("date field len: %i\n", strlen(main_page.date_label));
	main_page.fields_col1[DATE_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.date_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Core temperature");
  log("temp field len: %i\n", strlen(main_page.temp_label));
	main_page.fields_col1[TEMP_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.temp_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Core voltage");
  log("voltage field len: %i\n", strlen(main_page.voltage_label));
	main_page.fields_col1[VOLTAGE_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.voltage_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "SHRED");
  sprintf(main_page.shred_label, "%02x", main_page.shred);
  log("shred field len: %i\n", strlen(main_page.shred_label));
	main_page.fields_col1[SHRED_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.shred_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "BMC protocol version");
  sprintf(main_page.bmc_version_label, "%i.%i.%i", pages_params.bmc_version[0], pages_params.bmc_version[1], pages_params.bmc_version[2]);
  log("bmc version field len: %i\n", strlen(main_page.bmc_version_label));
	main_page.fields_col1[BMC_PROTO_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.bmc_version_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Recovery rfs version");
  FILE *rfs = fopen("/etc/recovery-version", "r");
  if (rfs != NULL) {
    fread(main_page.rfs_version, sizeof(char), 20, rfs);
    fclose(rfs);
    main_page.fields_col1[RFS_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.rfs_version, PAGE_COLOR);
  } else {
    sprintf(main_page.rfs_version, "UNKNOWN");
    main_page.fields_col1[RFS_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.rfs_version, RED_COLOR);
  }
  log("rfs version field len: %i\n", strlen(main_page.rfs_version));
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Kernel release");
  FILE *kernel_release = fopen("/proc/sys/kernel/osrelease", "r");
  if (rfs != NULL) {
    fread(main_page.kernel_release, sizeof(char), 20, kernel_release);
    fclose(kernel_release);
    main_page.fields_col1[KERNEL_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.kernel_release, PAGE_COLOR);
  } else {
    sprintf(main_page.kernel_release, "UNKNOWN");
    main_page.fields_col1[KERNEL_VAL] = mk_label(LABEL_WIDTH-3, 0, y, main_page.kernel_release, RED_COLOR);
  }
  log("kernel version field len: %i\n", strlen(main_page.kernel_release));
  y+=2;

  
  main_page.fields_col1[NULL_VAL] = NULL;
  
  main_page.f1 = new_form(main_page.fields_col1);
  scale_form(main_page.f1, &height, &width);
  set_form_win(main_page.f1, main_page.wp.w);
  main_page.sw = derwin(main_page.wp.w, height, LABEL_WIDTH-3, 2, LABEL_WIDTH);
  set_form_sub(main_page.f1, main_page.sw);

  post_form(main_page.f1);
  
  y=0;

  tm.tm_year = 1996-1900;
  tm.tm_mon = 0;           // Month, 0 - jan
  tm.tm_mday = 1;          // Day of the month
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
  t = mktime(&tm);
  int offset = (fru.mfg_date[2] | (fru.mfg_date[1]<<8) | (fru.mfg_date[0]<<16));
  log("offset: 0x%08x\n", offset);
  offset*=60;
  t += offset;
  tm = *localtime(&t);
  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Board MFG Date");
  strftime(main_page.mfg_date_label, 20, "%Y-%m-%d", &tm);
  main_page.fields_col2[BRD_MFG_DATE_VAL] = mk_label(LABEL_WIDTH, 0, y, main_page.mfg_date_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Board manufacturer");
	main_page.fields_col2[BRD_MFG_NAME_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_mfg_name, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Board product name");
	main_page.fields_col2[BRD_PRODUCT_NAME_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_product_name, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Board serial number");
	main_page.fields_col2[BRD_SERIAL_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_serial_number, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Board part number");
	main_page.fields_col2[BRD_PN_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_part_number, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Board FRU file ID");
	main_page.fields_col2[BRD_FRU_ID_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_fru_id, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Product manufacturer");
	main_page.fields_col2[PRODUCT_MFG_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_p_product_mfg, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Product part/model number");
	main_page.fields_col2[PART_MODEL_NUMBER_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_p_part_model_number, PAGE_COLOR);
  y+=2;
  
  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Product version");
  main_page.fields_col2[PRODUCT_VERSION_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_p_product_version, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Product serial number");
	main_page.fields_col2[PRODUCT_SERIAL_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_p_serial_number, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2*LABEL_WIDTH-3, "Product FRU file ID");
  main_page.fields_col2[PRODUCT_FRU_ID_VAL] = mk_label(LABEL_WIDTH, 0, y, (char *)fru.val_p_fru_id, PAGE_COLOR);
  y+=2;

  main_page.fields_col2[COL2_NULL_VAL] = NULL;
  
  main_page.f2 = new_form(main_page.fields_col2);
  scale_form(main_page.f2, &height, &width);
  set_form_win(main_page.f2, main_page.wp.w);
  main_page.sw = derwin(main_page.wp.w, height, LABEL_WIDTH, 2, 3*LABEL_WIDTH-1);
  set_form_sub(main_page.f2, main_page.sw);

  post_form(main_page.f2);

  redrawwin(main_page.wp.w);
}

int
main_page_process(int ch) {
  if (!main_page.wp.hidden) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    curs_set(0);

    sprintf(main_page.time_label, "%02i:%02i:%02i UTC", tm.tm_hour, tm.tm_min, tm.tm_sec);
    set_field_buffer(main_page.fields_col1[TIME_VAL], 0, main_page.time_label);
    sprintf(main_page.date_label, "%04i-%02i-%02i", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
    set_field_buffer(main_page.fields_col1[DATE_VAL], 0, main_page.date_label);

    read_pvt();
    set_field_buffer(main_page.fields_col1[TEMP_VAL], 0, main_page.temp_label);
    set_field_buffer(main_page.fields_col1[VOLTAGE_VAL], 0, main_page.voltage_label);

    wnoutrefresh(main_page.wp.w);
  }
  return 0;
}

void
deinit_main_page(void) {
  int i;
  unpost_form(main_page.f1);
	free_form(main_page.f1);
  for (i=0; i<N_FIELDS; i++)
    free_field(main_page.fields_col1[i]);

  unpost_form(main_page.f2);
	free_form(main_page.f2);
  for (i=0; i<COL2_N_FIELDS; i++)
    free_field(main_page.fields_col2[i]);

  delwin(main_page.wp.w);
}

struct window_params *
get_main_page_wp(void) {
  return &main_page.wp;
}
