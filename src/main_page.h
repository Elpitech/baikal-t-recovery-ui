#ifndef __MAIN_PAGE_H__
#define __MAIN_PAGE_H__

#include <menu.h>
#include <form.h>

void init_main_page(void);
int main_page_process(int ch);
void deinit_main_page(void);
struct window_params *get_main_page_wp(void);

#endif/*__MAIN_PAGE_H__*/
