#ifndef __BOOT_PAGE_H__
#define __BOOT_PAGE_H__

void init_boot_page(void);
int boot_page_process(int ch);
void deinit_boot_page(void);
struct window_params * get_boot_page_wp(void);
  
#endif/*__BOOT_PAGE_H__*/
