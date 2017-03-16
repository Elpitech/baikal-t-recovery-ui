#ifndef __NET_PAGE_H__
#define __NET_PAGE_H__

void init_net_page(void);
//void net_save_mac(void);
int net_page_process(int ch);
void deinit_net_page(void);
struct window_params *get_net_page_wp(void);

#endif/*__NET_PAGE_H__*/
