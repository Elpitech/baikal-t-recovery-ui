#ifndef __TOP_MENU_H__
#define __TOP_MENU_H__

#include "pages.h"

#define TOP_MENU_H 2
#define TOP_MENU_W COLS

void init_top_menu(struct window_params *main, struct window_params *boot, struct window_params *net, struct window_params *rec);
void hide_all_panels_except(struct window_params *p);
int top_menu_store(FILE *f);
int top_menu_load(FILE *f);
int top_menu_process(int ch);
void deinit_top_menu(void);
void top_menu_select_current(void);
void top_menu_unselect_all(void);

#endif/*__TOP_MENU_H__*/
