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
#include "main_page.h"
#include "top_menu.h"
#define TAG "MAIN_PAGE"

#define LABEL_WIDTH 20

enum fields {
  TIME_LABEL = 0,
  TIME_VAL,
  DATE_LABEL,
  DATE_VAL,
  SHRED_LABEL,
  SHRED_VAL,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct {
  WINDOW *w;
  WINDOW *sw;
  uint32_t shred;
  char shred_label[20];
  char time_label[20];
  char date_label[20];
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
} main_page;

FIELD *
mk_label(int w, int x, int y, char *string, chtype c) {
  FIELD *f = new_field(1, w, y, x, 0, 0);
  field_opts_off(f, O_EDIT);
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
  main_page.w = newwin(LINES-TOP_MENU_H,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(main_page.w, 0, 0);
  wbkgd(main_page.w, PAGE_COLOR);

  getmaxyx(main_page.w, height, width);
  (void)height;

  main_page.shred = read_shred();
  log("shred: 0x%08x\n", main_page.shred);
  t = time(NULL);
  tm = *localtime(&t);

  main_page.fields[TIME_LABEL] = mk_label(LABEL_WIDTH, 0, TIME_LABEL, "TIME", PAGE_COLOR);
  sprintf(main_page.time_label, "%02i:%02i:%02i", tm.tm_hour, tm.tm_min, tm.tm_sec);
	main_page.fields[TIME_VAL] = mk_label(LABEL_WIDTH, LABEL_WIDTH, TIME_LABEL, main_page.time_label, PAGE_COLOR);

  main_page.fields[DATE_LABEL] = mk_label(LABEL_WIDTH, 0, DATE_LABEL, "DATE", PAGE_COLOR);
  sprintf(main_page.date_label, "%02i-%02i-%04i", tm.tm_mday, tm.tm_mon+1, tm.tm_year + 1900);
	main_page.fields[DATE_VAL] = mk_label(LABEL_WIDTH, LABEL_WIDTH, DATE_LABEL, main_page.date_label, PAGE_COLOR);

  main_page.fields[SHRED_LABEL] = mk_label(LABEL_WIDTH, 0, SHRED_LABEL, "SHRED", PAGE_COLOR);
  sprintf(main_page.shred_label, "0x%08x", main_page.shred);
	main_page.fields[SHRED_VAL] = mk_label(LABEL_WIDTH, LABEL_WIDTH, SHRED_LABEL, main_page.shred_label, PAGE_COLOR);
  main_page.fields[NULL_VAL] = NULL;
  
  main_page.f = new_form(main_page.fields);
  scale_form(main_page.f, &height, &width);
  set_form_win(main_page.f, main_page.w);
  main_page.sw = derwin(main_page.w, height, width, 2, 2);
  set_form_sub(main_page.f, main_page.sw);

  post_form(main_page.f);


  
  /* swprintf(main_page.shred_label, 20, L"0x%08x", main_page.shred); */
  /* label(main_page.w, 2, 2, L"SHRED", PAGE_LABEL_COLOR); */
  /* label(main_page.w, 2, 10, main_page.shred_label, PAGE_COLOR); */

  /* label(main_page.w, 2, 2, L"SHRED", PAGE_LABEL_COLOR); */
  /* label(main_page.w, 2, 10, main_page.shred_label, PAGE_COLOR); */

  redrawwin(main_page.w);
	//wnoutrefresh(main_page.w);
  //win_show(, label, 1);
}

int
main_page_process(int ch) {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  sprintf(main_page.time_label, "%02i:%02i:%02i", tm.tm_hour, tm.tm_min, tm.tm_sec);
  set_field_buffer(main_page.fields[TIME_VAL], 0, main_page.time_label);
  //redrawwin(main_page.sw);
  //redrawwin(main_page.w);
  //wnoutrefresh(main_page.sw);
	wnoutrefresh(main_page.w);
  return 0;
}

void
deinit_main_page(void) {
  int i;
  unpost_form(main_page.f);
	free_form(main_page.f);
  for (i=0; i<N_FIELDS; i++)
    free_field(main_page.fields[i]);
  delwin(main_page.w);
}
