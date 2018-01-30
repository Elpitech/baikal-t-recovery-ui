#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>
#include <form.h>

#include <time.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "pages.h"
#include "main_page.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "NET_PAGE"

#define LABEL_WIDTH 25
//                     |                 |
#define DHCP_LABEL_TXT "      Start"

#define IFF_UP (1<<0)

enum mac_fields {
  DHCP_LABEL,
  MAC0_VAL,
  MAC1_VAL,
  MAC2_VAL,
  MAC3_VAL,
  MAC4_VAL,
  MAC5_VAL,
#if ! defined(BOARD_MITX4)
  MAC10_VAL,
  MAC11_VAL,
  MAC12_VAL,
  MAC13_VAL,
  MAC14_VAL,
  MAC15_VAL,
  MAC20_VAL,
  MAC21_VAL,
  MAC22_VAL,
  MAC23_VAL,
  MAC24_VAL,
  MAC25_VAL,
#endif
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct {
  struct window_params wp;
  WINDOW *sw;
  uint32_t shred;
  char mac0_val[LABEL_WIDTH];
  char mac1_val[LABEL_WIDTH];
  char mac2_val[LABEL_WIDTH];
  char ip_val[NI_MAXHOST];
  FIELD *fields[N_FIELDS+1];
	FORM  *f;
} net_page;

int
get_ip_addr(char *buf) {
  struct ifaddrs *ifaddr, *ifa;
  int s;

  if (getifaddrs(&ifaddr) == -1) {
    err("getifaddrs failed to obtain IP");
    return -1;
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;
    s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    if ((strcmp(ifa->ifa_name,"eth0")==0) && (ifa->ifa_addr->sa_family==AF_INET)) {
      log("ifa_flags: %08x\n", ifa->ifa_flags);
      if (s != 0) {
        err("getnameinfo() failed: %s\n", gai_strerror(s));
        memset(buf, 0, NI_MAXHOST);
        return -2;
      }
      if (! (ifa->ifa_flags & IFF_UP)) {
        log("Interface seems to be down\n");
        memset(buf, 0, NI_MAXHOST);
        return -3;
      }
      log("\tInterface : <%s>\n", ifa->ifa_name );
      log("\t  Address : <%s>\n", buf); 
    }
  }
  
  freeifaddrs(ifaddr);
  return 0;
}

void
init_net_page(void) {
  int width, height;
  int i = 0;
  int ret;
  int cy = 2;
  net_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(net_page.wp.w, 0, 0);
  wbkgd(net_page.wp.w, PAGE_COLOR);

  net_page.wp.p = new_panel(net_page.wp.w);

  getmaxyx(net_page.wp.w, height, width);
  (void)height;

  //width = 6*3-1 -> 6 mac fields by 2 symbols = 6*2 + 6-1 single char spaces
  //this width makes it less ugly
  mvwaddstr(net_page.wp.w, cy, 2, "DHCP client");
  net_page.fields[DHCP_LABEL] = mk_label(6*3-1, 0, cy-2, DHCP_LABEL_TXT, BG_COLOR);

  cy+=2;
#if defined(BOARD_MITX4)
  mvwaddstr(net_page.wp.w, cy, 2, "MAC address");
#else
  mvwaddstr(net_page.wp.w, cy, 2, "MAC0 address");
#endif
  memset(net_page.mac0_val, 0, LABEL_WIDTH);
  for (i=0;i<6; i++) {
    sprintf(net_page.mac0_val+3*i, "%02x", fru.mac0[i]);
    net_page.fields[MAC0_VAL+i] = mk_editable_field_regex(2, 3*i, cy-2, net_page.mac0_val+3*i, "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR);
  }

  cy+=2;
#if ! defined(BOARD_MITX4)
  mvwaddstr(net_page.wp.w, cy, 2, "MAC1 address");
  memset(net_page.mac1_val, 0, LABEL_WIDTH);
  for (i=0;i<6; i++) {
    sprintf(net_page.mac1_val+3*i, "%02x", fru.mac1[i]);
    net_page.fields[MAC10_VAL+i] = mk_editable_field_regex(2, 3*i, cy-2, net_page.mac1_val+3*i, "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR);
  }
  cy+=2;
  mvwaddstr(net_page.wp.w, cy, 2, "MAC2 address");
  memset(net_page.mac2_val, 0, LABEL_WIDTH);
  for (i=0;i<6; i++) {
    sprintf(net_page.mac2_val+3*i, "%02x", fru.mac2[i]);
    net_page.fields[MAC20_VAL+i] = mk_editable_field_regex(2, 3*i, cy-2, net_page.mac2_val+3*i, "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR);
  }
  cy+=2;
#endif

  mvwaddstr(net_page.wp.w, cy, 2, "Current IP");
  ret = get_ip_addr(net_page.ip_val);
  if (ret != 0) {
    sprintf(net_page.ip_val, "N/A");
  }
  mvwaddstr(net_page.wp.w, cy, LABEL_WIDTH, net_page.ip_val);


  net_page.fields[NULL_VAL] = NULL;
  
  net_page.f = new_form(net_page.fields);
  scale_form(net_page.f, &height, &width);
  set_form_win(net_page.f, net_page.wp.w);
  net_page.sw = derwin(net_page.wp.w, height, LABEL_WIDTH, 2, LABEL_WIDTH);
  set_form_sub(net_page.f, net_page.sw);

  post_form(net_page.f);

  redrawwin(net_page.wp.w);
}

void
net_save_mac(int iface) {
  enum mac_fields first = MAC0_VAL+iface*6;
  uint8_t mac[6] = {0};
  int temp = 0;
  sscanf(field_buffer(net_page.fields[first], 0), "%02x", &temp);
  mac[0] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[first+1], 0), "%02x", &temp);
  mac[1] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[first+2], 0), "%02x", &temp);
  mac[2] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[first+3], 0), "%02x", &temp);
  mac[3] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[first+4], 0), "%02x", &temp);
  mac[4] = temp&0xff;
  temp = 0;
  sscanf(field_buffer(net_page.fields[first+5], 0), "%02x", &temp);
  mac[5] = temp&0xff;
  temp = 0;

  log("Save MAC: [%02x:%02x:%02x:%02x:%02x:%02x]\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  fru_mrec_update_mac(&fru, mac, iface);
}

int
net_page_process(int ch) {
  FIELD *f;
  if (!net_page.wp.hidden) {
    curs_set(1);
    switch (ch) {
    case KEY_DOWN:
      form_driver(net_page.f, REQ_NEXT_FIELD);
      break;
    case KEY_UP:
      form_driver(net_page.f, REQ_PREV_FIELD);
      break;
		case KEY_BACKSPACE:
		case 127:
      form_driver(net_page.f, REQ_DEL_PREV);
      break;
    case KEY_DC:
      form_driver(net_page.f, REQ_DEL_CHAR);
			break;
    case RKEY_ENTER://KEY_ENTER:
      f = current_field(net_page.f);
      if (f == net_page.fields[DHCP_LABEL]) {
        pages_params.start = START_DHCP;
        log("Set start dhcp flag\n");
      } else {
        net_save_mac(0);
#if ! defined(BOARD_MITX4)
        net_save_mac(1);
        net_save_mac(2);
#endif
      }
      break;
    case RKEY_ESC:
      break;
    default:
      form_driver(net_page.f, ch);
      break;
    }
    wnoutrefresh(net_page.wp.w);
  }
  return 0;
}

void
deinit_net_page(void) {
  int i;
  unpost_form(net_page.f);
	free_form(net_page.f);
  for (i=0; i<N_FIELDS; i++)
    free_field(net_page.fields[i]);
  delwin(net_page.wp.w);
}

struct window_params *
get_net_page_wp(void) {
  return &net_page.wp;
}
