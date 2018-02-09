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
#include "field_utils.h"
#include "top_menu.h"
#include "fru.h"

#define TAG "NET_PAGE"

#define MAC_SZ (6*3)
#define LABEL_WIDTH 25
#define COL1_W (LABEL_WIDTH-2)
#define COL2_OFF (COL1_W)
#define COL2_W (MAC_SZ-1)
#define COL3_OFF (COL2_OFF+COL2_W)
#define COL3_W (LABEL_WIDTH+2)
#define COL4_OFF (COL3_OFF+COL3_W)
#define COL4_W (LABEL_WIDTH)

//                     |                 |
#define DHCP_LABEL_TXT "      Start"

#define IFF_UP (1<<0)
#if ! defined(BOARD_MITX4)
#define MAC0_TXT "MAC0 address"
#else
#define MAC0_TXT "MAC address"
#endif

enum mac_fields {
  DHCP_LABEL,
  DHCP_BTN,
  MAC_LABEL,
  MAC0_VAL,
  MAC1_VAL,
  MAC2_VAL,
  MAC3_VAL,
  MAC4_VAL,
  MAC5_VAL,
#if ! defined(BOARD_MITX4)
  MAC1_LABEL,
  MAC10_VAL,
  MAC11_VAL,
  MAC12_VAL,
  MAC13_VAL,
  MAC14_VAL,
  MAC15_VAL,

  MAC2_LABEL,
  MAC20_VAL,
  MAC21_VAL,
  MAC22_VAL,
  MAC23_VAL,
  MAC24_VAL,
  MAC25_VAL,
#endif
  CUR_IP_LABEL,
  CUR_IP_VAL,
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct field_par fp[] = {
  [DHCP_LABEL] = LABEL_PAR(0, 0, COL1_W, "DHCP client", PAGE_COLOR, PAGE_COLOR),
  [DHCP_BTN] = BUTTON_PAR(COL2_OFF, 0, MAC_SZ-1, DHCP_LABEL_TXT, BG_COLOR, BG_COLOR),

  [CUR_IP_LABEL] = LABEL_PAR(0, 2, COL1_W, "Current IP", PAGE_COLOR, PAGE_COLOR),
  [CUR_IP_VAL] = LABEL_PAR(COL2_OFF, 2, COL1_W, "Current IP", PAGE_COLOR, PAGE_COLOR),

  
  [MAC_LABEL] = LABEL_PAR(0, 4, COL1_W, MAC0_TXT, PAGE_COLOR, PAGE_COLOR),
  [MAC0_VAL] = LINE_PAR(COL2_OFF, 4, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC1_VAL] = LINE_PAR(COL2_OFF+3, 4, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC2_VAL] = LINE_PAR(COL2_OFF+6, 4, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC3_VAL] = LINE_PAR(COL2_OFF+9, 4, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC4_VAL] = LINE_PAR(COL2_OFF+12, 4, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC5_VAL] = LINE_PAR(COL2_OFF+15, 4, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
#if ! defined(BOARD_MITX4)
  [MAC1_LABEL] = LABEL_PAR(0, 4, COL1_W, "MAC1 address", PAGE_COLOR, PAGE_COLOR),
  [MAC10_VAL] = LINE_PAR(COL2_OFF, 6, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC11_VAL] = LINE_PAR(COL2_OFF+3, 6, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC12_VAL] = LINE_PAR(COL2_OFF+6, 6, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC13_VAL] = LINE_PAR(COL2_OFF+9, 6, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC14_VAL] = LINE_PAR(COL2_OFF+12, 6, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC15_VAL] = LINE_PAR(COL2_OFF+15, 6, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),

  [MAC2_LABEL] = LABEL_PAR(0, 6, COL1_W, "MAC2 address", PAGE_COLOR, PAGE_COLOR),
  [MAC20_VAL] = LINE_PAR(COL2_OFF, 8, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC21_VAL] = LINE_PAR(COL2_OFF+3, 8, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC22_VAL] = LINE_PAR(COL2_OFF+6, 8, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC23_VAL] = LINE_PAR(COL2_OFF+9, 8, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC24_VAL] = LINE_PAR(COL2_OFF+12, 8, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC25_VAL] = LINE_PAR(COL2_OFF+15, 8, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
#endif
};

static struct {
  struct window_params wp;
  WINDOW *w;
  char mac0_val[MAC_SZ];
  char mac1_val[MAC_SZ];
  char mac2_val[MAC_SZ];
  char ip_val[NI_MAXHOST];
  FIELD *fields[N_FIELDS+1];
  FORM *form;
  bool edit_mode;
  bool selected;
  FIELD *first_active;
  FIELD *last_active;
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
  net_page.edit_mode = false;
  net_page.selected = false;
  net_page.first_active = NULL;
  net_page.last_active = NULL;
  net_page.wp.w = newwin(LINES-TOP_MENU_H-1,0,TOP_MENU_H,0);
  box(net_page.wp.w, 0, 0);
  wbkgd(net_page.wp.w, PAGE_COLOR);

  net_page.wp.p = new_panel(net_page.wp.w);

  getmaxyx(net_page.wp.w, height, width);
  (void)height;

  for (i=0; i<N_FIELDS; i++) {
    net_page.fields[i] = mk_field(&fp[i]);
    if (fp[i].ft != FT_LABEL) {
      if (net_page.first_active == NULL) {
        net_page.first_active = net_page.fields[i];
      }
      net_page.last_active = net_page.fields[i];
    }
  }
  net_page.fields[NULL_VAL] = NULL;
  
  net_page.form = new_form(net_page.fields);
  form_opts_off(net_page.form, O_NL_OVERLOAD);
  form_opts_off(net_page.form, O_BS_OVERLOAD);
  scale_form(net_page.form, &height, &width);
  set_form_win(net_page.form, net_page.wp.w);
  net_page.w = derwin(net_page.wp.w, height, width, 2, 2);
  set_form_sub(net_page.form, net_page.w);

  post_form(net_page.form);

  redrawwin(net_page.wp.w);

  int ret = get_ip_addr(net_page.ip_val);
  if (ret != 0) {
    sprintf(net_page.ip_val, "N/A");
  }
  set_field_buffer(net_page.fields[CUR_IP_VAL], 0, net_page.ip_val);

  memset(net_page.mac0_val, 0, MAC_SZ);
  for (i=0;i<6; i++) {
    sprintf(net_page.mac0_val+3*i, "%02x", fru.mac0[i]);
    set_field_buffer(net_page.fields[MAC0_VAL+i], 0, net_page.mac0_val+3*i);
  }
#if defined(BOARD_MITX4)
  field_opts_off(net_page.fields[MAC5_VAL], O_AUTOSKIP);
#else
  memset(net_page.mac1_val, 0, MAC_SZ);
  for (i=0;i<6; i++) {
    sprintf(net_page.mac1_val+3*i, "%02x", fru.mac1[i]);
    set_field_buffer(net_page.fields[MAC10_VAL+i], 0, net_page.mac1_val+3*i);
  }
  memset(net_page.mac2_val, 0, MAC_SZ);
  for (i=0;i<6; i++) {
    sprintf(net_page.mac2_val+3*i, "%02x", fru.mac2[i]);
    set_field_buffer(net_page.fields[MAC20_VAL+i], 0, net_page.mac2_val+3*i);
  }
#endif
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

int
net_page_process(int ch) {
  if (!net_page.wp.hidden) {
    FIELD *f = current_field(net_page.form);
    if (!net_page.selected) {
      curs_set(0);
      switch (ch) {
      case KEY_DOWN:
        net_page.selected = true;
        pages_params.use_arrows = false;
        net_page.edit_mode = false;
        break;
      }
    } else {
      curs_set(1);
      switch (ch) {
      case KEY_DOWN:
        if (net_page.edit_mode) {
          net_save_mac(0);
#if ! defined(BOARD_MITX4)
          net_save_mac(1);
          net_save_mac(2);
#endif
          field_par_unset_line_bg(fp, net_page.fields, N_FIELDS);
          net_page.edit_mode = false;
        }
        if (f != net_page.last_active) {
          form_driver(net_page.form, REQ_NEXT_FIELD);
        }
        break;
      case KEY_UP:
        if (net_page.edit_mode) {
          net_save_mac(0);
#if ! defined(BOARD_MITX4)
          net_save_mac(1);
          net_save_mac(2);
#endif
          field_par_unset_line_bg(fp, net_page.fields, N_FIELDS);
          net_page.edit_mode = false;
        }
        if (f != net_page.first_active) {
          form_driver(net_page.form, REQ_PREV_FIELD);
        } else {
          net_page.selected = false;
          pages_params.use_arrows = true;
        }
        break;
      case RKEY_ENTER:
        if (f == net_page.fields[DHCP_BTN]) {
          pages_params.start = START_DHCP;
          flog("Set start dhcp flag\n");
        }
        break;
      case RKEY_ESC:
        break;
      case KEY_LEFT:
        if (field_opts(f) & O_EDIT) {
          net_page.edit_mode = true;
          field_par_set_line_bg(fp, f, net_page.fields, N_FIELDS);
          form_driver(net_page.form, REQ_PREV_CHAR);
        }
        break;
      case KEY_RIGHT:
        if (field_opts(f) & O_EDIT) {
          net_page.edit_mode = true;
          field_par_set_line_bg(fp, f, net_page.fields, N_FIELDS);
          form_driver(net_page.form, REQ_NEXT_CHAR);
        }
        break;
      case KEY_BACKSPACE:
      case 127:
        net_page.edit_mode = true;
        field_par_set_line_bg(fp, f, net_page.fields, N_FIELDS);
        form_driver(net_page.form, REQ_DEL_PREV);
        break;
      case KEY_DC:
        net_page.edit_mode = true;
        field_par_set_line_bg(fp, f, net_page.fields, N_FIELDS);
        form_driver(net_page.form, REQ_DEL_CHAR);
        break;
      case -1:
        break;
      default:
        net_page.edit_mode = true;
        field_par_set_line_bg(fp, f, net_page.fields, N_FIELDS);
        form_driver(net_page.form, ch);
        break;
      }

    }
  }
  return 0;
}

void
deinit_net_page(void) {
  int i;
  unpost_form(net_page.form);
	free_form(net_page.form);
  for (i=0; i<N_FIELDS; i++)
    free_field(net_page.fields[i]);
  delwin(net_page.wp.w);
}

struct window_params *
get_net_page_wp(void) {
  return &net_page.wp;
}
