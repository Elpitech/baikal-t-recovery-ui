#include "field_utils.h"
#include <menu.h>
#include <form.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "common.h"

#define TAG "FIELD_UTILS"

void
field_par_set_line_bg(struct field_par *fp, FIELD *f, FIELD **fields, int n_fields) {
  int i;
  for (i=0; i<n_fields; i++) {
    if (fp[i].ft == FT_LINE) {
      if (f==fields[i]) {
        set_field_back(fields[i], EDIT_COLOR);
        set_field_fore(fields[i], EDIT_COLOR);
      } else {
        if (field_opts(fields[i]) & O_ACTIVE) {
          set_field_back(fields[i], BG_COLOR);
          set_field_fore(fields[i], BG_COLOR);
        }
      }
    }
  }
}

void
field_par_unset_line_bg(struct field_par *fp, FIELD **fields, int n_fields) {
  int i;
  for (i=0; i<n_fields; i++) {
    if (fp[i].ft == FT_LINE) {
      if (field_opts(fields[i]) & O_ACTIVE) {
        set_field_back(fields[i], BG_COLOR);
        set_field_fore(fields[i], BG_COLOR);
      }
    }
  }
}

FIELD *
mk_spinner(int w, int x, int y, char *strings, int default_n, chtype c, bool centered) {
  flog("mk_spinner\n");
  FIELD *f = new_field(1, w, y, x, 0, 0);
  int i = 0;
  int n_str = 0;
  int str_ctr = 0;
  int total_len = 0;
  char *start = 0;
  char *end = 0;
  struct spinner_arg *s = (struct spinner_arg *)malloc(sizeof(struct spinner_arg));
  //count strs
  while (1) {
    if (i!=0 && strings[i]==';' && strings[i-1]=='\0') {
      break;
    }
    if (strings[i]=='\0') {
      n_str++;
    }
    i++;
  }
  s->strs = malloc(sizeof(char *)*n_str);
  total_len = i;
  start = strings;
  str_ctr = 0;
  flog("Found %i strings\n", n_str);
  
  for (i=0;i<total_len;i++) {
    if ((i!=0 && strings[i]==';' && strings[i-1]=='\0') || (str_ctr>=n_str)) {
      break;
    }
    if (strings[i]=='\0') {
      end = &strings[i];
      ptrdiff_t sz = (int)(end-start);
      s->strs[str_ctr] = (char *)malloc(sz+1);
      memset(s->strs[str_ctr], 0, sz+1);
      memcpy(s->strs[str_ctr], start, sz);
      flog("Found string %i: %s at %i, len: %i\n", str_ctr, s->strs[str_ctr], start-strings, end-start);
      str_ctr ++;
      start = end+1;
    }
  }
  flog("Finished parsing strings\n");
  s->current_str = default_n;
  s->n_str = n_str;
  set_field_userptr(f, (void *)s);
  field_opts_off(f, O_EDIT);
  set_field_buffer(f, 0, s->strs[default_n]);
  if (centered) {
    set_field_just(f, JUSTIFY_CENTER);
  }
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

void
free_spinner_data(FIELD *f) {
  void *p = field_userptr(f);
  if (p==NULL) {
    return;
  }
  struct spinner_arg *s = (struct spinner_arg *)p;
  int i = 0;
  for (;i<s->n_str;i++) {
    free(s->strs[i]);
  }
  free(s->strs);
  free(s);
}

int
spinner_current_index(FIELD *f) {
  void *p = field_userptr(f);
  if (p==NULL) {
    return -1;
  }
  struct spinner_arg *s = (struct spinner_arg *)p;
  return s->current_str;
}

FIELD *
mk_field(struct field_par *fp) {
  FIELD *f;
  switch (fp->ft) {
  case FT_LABEL:
    f = mk_label_colored(fp->w, fp->x, fp->y, fp->txt, fp->fore, fp->back);
    //flog("label %i %i %i %s\n", fp->w, fp->x, fp->y, fp->txt);
    break;
  case FT_LINE:
    f = mk_editable_field_regex_ex(fp->w, fp->x, fp->y, fp->txt, fp->regex, fp->fore, fp->autoskip, fp->static_size, fp->max_growth, fp->active);
    break;
  case FT_SPINNER:
    f = mk_spinner(fp->w, fp->x, fp->y, fp->txt, fp->iarg, fp->fore, false);
    break;
  case FT_BUTTON:
    f = mk_button(fp->w, fp->x, fp->y, fp->txt, fp->fore);
    break;
  }
  return f;
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
  field_opts_off(f, O_ACTIVE);
  set_field_buffer(f, 0, string);
  set_field_fore(f, fore);
  set_field_back(f, back);
  return f;
}

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
mk_editable_field_regex_ex(int w, int x, int y, char *string, char *regex, chtype c, bool autoskip, bool static_size, int max_growth, bool active) {
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
  if (!active) {
    field_opts_off(f, O_ACTIVE);
  }
  set_field_buffer(f, 0, string);
  set_field_fore(f, c);
  set_field_back(f, c);
  return f;
}

FIELD *
mk_editable_field_regex(int w, int x, int y, char *string, char *regex, chtype c) {
  return mk_editable_field_regex_ex(w, x, y, string, regex, c, true, true, 0, true);
}
