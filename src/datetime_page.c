#include <linux/rtc.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>
#include <form.h>

#include "common.h"
#include "pages.h"
#include "main_page.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "DATETIME_PAGE"

#define LABEL_WIDTH 25
//                     |                 |
#define OFF_YEAR LABEL_WIDTH
#define W_YEAR 4
#define OFF_MONTH (LABEL_WIDTH+W_YEAR+1)
#define W_MONTH 2
#define OFF_DAY (OFF_MONTH+W_MONTH+1)
#define W_DAY 2

#define OFF_HR LABEL_WIDTH
#define W_HR 2
#define OFF_MN (LABEL_WIDTH+W_HR+1)
#define W_MN 2
#define OFF_SC (OFF_MN+W_MN+1)
#define W_SC 2

enum mac_fields {
  DATE_LABEL = 0,
  DATE_YEAR_EDIT,
  DATE_MONTH_EDIT,
  DATE_DAY_EDIT,
  TIME_LABEL,
  TIME_HR_EDIT,
  TIME_MN_EDIT,
  TIME_SC_EDIT,  
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct {
  struct window_params wp;
  WINDOW *sw;
  char year_val[5];
  char month_val[2];
  char day_val[2];
  char hour_val[2];
  char min_val[2];
  char sec_val[2];
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
  bool update_time;
} dt_page;

void
init_dt_page(void) {
  int width, height;
  int cy = 0;
  time_t t;
  struct tm tm;
  dt_page.update_time = true;

  dt_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);
  box(dt_page.wp.w, 0, 0);
  wbkgd(dt_page.wp.w, PAGE_COLOR);

  dt_page.wp.p = new_panel(dt_page.wp.w);

  getmaxyx(dt_page.wp.w, height, width);
  (void)height;

  t = time(NULL);
  tm = *localtime(&t);

  sprintf(dt_page.year_val, "%04i", tm.tm_year + 1900);
  sprintf(dt_page.month_val, "%02i", tm.tm_mon + 1);
  sprintf(dt_page.day_val, "%02i", tm.tm_mday);
  dt_page.fields[DATE_LABEL] = mk_label(LABEL_WIDTH, 0, cy, "Date", PAGE_COLOR);
  dt_page.fields[DATE_YEAR_EDIT] = mk_editable_field_regex(W_YEAR, OFF_YEAR, cy, dt_page.year_val, "[12][0-9][0-9][0-9]", BG_COLOR);
  dt_page.fields[DATE_MONTH_EDIT] = mk_editable_field_regex(W_MONTH, OFF_MONTH, cy, dt_page.month_val, "[01][0-9]", BG_COLOR);
  dt_page.fields[DATE_DAY_EDIT] = mk_editable_field_regex(W_DAY, OFF_DAY, cy, dt_page.day_val, "[0123][0-9]", BG_COLOR);
  cy+=2;

  sprintf(dt_page.hour_val, "%02i", tm.tm_hour);
  sprintf(dt_page.min_val, "%02i", tm.tm_min);
  sprintf(dt_page.sec_val, "%02i", tm.tm_sec);
  dt_page.fields[TIME_LABEL] = mk_label(LABEL_WIDTH, 0, cy, "UTC Time", PAGE_COLOR);
  dt_page.fields[TIME_HR_EDIT] = mk_editable_field_regex(W_HR, OFF_HR, cy, dt_page.hour_val, "[0-9][0-9]", BG_COLOR);
  dt_page.fields[TIME_MN_EDIT] = mk_editable_field_regex(W_MN, OFF_MN, cy, dt_page.min_val, "[0-9][0-9]", BG_COLOR);
  dt_page.fields[TIME_SC_EDIT] = mk_editable_field_regex(W_SC, OFF_SC, cy, dt_page.sec_val, "[0-9][0-9]", BG_COLOR);

  dt_page.fields[NULL_VAL] = NULL;
  
  dt_page.f = new_form(dt_page.fields);
  scale_form(dt_page.f, &height, &width);
  set_form_win(dt_page.f, dt_page.wp.w);
  dt_page.sw = derwin(dt_page.wp.w, height, LABEL_WIDTH*2, 2, 2);
  set_form_sub(dt_page.f, dt_page.sw);

  post_form(dt_page.f);

  redrawwin(dt_page.wp.w);
}

void
dt_update_datetime(void) {
  if (dt_page.update_time) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(dt_page.year_val, "%04i", tm.tm_year + 1900);
    sprintf(dt_page.month_val, "%02i", tm.tm_mon + 1);
    sprintf(dt_page.day_val, "%02i", tm.tm_mday);
    set_field_buffer(dt_page.fields[DATE_YEAR_EDIT], 0, dt_page.year_val);
    set_field_buffer(dt_page.fields[DATE_MONTH_EDIT], 0, dt_page.month_val);
    set_field_buffer(dt_page.fields[DATE_DAY_EDIT], 0, dt_page.day_val);
    
    sprintf(dt_page.hour_val, "%02i", tm.tm_hour);
    sprintf(dt_page.min_val, "%02i", tm.tm_min);
    sprintf(dt_page.sec_val, "%02i", tm.tm_sec);
    set_field_buffer(dt_page.fields[TIME_HR_EDIT], 0, dt_page.hour_val);
    set_field_buffer(dt_page.fields[TIME_MN_EDIT], 0, dt_page.min_val);
    set_field_buffer(dt_page.fields[TIME_SC_EDIT], 0, dt_page.sec_val);
  }
}

int
dt_save_datetime(void) {
  int y, m, d;
  int hr, mn, sc;
  time_t t;
  struct tm tm;
  struct rtc_time rtc;
  int fd;
  
  sscanf(field_buffer(dt_page.fields[DATE_YEAR_EDIT], 0), "%i", &y);
  sscanf(field_buffer(dt_page.fields[DATE_MONTH_EDIT], 0), "%i", &m);
  sscanf(field_buffer(dt_page.fields[DATE_DAY_EDIT], 0), "%i", &d);
  sscanf(field_buffer(dt_page.fields[TIME_HR_EDIT], 0), "%i", &hr);
  sscanf(field_buffer(dt_page.fields[TIME_MN_EDIT], 0), "%i", &mn);
  sscanf(field_buffer(dt_page.fields[TIME_SC_EDIT], 0), "%i", &sc);
  if (sc>=0 && sc<=60) {
    tm.tm_sec = sc;
    set_field_fore(dt_page.fields[TIME_SC_EDIT], BG_COLOR);
    set_field_back(dt_page.fields[TIME_SC_EDIT], BG_COLOR);
  } else {
    set_field_fore(dt_page.fields[TIME_SC_EDIT], RED_EDIT_COLOR);
    set_field_back(dt_page.fields[TIME_SC_EDIT], RED_EDIT_COLOR);
    return -1;
  }
  if (mn>=0 && mn<=59) {
    tm.tm_min = mn;
    set_field_fore(dt_page.fields[TIME_MN_EDIT], BG_COLOR);
    set_field_back(dt_page.fields[TIME_MN_EDIT], BG_COLOR);
  } else {
    set_field_fore(dt_page.fields[TIME_MN_EDIT], RED_EDIT_COLOR);
    set_field_back(dt_page.fields[TIME_MN_EDIT], RED_EDIT_COLOR);
    return -2;
  }
  if (hr>=0 && hr<=23) {
    tm.tm_hour = hr;
    set_field_fore(dt_page.fields[TIME_HR_EDIT], BG_COLOR);
    set_field_back(dt_page.fields[TIME_HR_EDIT], BG_COLOR);
  } else {
    set_field_fore(dt_page.fields[TIME_HR_EDIT], RED_EDIT_COLOR);
    set_field_back(dt_page.fields[TIME_HR_EDIT], RED_EDIT_COLOR);
    return -3;
  }
  if (d>=1 && d<=31) {
    tm.tm_mday = d;
    set_field_fore(dt_page.fields[DATE_DAY_EDIT], BG_COLOR);
    set_field_back(dt_page.fields[DATE_DAY_EDIT], BG_COLOR);
  } else {
    set_field_fore(dt_page.fields[DATE_DAY_EDIT], RED_EDIT_COLOR);
    set_field_back(dt_page.fields[DATE_DAY_EDIT], RED_EDIT_COLOR);
    return -4;
  }
  if (m>=1 && m<=12) {
    tm.tm_mon = m-1;
    set_field_fore(dt_page.fields[DATE_MONTH_EDIT], BG_COLOR);
    set_field_back(dt_page.fields[DATE_MONTH_EDIT], BG_COLOR);
  } else {
    set_field_fore(dt_page.fields[DATE_MONTH_EDIT], RED_EDIT_COLOR);
    set_field_back(dt_page.fields[DATE_MONTH_EDIT], RED_EDIT_COLOR);
    return -5;
  }
  if (y>=1970) {
    tm.tm_year = y-1900;
    set_field_fore(dt_page.fields[DATE_YEAR_EDIT], BG_COLOR);
    set_field_back(dt_page.fields[DATE_YEAR_EDIT], BG_COLOR);
  } else {
    set_field_fore(dt_page.fields[DATE_YEAR_EDIT], RED_EDIT_COLOR);
    set_field_back(dt_page.fields[DATE_YEAR_EDIT], RED_EDIT_COLOR);
    return -6;
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
  log("Save time: %04i-%02i-%02i %02i:%02i:%02i\n", y, m, d, hr, mn, sc);
  return 0;
}

int
dt_page_process(int ch) {
  int ret;
  if (!dt_page.wp.hidden) {
    dt_update_datetime();
    curs_set(1);
    switch (ch) {
    case KEY_DOWN:
      form_driver(dt_page.f, REQ_NEXT_FIELD);
      break;
    case KEY_UP:
      form_driver(dt_page.f, REQ_PREV_FIELD);
      break;
		case KEY_BACKSPACE:
		case 127:
      form_driver(dt_page.f, REQ_DEL_PREV);
      break;
    case KEY_DC:
      form_driver(dt_page.f, REQ_DEL_CHAR);
			break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      dt_page.update_time = false;
      form_driver(dt_page.f, ch);
      break;
    case RKEY_ENTER://KEY_ENTER:
      ret = dt_save_datetime();
      if (ret==0) {
        dt_page.update_time = true;
      }
      //f = current_field(dt_page.f);
      //if (f == dt_page.fields[DHCP_LABEL]) {
      //}
      break;
    case RKEY_ESC:
      dt_page.update_time = true;
      break;
    default:
      form_driver(dt_page.f, ch);
      break;
    }
    wnoutrefresh(dt_page.wp.w);
  }
  return 0;
}

void
deinit_dt_page(void) {
  int i;
  unpost_form(dt_page.f);
	free_form(dt_page.f);
  for (i=0; i<N_FIELDS; i++)
    free_field(dt_page.fields[i]);
  delwin(dt_page.wp.w);
}

struct window_params *
get_dt_page_wp(void) {
  return &dt_page.wp;
}
