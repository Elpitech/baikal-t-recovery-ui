#include <unistd.h>
#include <sys/reboot.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>

#include "common.h"
#include "top_menu.h"
#include "main_page.h"
#include "boot_page.h"
#include "net_page.h"
#include "recovery_page.h"
#include "pages.h"
#include "fru.h"

#define TAG "MAIN"

FILE *logfile;
struct pages pages_params;
struct fru fru_back;

void win_show(WINDOW *win, wchar_t *label, int label_color);
void print_in_middle(WINDOW *win, int starty, int startx, int width, wchar_t *string, chtype color);

void
store(void) {
  int ret;
  FILE *f;
  f = fopen(STATE_PATH, "w");
  if (f == NULL) {
    return;
  }
  ret = top_menu_store(f);
  if (ret < 0) {
    return;
  }
  ret = recovery_page_store(f);
  if (ret < 0) {
    return;
  }
  fclose(f);
}

int
load(void) {
  int ret;
  FILE *f;
  struct stat st;
  ret = stat(STATE_PATH, &st);
  if (ret < 0) {
    log("Failed to stat %s: %i, errno: %s\n", STATE_PATH, ret, strerror(errno));
    return -1;
  }
  f = fopen(STATE_PATH, "r");
  if (f == NULL) {
    return -2;
  }
  ret = top_menu_load(f);
  if (ret != 0) {
    fclose(f);
    return -3;
  }
  ret = recovery_page_load(f);
  if (ret != 0) {
    fclose(f);
    return -4;
  }
  return 0;
}


int main(void) {
  int ch;
  int esc = 0;
  bool update_eeprom = false;
  //struct timeval tv = { 0L, 500000L };
  //fd_set fds;

  logfile = fopen("/var/log/recovery-ui.log", "w");
  log("Started log\n");
  log("ttyname: %s\n", ttyname(0));
  
  //setlocale(LC_ALL, "ru_RU.UTF-8");
  log("Parse FRU\n");
  if (fru_open_parse()) {
    sleep(1);
    fru_open_parse();
  }
	initscr();			/* Start curses mode 		*/
  log("Start color\n");
  start_color();
	raw();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */
  cbreak();
  //nodelay(stdscr, TRUE);
  timeout(500);
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  init_pair(2, COLOR_BLUE, COLOR_WHITE);
  init_pair(3, COLOR_WHITE, COLOR_BLUE);
  init_pair(4, COLOR_BLACK, COLOR_BLACK);
  init_pair(5, COLOR_GREEN, COLOR_WHITE);
  init_pair(6, COLOR_RED, COLOR_WHITE);
  init_pair(7, COLOR_WHITE, COLOR_RED);
  init_pair(8, COLOR_WHITE, COLOR_BLACK);
  curs_set(0);
  log("Init done\n");
  
  init_main_page();
  init_boot_page();
  init_net_page();
  init_recovery_page();
  init_top_menu(get_main_page_wp(), get_boot_page_wp(), get_net_page_wp(), get_recovery_page_wp());
  if (load()!=0) {
    hide_all_panels_except(get_main_page_wp());
  }
  update_panels();
  pages_params.exclusive = P_NONE;
  pages_params.int_recovery_valid = false;
  pages_params.start = START_NONE;
  pages_params.ext_recovery_valid = false;

	while((esc <= 2) && (pages_params.start == START_NONE)) {
    ch = wgetch(stdscr);
    if (ch != ERR) {
      log("CH: 0x%08x\n", ch);
      if (ch != RKEY_ESC) {
        esc = 0;
      } else {
        //esc ++;
      }
    }
    switch (ch) {
    case RKEY_F10:
      update_eeprom = true;
      esc = 10;
      break;
    case RKEY_F6:
      esc = 10;
      break;
    }
    if (top_menu_process(ch)!=0) {
      continue;
    }
    if (main_page_process(ch)!=0) {
      continue;
    }
    if (net_page_process(ch)!=0) {
      continue;
    }
    if (boot_page_process(ch)!=0) {
      continue;
    }
    if (recovery_page_process(ch)!=0) {
      continue;
    }
    update_panels();
    doupdate();
  }

  store();

  curs_set(1);
  deinit_net_page();
  deinit_boot_page();
  deinit_main_page();
  deinit_top_menu();
	endwin();			/* End curses mode		  */
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  switch (pages_params.start) {
  case START_EXT:
    execl("/bin/ash", "ash", EXT_RECOVERY_PATH, pages_params.ext_recovery_tar_path, pages_params.ext_recovery_mdev, NULL);
    break;
  case START_INT:
    execl("/bin/ash", "ash", INT_RECOVERY_PATH, pages_params.int_recovery_tar_path, pages_params.int_recovery_mdev, NULL);
    break;
  case START_ROM_UP:
    execl("/bin/ash", "ash", ROM_UPDATE_SCRIPT_PATH, pages_params.rom_path, NULL);
    break;
  case START_ROM_DOWN:
    execl("/bin/ash", "ash", ROM_DOWNLOAD_SCRIPT_PATH, pages_params.rom_url, NULL);
    break;
  case START_DHCP:
    execl("/sbin/udhcpc", "udhcpc", NULL);
    break;
  default:
    break;
  }
  if (update_eeprom) {
    int i = 0;
    fru_update_mrec_eeprom();
    printf("Saving ");
    for (;i<10;i++) {
      sleep(1);
      printf(".");
      fflush(stdout);
    }
    sync();
  }
  sync();
  reboot(RB_AUTOBOOT);

	return 0;
}
