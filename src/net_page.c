#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>
#include <form.h>

#include <time.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
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

#define IP_W 3
#define ADDR1_OFF COL2_OFF
#define ADDR2_OFF (ADDR1_OFF+IP_W+1)
#define ADDR3_OFF (ADDR2_OFF+IP_W+1)
#define ADDR4_OFF (ADDR3_OFF+IP_W+1)

//                     |                 |
#define CONFIGURE_TXT  "      Start"

#define IFF_UP (1<<0)
#if ! defined(BOARD_MITX4)
#define MAC0_TXT "MAC0 address"
#else
#define MAC0_TXT "MAC address"
#endif

enum mac_fields {
  IP_SETTINGS_LABEL,
  IP_SETTINGS_SPN,
  IP_ADDR_SETTINGS_LABEL,
  IP_ADDR_VAL1, IP_ADDR_DOT1, IP_ADDR_VAL2, IP_ADDR_DOT2, IP_ADDR_VAL3, IP_ADDR_DOT3, IP_ADDR_VAL4,

  NETMASK_LABEL,
  NETMASK_VAL1, NETMASK_DOT1, NETMASK_VAL2, NETMASK_DOT2, NETMASK_VAL3, NETMASK_DOT3, NETMASK_VAL4,

  GATEWAY_LABEL,
  GATEWAY_VAL1, GATEWAY_DOT1, GATEWAY_VAL2, GATEWAY_DOT2, GATEWAY_VAL3, GATEWAY_DOT3, GATEWAY_VAL4,

  DNS1_LABEL,
  DNS1_VAL1, DNS1_DOT1, DNS1_VAL2, DNS1_DOT2, DNS1_VAL3, DNS1_DOT3, DNS1_VAL4,

  CONFIGURE_LABEL,
  CONFIGURE_BTN,
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
  NULL_VAL,
  N_FIELDS=NULL_VAL
};

static struct field_par fp[] = {
  [IP_SETTINGS_LABEL] = LABEL_PAR(0, 0, COL1_W, "IP settings", PAGE_COLOR, PAGE_COLOR),
  [IP_SETTINGS_SPN] = SPINNER_PAR(COL2_OFF, 0, COL2_W, "Automatic\0Manual\0;", 0, BG_COLOR, BG_COLOR),
  
  [IP_ADDR_SETTINGS_LABEL] = LABEL_PAR(0, 2, COL1_W, "  IP address", PAGE_COLOR, PAGE_COLOR),
  [IP_ADDR_VAL1] = LINE_PAR_EX(ADDR1_OFF, 2, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [IP_ADDR_DOT1] = LABEL_PAR(ADDR2_OFF-1, 2, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [IP_ADDR_VAL2] = LINE_PAR_EX(ADDR2_OFF, 2, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [IP_ADDR_DOT2] = LABEL_PAR(ADDR3_OFF-1, 2, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [IP_ADDR_VAL3] = LINE_PAR_EX(ADDR3_OFF, 2, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [IP_ADDR_DOT3] = LABEL_PAR(ADDR4_OFF-1, 2, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [IP_ADDR_VAL4] = LINE_PAR_EX(ADDR4_OFF, 2, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),

  [NETMASK_LABEL] = LABEL_PAR(0, 4, COL1_W, "  Netmask", PAGE_COLOR, PAGE_COLOR),
  [NETMASK_VAL1] = LINE_PAR_EX(ADDR1_OFF, 4, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [NETMASK_DOT1] = LABEL_PAR(ADDR2_OFF-1, 4, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [NETMASK_VAL2] = LINE_PAR_EX(ADDR2_OFF, 4, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [NETMASK_DOT2] = LABEL_PAR(ADDR3_OFF-1, 4, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [NETMASK_VAL3] = LINE_PAR_EX(ADDR3_OFF, 4, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [NETMASK_DOT3] = LABEL_PAR(ADDR4_OFF-1, 4, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [NETMASK_VAL4] = LINE_PAR_EX(ADDR4_OFF, 4, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  
  [GATEWAY_LABEL] = LABEL_PAR(0, 6, COL1_W, "  Gateway", PAGE_COLOR, PAGE_COLOR),
  [GATEWAY_VAL1] = LINE_PAR_EX(ADDR1_OFF, 6, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [GATEWAY_DOT1] = LABEL_PAR(ADDR2_OFF-1, 6, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [GATEWAY_VAL2] = LINE_PAR_EX(ADDR2_OFF, 6, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [GATEWAY_DOT2] = LABEL_PAR(ADDR3_OFF-1, 6, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [GATEWAY_VAL3] = LINE_PAR_EX(ADDR3_OFF, 6, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [GATEWAY_DOT3] = LABEL_PAR(ADDR4_OFF-1, 6, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [GATEWAY_VAL4] = LINE_PAR_EX(ADDR4_OFF, 6, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),

  [DNS1_LABEL] = LABEL_PAR(0, 8, COL1_W, "  DNS", PAGE_COLOR, PAGE_COLOR),
  [DNS1_VAL1] = LINE_PAR_EX(ADDR1_OFF, 8, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [DNS1_DOT1] = LABEL_PAR(ADDR2_OFF-1, 8, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [DNS1_VAL2] = LINE_PAR_EX(ADDR2_OFF, 8, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [DNS1_DOT2] = LABEL_PAR(ADDR3_OFF-1, 8, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [DNS1_VAL3] = LINE_PAR_EX(ADDR3_OFF, 8, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),
  [DNS1_DOT3] = LABEL_PAR(ADDR4_OFF-1, 8, 1, ".", PAGE_COLOR, PAGE_COLOR),
  [DNS1_VAL4] = LINE_PAR_EX(ADDR4_OFF, 8, IP_W, "   ", "*", PAGE_COLOR, PAGE_COLOR, true, 0, false, false),

  [CONFIGURE_LABEL] = LABEL_PAR(0, 10, COL1_W, "Configure network", PAGE_COLOR, PAGE_COLOR),
  [CONFIGURE_BTN] = BUTTON_PAR(COL2_OFF, 10, COL2_W, CONFIGURE_TXT, BG_COLOR, BG_COLOR),
  
  [MAC_LABEL] = LABEL_PAR(0, 12, COL1_W, MAC0_TXT, PAGE_COLOR, PAGE_COLOR),
  [MAC0_VAL] = LINE_PAR(COL2_OFF, 12, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC1_VAL] = LINE_PAR(COL2_OFF+3, 12, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC2_VAL] = LINE_PAR(COL2_OFF+6, 12, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC3_VAL] = LINE_PAR(COL2_OFF+9, 12, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC4_VAL] = LINE_PAR(COL2_OFF+12, 12, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC5_VAL] = LINE_PAR(COL2_OFF+15, 12, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
#if ! defined(BOARD_MITX4)
  [MAC1_LABEL] = LABEL_PAR(0, 14, COL1_W, "MAC1 address", PAGE_COLOR, PAGE_COLOR),
  [MAC10_VAL] = LINE_PAR(COL2_OFF, 14, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC11_VAL] = LINE_PAR(COL2_OFF+3, 14, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC12_VAL] = LINE_PAR(COL2_OFF+6, 14, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC13_VAL] = LINE_PAR(COL2_OFF+9, 14, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC14_VAL] = LINE_PAR(COL2_OFF+12, 14, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC15_VAL] = LINE_PAR(COL2_OFF+15, 14, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),

  [MAC2_LABEL] = LABEL_PAR(0, 16, COL1_W, "MAC2 address", PAGE_COLOR, PAGE_COLOR),
  [MAC20_VAL] = LINE_PAR(COL2_OFF, 16, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC21_VAL] = LINE_PAR(COL2_OFF+3, 16, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC22_VAL] = LINE_PAR(COL2_OFF+6, 16, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC23_VAL] = LINE_PAR(COL2_OFF+9, 16, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC24_VAL] = LINE_PAR(COL2_OFF+12, 16, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
  [MAC25_VAL] = LINE_PAR(COL2_OFF+15, 16, 2, "00", "[0-9a-fA-F][0-9a-fA-F]", BG_COLOR, BG_COLOR, true, 0, true),
#endif
};

static struct {
  struct window_params wp;
  WINDOW *w;
  char mac0_val[MAC_SZ];
  char mac1_val[MAC_SZ];
  char mac2_val[MAC_SZ];
  char ip_val[NI_MAXHOST+1];
  char netmask_val[NI_MAXHOST+1];
  char gw_val[NI_MAXHOST+1];
  char dns1_val[NI_MAXHOST+1];
  FIELD *fields[N_FIELDS+1];
  FORM *form;
  bool edit_mode;
  bool selected;
  FIELD *first_active;
  FIELD *last_active;
  bool ip_auto;
  bool ip_valid;
  bool nm_valid;
  bool gw_valid;
  bool dns1_valid;
} net_page;

int
get_gw(char *buf)  {
  char interface[] = "eth0";
  int ret = -1;
  
  char *cmd = (char *)malloc(1024);
  memset(cmd, 0, 1024);
  sprintf(cmd,"route -n | grep %s  | grep 'UG[ \t]' | awk '{print $2}'", interface);
  FILE* fp = popen(cmd, "r");
  char line[256]={};
  free(cmd);
  
  if(fgets(line, sizeof(line), fp) != NULL) {
    int l = strlen(line);
    l = (l>NI_MAXHOST?NI_MAXHOST:l);
    strncpy(buf, line, l);
    ret = 0;
  }
  
  pclose(fp);
  return ret;
}

int
get_ip_addr(char *buf, char *netmask) {
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
    s = getnameinfo(ifa->ifa_netmask, sizeof(struct sockaddr_in), netmask, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
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
      flog("\t  Netmask : <%s>\n", netmask);
    }
  }
  
  freeifaddrs(ifaddr);
  if (!correct_value) {
    return -4;
  }
  return 0;
}

void
set_ip_fields(int first_field, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4) {
  char buf[4];
  buf[3] = 0;
  snprintf(buf, 4, "%03i", v1);
  set_field_buffer(net_page.fields[first_field], 0, buf);
  first_field+=2;
  snprintf(buf, 4, "%03i", v2);
  set_field_buffer(net_page.fields[first_field], 0, buf);
  first_field+=2;
  snprintf(buf, 4, "%03i", v3);
  set_field_buffer(net_page.fields[first_field], 0, buf);
  first_field+=2;
  snprintf(buf, 4, "%03i", v4);
  set_field_buffer(net_page.fields[first_field], 0, buf);
}

void
set_auto_ip(bool automatic) {
  int first = IP_ADDR_VAL1;
  int last = GATEWAY_VAL4;
  for (;first<=last;first++) {
    if ((first == GATEWAY_LABEL) || (first == NETMASK_LABEL)) {
      continue;
    }
    if (automatic) {
      set_field_back(net_page.fields[first], PAGE_COLOR);
      set_field_fore(net_page.fields[first], PAGE_COLOR);
      field_opts_off(net_page.fields[first], O_ACTIVE);
    } else {
      set_field_back(net_page.fields[first], BG_COLOR);
      set_field_fore(net_page.fields[first], BG_COLOR);
      field_opts_on(net_page.fields[first], O_ACTIVE);
    }
    first++;
  }
}

void
check_ip_part(int part, int id) {
  if (part>255) {
    set_field_fore(net_page.fields[id], RED_EDIT_COLOR);
    set_field_back(net_page.fields[id], RED_EDIT_COLOR);
  } else {
    set_field_fore(net_page.fields[id], BG_COLOR);
    set_field_back(net_page.fields[id], BG_COLOR);        
  }
}

void
read_ip_parts(int first_id, unsigned int *p1, unsigned int *p2, unsigned int *p3, unsigned int *p4) {
  char *ip1 = field_buffer(net_page.fields[first_id], 0);
  char *ip2 = field_buffer(net_page.fields[first_id+2], 0);
  char *ip3 = field_buffer(net_page.fields[first_id+4], 0);
  char *ip4 = field_buffer(net_page.fields[first_id+6], 0);
  *p1 = strtoul(ip1, NULL, 10);
  *p2 = strtoul(ip2, NULL, 10);
  *p3 = strtoul(ip3, NULL, 10);
  *p4 = strtoul(ip4, NULL, 10);
}

bool
validate_ip_params(void) {
  unsigned int iip1, iip2, iip3, iip4 = 0;
  unsigned int inm1, inm2, inm3, inm4 = 0;
  unsigned int igw1, igw2, igw3, igw4 = 0;
  unsigned int idns11, idns12, idns13, idns14 = 0;

  read_ip_parts(IP_ADDR_VAL1, &iip1, &iip2, &iip3, &iip4);
  read_ip_parts(NETMASK_VAL1, &inm1, &inm2, &inm3, &inm4);
  read_ip_parts(GATEWAY_VAL1, &igw1, &igw2, &igw3, &igw4);
  read_ip_parts(DNS1_VAL1, &idns11, &idns12, &idns13, &idns14);
#if 0
  net_page.ip_valid = false;
  net_page.nm_valid = false;
  net_page.gw_valid = false;
  net_page.dns1_valid = false;

  //flog("idns1: %i.%i.%i.%i\n", idns11, idns12, idns13, idns14);
  if (idns11 || idns12 || idns13 || idns14) {
    if ((idns11<=255) && (idns12<=255) && (idns13<=255) && (idns14<=255)) {
      net_page.dns1_valid = true;
    }
    check_ip_part(idns11, DNS1_VAL1);
    check_ip_part(idns12, DNS1_VAL2);
    check_ip_part(idns13, DNS1_VAL3);
    check_ip_part(idns14, DNS1_VAL4);
  }
  //flog("iip: %i.%i.%i.%i\n", iip1, iip2, iip3, iip4);
  //flog("inm: %i.%i.%i.%i\n", inm1, inm2, inm3, inm4);
  //flog("igw: %i.%i.%i.%i\n", igw1, igw2, igw3, igw4);
  if (iip1 || iip2 || iip3 || iip4) {
    if ((iip1<=255) && (iip2<=255) && (iip3<=255) && (iip4<=255)) {
      net_page.ip_valid = true;
    }
    check_ip_part(iip1, IP_ADDR_VAL1);
    check_ip_part(iip2, IP_ADDR_VAL2);
    check_ip_part(iip3, IP_ADDR_VAL3);
    check_ip_part(iip4, IP_ADDR_VAL4);
  }
  if ((inm1<=255) && (inm2<=255) && (inm3<=255) && (inm4<=255)) {
    net_page.nm_valid = true;
  }
  check_ip_part(inm1, NETMASK_VAL1);
  check_ip_part(inm2, NETMASK_VAL2);
  check_ip_part(inm3, NETMASK_VAL3);
  check_ip_part(inm4, NETMASK_VAL4);
  if (igw1 || igw2 || igw3 || igw4) {
    if ((igw1<=255) && (igw2<=255) && (igw3<=255) && (igw4<=255)) {
      net_page.gw_valid = true;
    }
    check_ip_part(igw1, NETMASK_VAL1);
    check_ip_part(igw2, NETMASK_VAL2);
    check_ip_part(igw3, NETMASK_VAL3);
    check_ip_part(igw4, NETMASK_VAL4);
  }
  return (net_page.ip_valid && net_page.nm_valid && net_page.gw_valid && net_page.dns1_valid);
#endif
  sprintf(pages_params.ip, "%i.%i.%i.%i", iip1, iip2, iip3, iip4);
  sprintf(pages_params.nm, "%i.%i.%i.%i", inm1, inm2, inm3, inm4);
  sprintf(pages_params.gw, "%i.%i.%i.%i", igw1, igw2, igw3, igw4);
  sprintf(pages_params.dns1, "%i.%i.%i.%i", idns11, idns12, idns13, idns14);
  return true;
}

void
set_auto_dns(bool automatic) {
  int first = DNS1_VAL1;
  int last = DNS1_VAL4;
  for (;first<=last;first++) {
    if (automatic) {
      set_field_back(net_page.fields[first], PAGE_COLOR);
      set_field_fore(net_page.fields[first], PAGE_COLOR);
      field_opts_off(net_page.fields[first], O_ACTIVE);
    } else {
      set_field_back(net_page.fields[first], BG_COLOR);
      set_field_fore(net_page.fields[first], BG_COLOR);
      field_opts_on(net_page.fields[first], O_ACTIVE);
    }
    first++;
  }
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

  set_field_type(net_page.fields[IP_ADDR_VAL1], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[IP_ADDR_VAL2], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[IP_ADDR_VAL3], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[IP_ADDR_VAL4], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[NETMASK_VAL1], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[NETMASK_VAL2], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[NETMASK_VAL3], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[NETMASK_VAL4], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[GATEWAY_VAL1], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[GATEWAY_VAL2], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[GATEWAY_VAL3], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[GATEWAY_VAL4], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[DNS1_VAL1], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[DNS1_VAL2], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[DNS1_VAL3], TYPE_INTEGER, 3, 0, 255);
  set_field_type(net_page.fields[DNS1_VAL4], TYPE_INTEGER, 3, 0, 255);



  int ret = get_ip_addr(net_page.ip_val, net_page.netmask_val);
  if (ret != 0) {
    set_ip_fields(IP_ADDR_VAL1, 0, 0, 0, 0);
    set_ip_fields(NETMASK_VAL1, 0, 0, 0, 0);
    set_ip_fields(GATEWAY_VAL1, 0, 0, 0, 0);
  } else {
    int v1, v2, v3, v4;
    sscanf(net_page.ip_val, "%i.%i.%i.%i", &v1, &v2, &v3, &v4);
    flog("IP     : %i.%i.%i.%i\n", v1, v2, v3, v4);
    set_ip_fields(IP_ADDR_VAL1, v1, v2, v3, v4);
    sscanf(net_page.netmask_val, "%i.%i.%i.%i", &v1, &v2, &v3, &v4);
    flog("NETMASK: %i.%i.%i.%i\n", v1, v2, v3, v4);
    set_ip_fields(NETMASK_VAL1, v1, v2, v3, v4);

    ret = get_gw(net_page.gw_val);
    if (ret != 0) {
      set_ip_fields(GATEWAY_VAL1, 0, 0, 0, 0);
    } else {
      sscanf(net_page.gw_val, "%i.%i.%i.%i", &v1, &v2, &v3, &v4);
      flog("GATEWAY: %i.%i.%i.%i\n", v1, v2, v3, v4);
      set_ip_fields(GATEWAY_VAL1, v1, v2, v3, v4);
    }
  }

  res_init();
  flog("nscount: %i\n", _res.nscount);
  if (_res.nscount>0) {
    int v1, v2, v3, v4;
    char *dnsaddr1 = inet_ntoa(_res.nsaddr_list[0].sin_addr);
    memcpy(net_page.dns1_val, dnsaddr1, strlen(dnsaddr1));
    flog("dns1: %s\n", dnsaddr1);
    sscanf(net_page.dns1_val, "%i.%i.%i.%i", &v1, &v2, &v3, &v4);
    set_ip_fields(DNS1_VAL1, v1, v2, v3, v4);
  } else {
    memset(net_page.dns1_val, 0, NI_MAXHOST+1);
    set_ip_fields(DNS1_VAL1, 0, 0, 0, 0);
  }


  //set_field_buffer(net_page.fields[CUR_IP_VAL], 0, net_page.ip_val);

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

  net_page.ip_auto = true;
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
        if (f == net_page.fields[CONFIGURE_BTN]) {

          if (!net_page.ip_auto) {
            validate_ip_params();
            flog("Set start ifup flag\n");
            pages_params.start = START_IFUP;
          } else {
            flog("Set start dhcp flag\n");
            pages_params.start = START_DHCP;
          }
        } else if (f == net_page.fields[IP_SETTINGS_SPN]) {
          spinner_spin(f);
          net_page.ip_auto = !net_page.ip_auto;
          set_auto_ip(net_page.ip_auto);
          set_auto_dns(net_page.ip_auto);
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
