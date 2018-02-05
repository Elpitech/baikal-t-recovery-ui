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
  bool edit_mode;
} net_page;

int
get_ip_addr(char *buf) {
  struct ifaddrs *ifaddr, *ifa;
  int s;
  bool correct_value = false;

  if (getifaddrs(&ifaddr) == -1) {
    ferr("getifaddrs failed to obtain IP");
    return -1;
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;
    s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    if ((strcmp(ifa->ifa_name,"eth0")==0) && (ifa->ifa_addr->sa_family==AF_INET)) {
      flog("ifa_flags: %08x\n", ifa->ifa_flags);
      if (s != 0) {
        ferr("getnameinfo() failed: %s\n", gai_strerror(s));
        memset(buf, 0, NI_MAXHOST);
        return -2;
      }
      if (! (ifa->ifa_flags & IFF_UP)) {
        flog("Interface seems to be down\n");
        memset(buf, 0, NI_MAXHOST);
        return -3;
      }
      correct_value = true;
      flog("\tInterface : <%s>\n", ifa->ifa_name );
      flog("\t  Address : <%s>\n", buf); 
    }
  }
  
  freeifaddrs(ifaddr);
  if (!correct_value) {
    return -4;
  }
  return 0;
}

void
init_net_page(void) {
  int width, height;
  int i = 0;
  int ret;
  int cy = 2;
  net_page.edit_mode = false;
  net_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);//TOP_MENU_H, TOP_MENU_W, 0, 0);
  box(net_page.wp.w, 0, 0);
  wbkgd(net_page.wp.w, PAGE_COLOR);

  net_page.wp.p = new_panel(net_page.wp.w);

  getmaxyx(net_page.wp.w, height, width);
  (void)height;

  //width = 6*3-1 -> 6 mac fields by 2 symbols = 6*2 + 6-1 single char spaces
  //this width makes it less ugly
  mvwaddstr(net_page.wp.w, cy, 2, "DHCP client");
  net_page.fields[DHCP_LABEL] = mk_button(6*3-1, 0, cy-2, DHCP_LABEL_TXT, BG_COLOR);

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
#if defined(BOARD_MITX4)
  field_opts_off(net_page.fields[MAC5_VAL], O_AUTOSKIP);
#else
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

  flog("Save MAC: [%02x:%02x:%02x:%02x:%02x:%02x]\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  fru_mrec_update_mac(&fru, mac, iface);
}

static void
update_background(FIELD *f) {
  int i = MAC0_VAL;
  int last = MAC5_VAL;
#if ! defined(BOARD_MITX4)
  last = MAC25_VAL;
#endif
  for (;i<=last;i++) {
    if (net_page.edit_mode && f == net_page.fields[i]) {
      set_field_back(net_page.fields[i], EDIT_COLOR);
      set_field_fore(net_page.fields[i], EDIT_COLOR);
    } else {
      set_field_back(net_page.fields[i], BG_COLOR);
      set_field_fore(net_page.fields[i], BG_COLOR);
    }
  }
  //wnoutrefresh(net_page.wp.w);
}

int
net_page_process(int ch) {
  FIELD *f = current_field(net_page.f);
  FIELD *editable_first = net_page.fields[MAC0_VAL];
  FIELD *editable_last = net_page.fields[MAC5_VAL];
#if ! defined(BOARD_MITX4)
  editable_last = net_page.fields[MAC25_VAL];
#endif
  if (!net_page.wp.hidden) {
    curs_set(1);
    switch (ch) {
    case KEY_DOWN:
      if (net_page.edit_mode) {
        if (f != editable_last) {
          form_driver(net_page.f, REQ_NEXT_FIELD);
        }
      } else {
        form_driver(net_page.f, REQ_NEXT_FIELD);
      }
      break;
    case KEY_UP:
      if (net_page.edit_mode) {
        if (f != editable_first) {
          form_driver(net_page.f, REQ_PREV_FIELD);
        }
      } else {
        form_driver(net_page.f, REQ_PREV_FIELD);
      }
      break;
		case KEY_BACKSPACE:
		case 127:
      form_driver(net_page.f, REQ_DEL_PREV);
      break;
    case KEY_DC:
      form_driver(net_page.f, REQ_DEL_CHAR);
			break;
    case RKEY_ENTER:
      if (f == net_page.fields[DHCP_LABEL]) {
        pages_params.start = START_DHCP;
        flog("Set start dhcp flag\n");
      } else {
        net_page.edit_mode = true;
        pages_params.use_arrows = false;
      }
      break;
    case RKEY_ESC:
      if (net_page.edit_mode) {
        net_save_mac(0);
#if ! defined(BOARD_MITX4)
        net_save_mac(1);
        net_save_mac(2);
#endif
        update_background(NULL);
        //wrefresh(net_page.wp.w);
        //wnoutrefresh(net_page.wp.w);
        //redrawwin(net_page.wp.w);
        pages_params.use_arrows = true;
        net_page.edit_mode = false;
        form_driver(net_page.f, ch);
      }
      break;
    case KEY_LEFT:
      form_driver(net_page.f, REQ_PREV_CHAR);
      break;
    case KEY_RIGHT:
      form_driver(net_page.f, REQ_NEXT_CHAR);
      break;
    default:
      if (net_page.edit_mode) {
        form_driver(net_page.f, ch);
      }
      break;
    }
    f = current_field(net_page.f);
    update_background(f);
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
