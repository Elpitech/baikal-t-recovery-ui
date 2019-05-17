#include <sys/stat.h>
#include <sys/ioctl.h>
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
#include <linux/rtc.h>
#include <time.h>
#include <fcntl.h>
#include <glob.h>

#include "common.h"
#include "pages.h"
#include "main_page.h"
#include "top_menu.h"
#include "fru.h"

#include "field_utils.h"

#define TAG "MAIN_PAGE"

#define SHRED_NGPIO 8

#define LABEL_WIDTH 25

#define SYS_TEMP_PATH "/sys/bus/platform/drivers/pvt-hwmon/1f200000.pvt/temp1_input"
#define SYS_COREV_PATH "/sys/bus/platform/drivers/pvt-hwmon/1f200000.pvt/in1_input"
#define PROC_OSRELEASE_PATH "/proc/sys/kernel/osrelease"

#if defined(BOARD_MITX4)
#define GPIOCHIP_GLOB "/sys/bus/i2c/devices/1-0021/gpio/*/base"
#elif defined (BOARD_BN1BT1)
#define GPIOCHIP_GLOB "/sys/bus/i2c/devices/1-0024/gpio/*/base"
#endif
#define BMC_VERSION_GLOB "/sys/bus/i2c/drivers/*-bmc/version"
#define BOOTREASON_GLOB "/sys/bus/i2c/drivers/*-bmc/bootreason"

#define COL1_W (LABEL_WIDTH-2)
#define COL2_OFF (COL1_W)
#define COL2_W (LABEL_WIDTH-3)
#define COL3_OFF (COL2_OFF+COL2_W)
#define COL3_W (LABEL_WIDTH+2)
#define COL4_OFF (COL3_OFF+COL3_W)
#define COL4_W (LABEL_WIDTH)

#define OFF_YEAR COL2_OFF
#define W_YEAR 4
#define OFF_MONTH (OFF_YEAR+W_YEAR+1)
#define W_MONTH 2
#define OFF_DAY (OFF_MONTH+W_MONTH+1)
#define W_DAY 2

#define OFF_HR COL2_OFF
#define W_HR 2
#define OFF_MN (OFF_HR+W_HR+1)
#define W_MN 2
#define OFF_SC (OFF_MN+W_MN+1)
#define W_SC 2

enum fields {
  BOOT_STATUS_LABEL=0,
  BOOT_STATUS_VAL,
  
  MFG_HW_TEST_STATUS_LABEL,
  MFG_HW_TEST_STATUS_VAL,
  
  RTC_STATUS_LABEL,
  RTC_STATUS_VAL,
  
  TIME_LABEL,
  TIME_HR_VAL, TIME_COLON_1, TIME_MN_VAL, TIME_COLON_2, TIME_SC_VAL,
  
  DATE_LABEL,
  DATE_YEAR_VAL, DATE_DASH_1, DATE_MONTH_VAL, DATE_DASH_2, DATE_DAY_VAL,
  
  CORE_TEMP_LABEL,
  CORE_TEMP_VAL,
  
  CORE_VOLT_LABEL,
  CORE_VOLT_VAL,
  
  SHRED_LABEL,
  SHRED_VAL,
  
  BMC_PROTO_VER_LABEL,
  BMC_PROTO_VER_VAL,
  
  RFS_VER_LABEL,
  RFS_VER_VAL,
  
  KERNEL_RELEASE_LABEL,
  KERNEL_RELEASE_VAL,

  BOARD_MFG_DATE_LABEL,
  BOARD_MFG_DATE_VAL,

  BOARD_MANUFACTURER_LABEL,
  BOARD_MANUFACTURER_VAL,

  BOARD_PRODUCT_NAME_LABEL,
  BOARD_PRODUCT_NAME_VAL,

  BOARD_SERIAL_LABEL,
  BOARD_SERIAL_VAL,

  BOARD_PART_NUMBER_LABEL,
  BOARD_PART_NUMBER_VAL,

  BOARD_FRU_ID_LABEL,
  BOARD_FRU_ID_VAL,

  PRODUCT_MANUFACTURER_LABEL,
  PRODUCT_MANUFACTURER_VAL,

  PRODUCT_PART_MODEL_NUMBER_LABEL,
  PRODUCT_PART_MODEL_NUMBER_VAL,

  PRODUCT_VERSION_LABEL,
  PRODUCT_VERSION_VAL,
  
  NULL_VAL,
  N_FIELDS=NULL_VAL
};


static struct field_par fp[] = {
  //left column
  [BOOT_STATUS_LABEL] = LABEL_PAR(0, 0, COL1_W, "Boot status", PAGE_COLOR, PAGE_COLOR),
  [BOOT_STATUS_VAL] = LABEL_PAR(COL2_OFF, 0, COL2_W, "N/A", PAGE_COLOR, PAGE_COLOR),
  
  [MFG_HW_TEST_STATUS_LABEL] = LABEL_PAR(0, 2, COL1_W, "MFG HW test status", PAGE_COLOR, PAGE_COLOR),
  [MFG_HW_TEST_STATUS_VAL] = LABEL_PAR(COL2_OFF, 2, COL2_W, "N/A", PAGE_COLOR, PAGE_COLOR),
  
  [RTC_STATUS_LABEL] = LABEL_PAR(0, 4, COL1_W, "RTC status", PAGE_COLOR, PAGE_COLOR),
  [RTC_STATUS_VAL] = LABEL_PAR(COL2_OFF, 4, COL2_W, "N/A", PAGE_COLOR, PAGE_COLOR),
  
  [TIME_LABEL] = LABEL_PAR(0, 6, COL1_W, "Time", PAGE_COLOR, PAGE_COLOR),
  [TIME_HR_VAL] = LINE_PAR(OFF_HR, 6, W_HR, "00", "[0-2][0-9]", BG_COLOR, BG_COLOR, true, 0, true),
  [TIME_COLON_1] = LABEL_PAR(OFF_MN-1, 6, 1, ":", BG_COLOR, BG_COLOR),
  [TIME_MN_VAL] = LINE_PAR(OFF_MN, 6, W_MN, "00", "[0-6][0-9]", BG_COLOR, BG_COLOR, true, 0, true),
  [TIME_COLON_2] = LABEL_PAR(OFF_SC-1, 6, 1, ":", BG_COLOR, BG_COLOR),
  [TIME_SC_VAL] = LINE_PAR(OFF_SC, 6, W_SC, "00", "[0-6][0-9]", BG_COLOR, BG_COLOR, true, 0, true),
  
  [DATE_LABEL] = LABEL_PAR(0, 8, COL1_W, "Date", PAGE_COLOR, PAGE_COLOR),
  [DATE_YEAR_VAL] = LINE_PAR(OFF_YEAR, 8, W_YEAR, "0000", "[0-2][0-9][0-9][0-9]", BG_COLOR, BG_COLOR, true, 0, true),
  [DATE_DASH_1] = LABEL_PAR(OFF_MONTH-1, 8, 1, "-", BG_COLOR, BG_COLOR),
  [DATE_MONTH_VAL] = LINE_PAR(OFF_MONTH, 8, W_MONTH, "00", "[0-1][0-9]", BG_COLOR, BG_COLOR, true, 0, true),
  [DATE_DASH_2] = LABEL_PAR(OFF_DAY-1, 8, 1, "-", BG_COLOR, BG_COLOR),
  [DATE_DAY_VAL] = LINE_PAR(OFF_DAY, 8, W_DAY, "00", "[0-3][0-9]", BG_COLOR, BG_COLOR, true, 0, false),

  [CORE_TEMP_LABEL] = LABEL_PAR(0, 10, COL1_W, "Core temperature", PAGE_COLOR, PAGE_COLOR),
  [CORE_TEMP_VAL] = LABEL_PAR(COL2_OFF, 10, COL2_W, "N/A", PAGE_COLOR, PAGE_COLOR),
  
  [CORE_VOLT_LABEL] = LABEL_PAR(0, 12, COL1_W, "Core voltage", PAGE_COLOR, PAGE_COLOR),
  [CORE_VOLT_VAL] = LABEL_PAR(COL2_OFF, 12, COL2_W, "N/A", PAGE_COLOR, PAGE_COLOR),
  
  [SHRED_LABEL] = LABEL_PAR(0, 14, COL1_W, "SHRED", PAGE_COLOR, PAGE_COLOR),
  [SHRED_VAL] = LABEL_PAR(COL2_OFF, 14, COL2_W, "N/A", PAGE_COLOR, PAGE_COLOR),
  
  [BMC_PROTO_VER_LABEL] = LABEL_PAR(0, 16, COL1_W, "BMC protocol version", PAGE_COLOR, PAGE_COLOR),
  [BMC_PROTO_VER_VAL] = LABEL_PAR(COL2_OFF, 16, COL2_W, "N/A", PAGE_COLOR, PAGE_COLOR),
  
  [RFS_VER_LABEL] = LABEL_PAR(0, 18, COL1_W, "Recovery rfs version", PAGE_COLOR, PAGE_COLOR),
  [RFS_VER_VAL] = LABEL_PAR(COL2_OFF, 18, COL2_W, "N/A", PAGE_COLOR, PAGE_COLOR),
  
  [KERNEL_RELEASE_LABEL] = LABEL_PAR(0, 20, COL1_W, "Kernel release", PAGE_COLOR, PAGE_COLOR),
  [KERNEL_RELEASE_VAL] = LABEL_PAR(COL2_OFF, 20, COL2_W, "N/A", PAGE_COLOR, PAGE_COLOR),

  //right column
  [BOARD_MFG_DATE_LABEL] = LABEL_PAR(COL3_OFF, 0, COL3_W, "Board MFG date", PAGE_COLOR, PAGE_COLOR),
  [BOARD_MFG_DATE_VAL] = LABEL_PAR(COL4_OFF, 0, COL4_W, "N/A", PAGE_COLOR, PAGE_COLOR),

  [BOARD_MANUFACTURER_LABEL] = LABEL_PAR(COL3_OFF, 2, COL3_W, "Board manufacturer", PAGE_COLOR, PAGE_COLOR),
  [BOARD_MANUFACTURER_VAL] = LABEL_PAR(COL4_OFF, 2, COL4_W, "N/A", PAGE_COLOR, PAGE_COLOR),

  [BOARD_PRODUCT_NAME_LABEL] = LABEL_PAR(COL3_OFF, 4, COL3_W, "Board product name", PAGE_COLOR, PAGE_COLOR),
  [BOARD_PRODUCT_NAME_VAL] = LABEL_PAR(COL4_OFF, 4, COL4_W, "N/A", PAGE_COLOR, PAGE_COLOR),

  [BOARD_SERIAL_LABEL] = LABEL_PAR(COL3_OFF, 6, COL3_W, "Board serial number", PAGE_COLOR, PAGE_COLOR),
  [BOARD_SERIAL_VAL] = LABEL_PAR(COL4_OFF, 6, COL4_W, "N/A", PAGE_COLOR, PAGE_COLOR),

  [BOARD_PART_NUMBER_LABEL] = LABEL_PAR(COL3_OFF, 8, COL3_W, "Board part number", PAGE_COLOR, PAGE_COLOR),
  [BOARD_PART_NUMBER_VAL] = LABEL_PAR(COL4_OFF, 8, COL4_W, "N/A", PAGE_COLOR, PAGE_COLOR),

  [BOARD_FRU_ID_LABEL] = LABEL_PAR(COL3_OFF, 10, COL3_W, "Board FRU file ID", PAGE_COLOR, PAGE_COLOR),
  [BOARD_FRU_ID_VAL] = LABEL_PAR(COL4_OFF, 10, COL4_W, "N/A", PAGE_COLOR, PAGE_COLOR),

  [PRODUCT_MANUFACTURER_LABEL] = LABEL_PAR(COL3_OFF, 12, COL3_W, "Product manufacturer", PAGE_COLOR, PAGE_COLOR),
  [PRODUCT_MANUFACTURER_VAL] = LABEL_PAR(COL4_OFF, 12, COL4_W, "N/A", PAGE_COLOR, PAGE_COLOR),

  [PRODUCT_PART_MODEL_NUMBER_LABEL] = LABEL_PAR(COL3_OFF, 14, COL3_W, "Product part/model number", PAGE_COLOR, PAGE_COLOR),
  [PRODUCT_PART_MODEL_NUMBER_VAL] = LABEL_PAR(COL4_OFF, 14, COL4_W, "N/A", PAGE_COLOR, PAGE_COLOR),

  [PRODUCT_VERSION_LABEL] = LABEL_PAR(COL3_OFF, 16, COL3_W, "Product version", PAGE_COLOR, PAGE_COLOR),
  [PRODUCT_VERSION_VAL] = LABEL_PAR(COL4_OFF, 16, COL4_W, "N/A", PAGE_COLOR, PAGE_COLOR),

};

enum BMC_BOOTREASON {
  BR_UNKNOWN=0,
  BR_NORMAL,
  BR_BOOTCONF_FAIL,
  BR_NOT_TESTED,
  BR_TEST_FAIL
};

static struct {
  struct window_params wp;
  WINDOW *w;
  uint32_t shred;
  char shred_label[20];
  char temp_label[20];
  char voltage_label[20];
  char mfg_date_label[20];
  char bmc_version_label[20];
  char rfs_version[21];
  char kernel_release[21];
  char year_val[5];
  char month_val[3];
  char day_val[3];
  char hour_val[3];
  char min_val[3];
  char sec_val[3];
  FIELD *fields[N_FIELDS+1];
  FORM *form;
  bool edit_mode;
  bool selected;
  FIELD *first_active;
  FIELD *last_active;
} main_page;


uint32_t read_shred(void) {
#ifdef REAL_DEVICES
  uint32_t val = 0;
  int ret = 0;
  uint32_t shred_val = 0;
  int i = 0;
  char buf[64] = {0};
  glob_t pglob;
  FILE *fchip;
  int gpio_base;

  if (glob(GPIOCHIP_GLOB, 0, NULL, &pglob)) {
    ferr("Failed to glob " GPIOCHIP_GLOB "\n");
    return 0;
  }
  fchip = fopen(pglob.gl_pathv[0], "r");
  if (!fchip) {
    ferr("Failed to open %s\n", pglob.gl_pathv[0]);
    globfree(&pglob);
    return 0;
  }
  globfree(&pglob);
  fscanf(fchip, "%d", &gpio_base);
  for (i=0;i<SHRED_NGPIO;i++) {
    flog("Opening /sys/class/gpio/export\n");
    FILE *export = fopen("/sys/class/gpio/export", "w");
    if (export == NULL) {
      ferr("Failed to open /sys/class/gpio/export\n");
      return val;
    }
    ret = sprintf(buf, "%i", i+gpio_base);
    flog("Exporting pin %s[len: %i]\n", buf, ret);
    fwrite(buf, sizeof(char), ret, export);
    fclose(export);
    sprintf(buf, "/sys/class/gpio/gpio%i/value", i+gpio_base);
    flog("Opening %s\n", buf);
    FILE *gpio = fopen(buf, "r");
    if (gpio == NULL) {
      ferr("Failed to open %s\n", buf);
      fclose(export);
      return shred_val;
    }
    ret = fscanf(gpio, "%i", &val);
    flog("gpio[%i] read returned %i, value: %i\n", i+gpio_base, ret, val);
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
  glob_t pglob;
  pages_params.bmc_version[0] = 0;
  pages_params.bmc_version[1] = 0;
  pages_params.bmc_version[2] = 0;
  pages_params.boot_reason[0] = 0;
  pages_params.boot_reason[1] = 0;
  FILE *version;

  if (glob(BMC_VERSION_GLOB, 0, NULL, &pglob)) {
    ferr("Failed to glob " BMC_VERSION_GLOB "\n");
    return 0;
  }
  version = fopen(pglob.gl_pathv[0], "r");

  if (version == NULL) {
    ferr("Failed to open %s\n", pglob.gl_pathv[0]);
    globfree(&pglob);
    return;
  }
  globfree(&pglob);
  ret = fscanf(version, "%i.%i.%i", &pages_params.bmc_version[0], &pages_params.bmc_version[1], &pages_params.bmc_version[2]);
  fclose(version);
  flog("BMC version read returned %i, value: %i.%i.%i\n", ret, pages_params.bmc_version[0], pages_params.bmc_version[1], pages_params.bmc_version[2]);
  if (pages_params.bmc_version[0] >= 2) {
    FILE *bootreason;

    if (glob(BOOTREASON_GLOB, 0, NULL, &pglob)) {
      ferr("Failed to glob " BOOTREASON_GLOB "\n");
      return 0;
    }
    bootreason = fopen(pglob.gl_pathv[0], "r");

    if (bootreason == NULL) {
      ferr("Failed to open %s\n", pglob.gl_pathv[0]);
      globfree(&pglob);
      return;
    }
    globfree(&pglob);
    ret = fscanf(bootreason, "%i", &reason);
    fclose(bootreason);
    pages_params.boot_reason[0] = reason & 0xff;
    pages_params.boot_reason[1] = (reason>>8) & 0xff;
    flog("Bootreason: %i/%i\n", pages_params.boot_reason[0], pages_params.boot_reason[1]);
  }
}

int
read_pvt(void) {
  FILE * pvt_temp_file = fopen(SYS_TEMP_PATH, "r");
  if (pvt_temp_file == NULL) {
    ferr("Failed to open %s\n", SYS_TEMP_PATH);
    sprintf(main_page.temp_label, "%.3f", 0.0f);
    return -1;
  }
  int pvt_temp = 0;
  fscanf(pvt_temp_file, "%i", &pvt_temp);
  fclose(pvt_temp_file);
  sprintf(main_page.temp_label, "%.3f Deg.C", ((float)pvt_temp)/1000.0f);

  FILE * pvt_core_voltage_file = fopen(SYS_COREV_PATH, "r");
  if (pvt_core_voltage_file == NULL) {
    ferr("Failed to open %s\n", SYS_COREV_PATH);
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
  int i = 0;
  main_page.edit_mode = false;
  main_page.selected = false;
  main_page.first_active = NULL;
  main_page.last_active = NULL;
  read_bmc_version();
  main_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(main_page.wp.w, 0, 0);
  wbkgd(main_page.wp.w, PAGE_COLOR);

  main_page.wp.p = new_panel(main_page.wp.w);

  getmaxyx(main_page.wp.w, height, width);
  (void)height;

  main_page.shred = read_shred();
  sprintf(main_page.shred_label, "%i", main_page.shred);
  flog("shred: 0x%08x\n", main_page.shred);
  t = time(NULL);
  tm = *localtime(&t);
  read_pvt();

  for (i=0; i<N_FIELDS; i++) {
    main_page.fields[i] = mk_field(&fp[i]);
    if (fp[i].ft != FT_LABEL) {
      if (main_page.first_active == NULL) {
        main_page.first_active = main_page.fields[i];
      }
      main_page.last_active = main_page.fields[i];
    }
  }
  main_page.fields[NULL_VAL] = NULL;
  
  main_page.form = new_form(main_page.fields);
  form_opts_off(main_page.form, O_NL_OVERLOAD);
  form_opts_off(main_page.form, O_BS_OVERLOAD);
  scale_form(main_page.form, &height, &width);
  set_form_win(main_page.form, main_page.wp.w);
  main_page.w = derwin(main_page.wp.w, height, width, 2, 2);
  set_form_sub(main_page.form, main_page.w);

  post_form(main_page.form);

  redrawwin(main_page.wp.w);

  if (pages_params.boot_reason[0] == BR_NORMAL) {
    set_field_buffer(main_page.fields[BOOT_STATUS_VAL], 0, "NORMAL");
    set_field_fore(main_page.fields[BOOT_STATUS_VAL], GREEN_COLOR);
  } else if (pages_params.boot_reason[0] == BR_BOOTCONF_FAIL) {
    set_field_buffer(main_page.fields[BOOT_STATUS_VAL], 0, "BOOTCONF FAIL");
    set_field_fore(main_page.fields[BOOT_STATUS_VAL], RED_COLOR);
  } else if (pages_params.boot_reason[0] == BR_TEST_FAIL) {
    set_field_buffer(main_page.fields[BOOT_STATUS_VAL], 0, "HW TEST FAIL");
    set_field_fore(main_page.fields[BOOT_STATUS_VAL], RED_COLOR);
  } else if (pages_params.boot_reason[0] == BR_NOT_TESTED) {
    set_field_buffer(main_page.fields[BOOT_STATUS_VAL], 0, "NOT TESTED");
    set_field_fore(main_page.fields[BOOT_STATUS_VAL], RED_COLOR);
  } else {
    set_field_buffer(main_page.fields[BOOT_STATUS_VAL], 0, "UNKNOWN");
    set_field_fore(main_page.fields[BOOT_STATUS_VAL], RED_COLOR);
  }

  flog("Test ok: %i\n", fru.test_ok);
  if (fru.test_ok==1) {
    set_field_buffer(main_page.fields[MFG_HW_TEST_STATUS_VAL], 0, "PASSED");
    set_field_fore(main_page.fields[MFG_HW_TEST_STATUS_VAL], GREEN_COLOR);
  } else if (fru.test_ok==2) {
    set_field_buffer(main_page.fields[MFG_HW_TEST_STATUS_VAL], 0, "FAILED");
    set_field_fore(main_page.fields[MFG_HW_TEST_STATUS_VAL], RED_COLOR);
  } else {
    set_field_buffer(main_page.fields[MFG_HW_TEST_STATUS_VAL], 0, "UNKNOWN");
    set_field_fore(main_page.fields[MFG_HW_TEST_STATUS_VAL], PAGE_COLOR);
  }

  if ((tm.tm_year + 1900)==1970) {
    set_field_buffer(main_page.fields[RTC_STATUS_VAL], 0, "FAIL");
    set_field_fore(main_page.fields[RTC_STATUS_VAL], RED_COLOR);
  } else {
    set_field_buffer(main_page.fields[RTC_STATUS_VAL], 0, "OK");
    set_field_fore(main_page.fields[RTC_STATUS_VAL], GREEN_COLOR);
  }
  
  sprintf(main_page.hour_val, "%02i", tm.tm_hour);
  sprintf(main_page.min_val, "%02i", tm.tm_min);
  sprintf(main_page.sec_val, "%02i", tm.tm_sec);
  set_field_buffer(main_page.fields[TIME_HR_VAL], 0, main_page.hour_val);
  set_field_buffer(main_page.fields[TIME_MN_VAL], 0, main_page.min_val);
  set_field_buffer(main_page.fields[TIME_SC_VAL], 0, main_page.sec_val);

  sprintf(main_page.year_val, "%04i", tm.tm_year + 1900);
  sprintf(main_page.month_val, "%02i", tm.tm_mon + 1);
  sprintf(main_page.day_val, "%02i", tm.tm_mday);
  set_field_buffer(main_page.fields[DATE_YEAR_VAL], 0, main_page.year_val);
  set_field_buffer(main_page.fields[DATE_MONTH_VAL], 0, main_page.month_val);
  set_field_buffer(main_page.fields[DATE_DAY_VAL], 0, main_page.day_val);

  set_field_buffer(main_page.fields[CORE_TEMP_VAL], 0, main_page.temp_label);
  set_field_buffer(main_page.fields[CORE_VOLT_VAL], 0, main_page.voltage_label);
  set_field_buffer(main_page.fields[SHRED_VAL], 0, main_page.shred_label);

  sprintf(main_page.bmc_version_label, "%i.%i.%i", pages_params.bmc_version[0], pages_params.bmc_version[1], pages_params.bmc_version[2]);
  set_field_buffer(main_page.fields[BMC_PROTO_VER_VAL], 0, main_page.bmc_version_label);

  FILE *rfs = fopen("/etc/recovery-version", "r");
  if (rfs != NULL) {
    fread(main_page.rfs_version, sizeof(char), 20, rfs);
    fclose(rfs);
    set_field_fore(main_page.fields[RFS_VER_VAL], PAGE_COLOR);
  } else {
    sprintf(main_page.rfs_version, "UNKNOWN");
    set_field_fore(main_page.fields[RFS_VER_VAL], RED_COLOR);
  }
  set_field_buffer(main_page.fields[RFS_VER_VAL], 0, main_page.rfs_version);

  FILE *kernel_release = fopen(PROC_OSRELEASE_PATH, "r");
  if (kernel_release != NULL) {
    fread(main_page.kernel_release, sizeof(char), 20, kernel_release);
    fclose(kernel_release);
    set_field_fore(main_page.fields[KERNEL_RELEASE_VAL], PAGE_COLOR);
  } else {
    sprintf(main_page.kernel_release, "UNKNOWN");
    set_field_fore(main_page.fields[KERNEL_RELEASE_VAL], RED_COLOR);
  }
  set_field_buffer(main_page.fields[KERNEL_RELEASE_VAL], 0, main_page.kernel_release);
  
// right column
  tm.tm_year = 1996-1900;
  tm.tm_mon = 0;           // Month, 0 - jan
  tm.tm_mday = 1;          // Day of the month
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
  t = mktime(&tm);
  int offset = (fru.mfg_date[2] | (fru.mfg_date[1]<<8) | (fru.mfg_date[0]<<16));
  flog("offset: 0x%08x\n", offset);
  offset*=60;
  t += offset;
  tm = *localtime(&t);
  strftime(main_page.mfg_date_label, 20, "%Y-%m-%d", &tm);
  set_field_buffer(main_page.fields[BOARD_MFG_DATE_VAL], 0, main_page.mfg_date_label);

  set_field_buffer(main_page.fields[BOARD_MANUFACTURER_VAL], 0, (char *)fru.val_mfg_name);

  set_field_buffer(main_page.fields[BOARD_PRODUCT_NAME_VAL], 0, (char *)fru.val_product_name);

  set_field_buffer(main_page.fields[BOARD_SERIAL_VAL], 0, (char *)fru.val_serial_number);

  set_field_buffer(main_page.fields[BOARD_PART_NUMBER_VAL], 0, (char *)fru.val_part_number);

  set_field_buffer(main_page.fields[BOARD_FRU_ID_VAL], 0, (char *)fru.val_fru_id);

  set_field_buffer(main_page.fields[PRODUCT_MANUFACTURER_VAL], 0, (char *)fru.val_p_product_mfg);

  set_field_buffer(main_page.fields[PRODUCT_PART_MODEL_NUMBER_VAL], 0, (char *)fru.val_p_part_model_number);

  set_field_buffer(main_page.fields[PRODUCT_VERSION_VAL], 0, (char *)fru.val_p_product_version);
}

void
main_update_datetime(void) {
  if (!main_page.edit_mode) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(main_page.year_val, "%04i", tm.tm_year + 1900);
    sprintf(main_page.month_val, "%02i", tm.tm_mon + 1);
    sprintf(main_page.day_val, "%02i", tm.tm_mday);
    set_field_buffer(main_page.fields[DATE_YEAR_VAL], 0, main_page.year_val);
    set_field_buffer(main_page.fields[DATE_MONTH_VAL], 0, main_page.month_val);
    set_field_buffer(main_page.fields[DATE_DAY_VAL], 0, main_page.day_val);
    
    sprintf(main_page.hour_val, "%02i", tm.tm_hour);
    sprintf(main_page.min_val, "%02i", tm.tm_min);
    sprintf(main_page.sec_val, "%02i", tm.tm_sec);
    set_field_buffer(main_page.fields[TIME_HR_VAL], 0, main_page.hour_val);
    set_field_buffer(main_page.fields[TIME_MN_VAL], 0, main_page.min_val);
    set_field_buffer(main_page.fields[TIME_SC_VAL], 0, main_page.sec_val);
  }
}

int
main_validate_datetime(struct tm *tm) {
  int y, m, d;
  int hr, mn, sc;
  char *py = field_buffer(main_page.fields[DATE_YEAR_VAL], 0);
  char *pm = field_buffer(main_page.fields[DATE_MONTH_VAL], 0);
  char *pd = field_buffer(main_page.fields[DATE_DAY_VAL], 0);
  char *phr = field_buffer(main_page.fields[TIME_HR_VAL], 0);
  char *pmn = field_buffer(main_page.fields[TIME_MN_VAL], 0);
  char *psc = field_buffer(main_page.fields[TIME_SC_VAL], 0);
  y = strtol(py, NULL, 10);
  m = strtol(pm, NULL, 10);
  d = strtol(pd, NULL, 10);
  hr = strtol(phr, NULL, 10);
  mn = strtol(pmn, NULL, 10);
  sc = strtol(psc, NULL, 10);
  flog("sc: %s, mn: %s, hr: %s, y: %s, m: %s, d: %s\nparsed sc: %i, mn: %i, hr: %i, y: %i, m: %i, d: %i\n", psc, pmn, phr, py, pm, pd, sc, mn, hr, y, m, d);
  if (sc>=0 && sc<=60) {
    if (tm != NULL) {
      tm->tm_sec = sc;
    }
    set_field_fore(main_page.fields[TIME_SC_VAL], BG_COLOR);
    set_field_back(main_page.fields[TIME_SC_VAL], BG_COLOR);
  } else {
    fwarn("Bad sec value: %i not in 0-60 range\n", sc);
    set_field_fore(main_page.fields[TIME_SC_VAL], RED_EDIT_COLOR);
    set_field_back(main_page.fields[TIME_SC_VAL], RED_EDIT_COLOR);
    return -1;
  }
  if (mn>=0 && mn<=59) {
    if (tm != NULL) {
      tm->tm_min = mn;
    }
    set_field_fore(main_page.fields[TIME_MN_VAL], BG_COLOR);
    set_field_back(main_page.fields[TIME_MN_VAL], BG_COLOR);
  } else {
    fwarn("Bad min value: %i not in 0-59 range\n", mn);
    set_field_fore(main_page.fields[TIME_MN_VAL], RED_EDIT_COLOR);
    set_field_back(main_page.fields[TIME_MN_VAL], RED_EDIT_COLOR);
    return -2;
  }
  if (hr>=0 && hr<=23) {
    if (tm != NULL) {
      tm->tm_hour = hr;
    }
    set_field_fore(main_page.fields[TIME_HR_VAL], BG_COLOR);
    set_field_back(main_page.fields[TIME_HR_VAL], BG_COLOR);
  } else {
    fwarn("Bad hour value: %i not in 0-23 range\n", hr);
    set_field_fore(main_page.fields[TIME_HR_VAL], RED_EDIT_COLOR);
    set_field_back(main_page.fields[TIME_HR_VAL], RED_EDIT_COLOR);
    return -3;
  }
  if (d>=1 && d<=31) {
    if (tm != NULL) {
      tm->tm_mday = d;
    }
    set_field_fore(main_page.fields[DATE_DAY_VAL], BG_COLOR);
    set_field_back(main_page.fields[DATE_DAY_VAL], BG_COLOR);
  } else {
    fwarn("Bad day value: %i not in 1-31 range\n", d);
    set_field_fore(main_page.fields[DATE_DAY_VAL], RED_EDIT_COLOR);
    set_field_back(main_page.fields[DATE_DAY_VAL], RED_EDIT_COLOR);
    return -4;
  }
  if (m>=1 && m<=12) {
    if (tm != NULL) {
      tm->tm_mon = m-1;
    }
    set_field_fore(main_page.fields[DATE_MONTH_VAL], BG_COLOR);
    set_field_back(main_page.fields[DATE_MONTH_VAL], BG_COLOR);
  } else {
    fwarn("Bad month value: %i not in 1-12 range\n", m);
    set_field_fore(main_page.fields[DATE_MONTH_VAL], RED_EDIT_COLOR);
    set_field_back(main_page.fields[DATE_MONTH_VAL], RED_EDIT_COLOR);
    return -5;
  }
  if (y>=1970) {
    if (tm != NULL) {
      tm->tm_year = y-1900;
    }
    set_field_fore(main_page.fields[DATE_YEAR_VAL], BG_COLOR);
    set_field_back(main_page.fields[DATE_YEAR_VAL], BG_COLOR);
  } else {
    fwarn("Bad year value: %i not >=1970\n", y);
    set_field_fore(main_page.fields[DATE_YEAR_VAL], RED_EDIT_COLOR);
    set_field_back(main_page.fields[DATE_YEAR_VAL], RED_EDIT_COLOR);
    return -6;
  }
  return 0;
}

int
main_save_datetime(void) {
  time_t t;
  struct tm tm;
  struct rtc_time rtc;
  int fd;
  if (main_validate_datetime(&tm)!=0) {
    return -1;
  }

  t = mktime(&tm);
  stime(&t);
  
  rtc.tm_sec = tm.tm_sec;
  rtc.tm_min = tm.tm_min;
  rtc.tm_hour = tm.tm_hour;
  rtc.tm_mday = tm.tm_mday;
  rtc.tm_mon = tm.tm_mon;
  rtc.tm_year = tm.tm_year;
  fd = open("/dev/rtc0", O_RDONLY);
  ioctl(fd, RTC_SET_TIME, &rtc);
  close(fd);
  return 0;
}

int
main_page_process(int ch) {
  static uint64_t last_t = 0;
  uint64_t cur_t = time(NULL);
  if (!main_page.wp.hidden) {
    FIELD *f = current_field(main_page.form);
    main_update_datetime();
    if ((cur_t-last_t)>1) {
      last_t = cur_t;
      read_pvt();
      set_field_buffer(main_page.fields[CORE_TEMP_VAL], 0, main_page.temp_label);
      set_field_buffer(main_page.fields[CORE_VOLT_VAL], 0, main_page.voltage_label);
    }
    if (!main_page.selected) {
      curs_set(0);
      switch (ch) {
      case KEY_DOWN:
        main_page.selected = true;
        pages_params.use_arrows = false;
        main_page.edit_mode = false;
        break;
      }
    } else {
      curs_set(1);
      switch (ch) {
      case KEY_DOWN:
        if (main_page.edit_mode) {
          field_par_unset_line_bg(fp, main_page.fields, N_FIELDS);
          main_save_datetime();
          main_page.edit_mode = false;
        }
        if (f != main_page.last_active) {
          form_driver(main_page.form, REQ_NEXT_FIELD);
        }
        break;
      case KEY_UP:
        if (main_page.edit_mode) {
          field_par_unset_line_bg(fp, main_page.fields, N_FIELDS);
          main_save_datetime();
          main_page.edit_mode = false;
        }
        if (f != main_page.first_active) {
          form_driver(main_page.form, REQ_PREV_FIELD);
        } else {
          main_page.selected = false;
          pages_params.use_arrows = true;
        }
        break;
      case RKEY_ENTER:
        break;
      case RKEY_ESC:
        break;
      case KEY_LEFT:
        main_page.edit_mode = true;
        field_par_set_line_bg(fp, f, main_page.fields, N_FIELDS);
        form_driver(main_page.form, REQ_PREV_CHAR);
        break;
      case KEY_RIGHT:
        main_page.edit_mode = true;
        field_par_set_line_bg(fp, f, main_page.fields, N_FIELDS);
        form_driver(main_page.form, REQ_NEXT_CHAR);
        break;
      case KEY_BACKSPACE:
      case 127:
        main_page.edit_mode = true;
        field_par_set_line_bg(fp, f, main_page.fields, N_FIELDS);
        form_driver(main_page.form, REQ_DEL_PREV);
        break;
      case KEY_DC:
        main_page.edit_mode = true;
        field_par_set_line_bg(fp, f, main_page.fields, N_FIELDS);
        form_driver(main_page.form, REQ_DEL_CHAR);
        break;
      case -1:
        break;
      default:
        main_page.edit_mode = true;
        field_par_set_line_bg(fp, f, main_page.fields, N_FIELDS);
        form_driver(main_page.form, ch);
        break;
      }
    }
    wnoutrefresh(main_page.wp.w);
  }
  return 0;
}

void
deinit_main_page(void) {
  int i;
  unpost_form(main_page.form);
	free_form(main_page.form);
  for (i=0; i<N_FIELDS; i++)
    free_field(main_page.fields[i]);

  delwin(main_page.wp.w);
}

struct window_params *
get_main_page_wp(void) {
  return &main_page.wp;
}
