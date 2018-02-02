#ifndef __DATETIME_PAGE_H__
#define __DATETIME_PAGE_H__

void init_dt_page(void);
//void net_save_mac(void);
int dt_page_process(int ch);
void deinit_dt_page(void);
struct window_params *get_dt_page_wp(void);

#endif/*__DATETIME_PAGE_H__*/
