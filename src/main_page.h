#ifndef __MAIN_PAGE_H__
#define __MAIN_PAGE_H__

#include <menu.h>
#include <form.h>

FIELD *mk_spinner(int w, int x, int y, char **strings, int n_str, int default_n, chtype c);
void spinner_spin(FIELD *f);
FIELD * mk_label(int w, int x, int y, char *string, chtype c);
FIELD * mk_label_colored(int w, int x, int y, char *string, chtype fore, chtype back);
FIELD * mk_label2(int w, int h, int x, int y, char *string, chtype c);
FIELD * mk_button(int w, int x, int y, char *string, chtype c);
FIELD * mk_editable_field_regex(int w, int x, int y, char *string, char *regex, chtype c);
FIELD *
mk_editable_field_regex_ex(int w, int x, int y, char *string, char *regex, chtype c, bool autoskip, bool static_size, int max_growth);

void init_main_page(void);
int main_page_process(int ch);
void deinit_main_page(void);
struct window_params *get_main_page_wp(void);

#endif/*__MAIN_PAGE_H__*/
