#ifndef __RECOVERY_PAGE_H__
#define __RECOVERY_PAGE_H__

void init_recovery_page(void);
int recovery_page_process(int ch);
void deinit_recovery_page(void);
struct window_params * get_recovery_page_wp(void);

#endif/*__RECOVERY_PAGE_H__*/
