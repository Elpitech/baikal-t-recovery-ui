#ifndef __FIELD_UTILS_H__
#define __FIELD_UTILS_H__

#include <form.h>

#define LABEL_PAR(x, y, w, str, fore, back) {FT_LABEL, NULL, x, y, w, str, NULL, fore, back, false, 0, false, false, 0}
#define LINE_PAR(x, y, w, str, regex, fore, back, static_size, max_growth, autoskip) {FT_LINE, NULL, x, y, w, str, regex, fore, back, static_size, max_growth, autoskip, true, 0}
#define LINE_PAR_EX(x, y, w, str, regex, fore, back, static_size, max_growth, autoskip, active) {FT_LINE, NULL, x, y, w, str, regex, fore, back, static_size, max_growth, autoskip, active, 0}
#define SPINNER_PAR(x, y, w, strs, default_n, fore, back) {FT_SPINNER, NULL, x, y, w, strs, NULL, fore, back, false, 0, false, true, default_n}
#define BUTTON_PAR(x, y, w, str, fore, back) {FT_BUTTON, NULL, x, y, w, str, NULL, fore, back, false, 0, false, true, 0}

enum FIELD_TYPE {
  FT_LABEL=0,
  FT_LINE,
  FT_BUTTON,
  FT_SPINNER
};

struct field_par {
  enum FIELD_TYPE ft;
  FIELD *f;
  int x;
  int y;
  int w;
  char *txt;
  char *regex;
  chtype fore;
  chtype back;
  bool static_size;
  int max_growth;
  bool autoskip;
  bool active;
  int iarg;
};

struct spinner_arg {
  int current_str;
  int n_str;
  char **strs;
};

void field_par_set_line_bg(struct field_par *fp, FIELD *f, FIELD **fields, int n_fields);
void field_par_unset_line_bg(struct field_par *fp, FIELD **fields, int n_fields);
FIELD * mk_spinner(int w, int x, int y, char *strings, int default_n, chtype c, bool centered);
void spinner_spin(FIELD *f);
void free_spinner_data(FIELD *f);
int spinner_current_index(FIELD *f);
FIELD * mk_field(struct field_par *fp);
FIELD * mk_button(int w, int x, int y, char *string, chtype c);
FIELD * mk_label(int w, int x, int y, char *string, chtype c);
FIELD * mk_label_colored(int w, int x, int y, char *string, chtype fore, chtype back);
FIELD * mk_label2(int w, int h, int x, int y, char *string, chtype c);
FIELD * mk_editable_field_regex_ex(int w, int x, int y, char *string, char *regex, chtype c, bool autoskip, bool static_size, int max_growth, bool active);
FIELD * mk_editable_field_regex(int w, int x, int y, char *string, char *regex, chtype c);

#endif/*__FIELD_UTILS_H__*/
