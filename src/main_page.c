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

#define LABEL_WIDTH 30

enum fields {
  TIME_VAL=0,
  DATE_VAL,
  SHRED_VAL,
  TESTOK_VAL,
  BRD_MFG_DATE_VAL,
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
  
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct {
  struct window_params wp;
  WINDOW *w;
  WINDOW *sw;
  uint32_t shred;
  char shred_label[20];
  char time_label[20];
  char mfg_date_label[20];
  char date_label[20];
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
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
mk_label(int w, int x, int y, char *string, chtype c) {
  FIELD *f = new_field(1, w, y, x, 0, 0);
  field_opts_off(f, O_EDIT);
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
mk_editable_field_regex(int w, int x, int y, char *string, char *regex, chtype c) {
  FIELD *f = new_field(1, w, y, x, 0, 0);
  set_max_field(f, w);
  set_field_type(f, TYPE_REGEXP, regex);
  set_field_opts(f, O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE | O_AUTOSKIP | O_BLANK);
  //field_opts_off(f, O_AUTOSKIP);
  set_field_buffer(f, 0, string);
  set_field_fore(f, c);
  set_field_back(f, c);
  return f;
}

void
init_main_page(void) {
  int width, height;
  time_t t;
  struct tm tm;
  int y = 0;
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

  mvwaddstr(main_page.wp.w, y+2, 2, "Time");
  sprintf(main_page.time_label, "%02i:%02i:%02i UTC", tm.tm_hour, tm.tm_min, tm.tm_sec);
	main_page.fields[TIME_VAL] = mk_label(LABEL_WIDTH, 0, y, main_page.time_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Date");
  sprintf(main_page.date_label, "%02i-%02i-%04i", tm.tm_mday, tm.tm_mon+1, tm.tm_year + 1900);
	main_page.fields[DATE_VAL] = mk_label(LABEL_WIDTH, 0, y, main_page.date_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "SHRED");
  sprintf(main_page.shred_label, "%02x", main_page.shred);
	main_page.fields[SHRED_VAL] = mk_label(LABEL_WIDTH, 0, y, main_page.shred_label, PAGE_COLOR);
  y+=2;

  log("Test ok: %i\n", fru.test_ok);
  mvwaddstr(main_page.wp.w, y+2, 2, "MFG HW test status");
  if (fru.test_ok==1) {
    main_page.fields[TESTOK_VAL] = mk_label(LABEL_WIDTH, 0, y, "PASSED", GREEN_COLOR);
  } else if (fru.test_ok==2) {
    main_page.fields[TESTOK_VAL] = mk_label(LABEL_WIDTH, 0, y, "FAILED", COLOR_RED);
  } else if (fru.test_ok==0) {
    main_page.fields[TESTOK_VAL] = mk_label(LABEL_WIDTH, 0, y, "UNKNOWN", PAGE_COLOR);
  }
  y+=2;

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
  mvwaddstr(main_page.wp.w, y+2, 2, "Board MFG Date");
  strftime(main_page.mfg_date_label, 20, "%Y-%m-%d", &tm);
  main_page.fields[BRD_MFG_DATE_VAL] = mk_label(LABEL_WIDTH, 0, y, main_page.mfg_date_label, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Board manufacturer");
	main_page.fields[BRD_MFG_NAME_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_mfg_name, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Board product name");
	main_page.fields[BRD_PRODUCT_NAME_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_product_name, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Board serial number");
	main_page.fields[BRD_SERIAL_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_serial_number, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Board part number");
	main_page.fields[BRD_PN_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_part_number, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Board FRU file ID");
	main_page.fields[BRD_FRU_ID_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_fru_id, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Product manufacturer");
	main_page.fields[PRODUCT_MFG_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_p_product_mfg, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Product part/model number");
	main_page.fields[PART_MODEL_NUMBER_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_p_part_model_number, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Product version");
  main_page.fields[PRODUCT_VERSION_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_p_product_version, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Product serial number");
	main_page.fields[PRODUCT_SERIAL_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_p_serial_number, PAGE_COLOR);
  y+=2;

  mvwaddstr(main_page.wp.w, y+2, 2, "Product FRU file ID");
  main_page.fields[PRODUCT_FRU_ID_VAL] = mk_label(LABEL_WIDTH, 0, y, fru.val_p_fru_id, PAGE_COLOR);
  y+=2;
  
  main_page.fields[NULL_VAL] = NULL;
  
  main_page.f = new_form(main_page.fields);
  scale_form(main_page.f, &height, &width);
  set_form_win(main_page.f, main_page.wp.w);
  main_page.sw = derwin(main_page.wp.w, height, LABEL_WIDTH, 2, LABEL_WIDTH);
  set_form_sub(main_page.f, main_page.sw);

  post_form(main_page.f);


  
  /* swprintf(main_page.shred_label, 20, L"0x%08x", main_page.shred); */
  /* label(main_page.wp.w, 2, 2, L"SHRED", PAGE_LABEL_COLOR); */
  /* label(main_page.wp.w, 2, 10, main_page.shred_label, PAGE_COLOR); */

  /* label(main_page.wp.w, 2, 2, L"SHRED", PAGE_LABEL_COLOR); */
  /* label(main_page.wp.w, 2, 10, main_page.shred_label, PAGE_COLOR); */

  redrawwin(main_page.wp.w);
	//wnoutrefresh(main_page.wp.w);
  //win_show(, label, 1);
}

int
main_page_process(int ch) {
  if (!main_page.wp.hidden) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    curs_set(0);
    
    sprintf(main_page.time_label, "%02i:%02i:%02i UTC", tm.tm_hour, tm.tm_min, tm.tm_sec);
    set_field_buffer(main_page.fields[TIME_VAL], 0, main_page.time_label);
    //redrawwin(main_page.sw);
    //redrawwin(main_page.wp.w);
    //wnoutrefresh(main_page.sw);
    wnoutrefresh(main_page.wp.w);
  }
  return 0;
}

void
deinit_main_page(void) {
  int i;
  unpost_form(main_page.f);
	free_form(main_page.f);
  for (i=0; i<N_FIELDS; i++)
    free_field(main_page.fields[i]);
  delwin(main_page.wp.w);
}

struct window_params *
get_main_page_wp(void) {
  return &main_page.wp;
}
