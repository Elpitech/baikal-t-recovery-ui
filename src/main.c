#include <unistd.h>
#include <sys/reboot.h>
#include <sys/select.h>

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


int main(void) {
  int ch;
  int esc = 0;
  bool update_eeprom = false;
  //struct timeval tv = { 0L, 500000L };
  //fd_set fds;

  logfile = fopen("/tmp/recovery-ui.log", "w");
  log("Started log\n");
  
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
  curs_set(0);
  log("Init done\n");
  
  init_main_page();
  init_boot_page();
  init_net_page();
  init_recovery_page();
  init_top_menu(get_main_page_wp(), get_boot_page_wp(), get_net_page_wp(), get_recovery_page_wp());
  hide_all_panels_except(get_main_page_wp());
  update_panels();
  pages_params.exclusive = P_NONE;
  pages_params.recovery_valid = false;
  pages_params.start_recovery = false;

	while((esc <= 2) && (!pages_params.start_recovery)) {
    ch = wgetch(stdscr);
    if (ch != ERR) {
      log("CH: 0x%08x\n", ch);
      if (ch != RKEY_ESC) {
        esc = 0;
      } else {
        //esc ++;
      }
    }
    //if (pages_params.exclusive == P_NONE) {
      switch (ch) {
      case RKEY_F10:
        update_eeprom = true;
        esc = 10;
        break;
      case RKEY_F6:
        //fru_update_mrec_eeprom();
        esc = 10;
        break;
      }
      //}

      //if (pages_params.exclusive == P_NONE) {
      if (top_menu_process(ch)!=0) {
        continue;
      }
      //}
      //if ((pages_params.exclusive == P_NONE) || (pages_params.exclusive == P_MAIN)) {
      if (main_page_process(ch)!=0) {
        continue;
      }
      //}
      //if ((pages_params.exclusive == P_NONE) || (pages_params.exclusive == P_NET)) {
      if (net_page_process(ch)!=0) {
        continue;
      }
      //}
      //if ((pages_params.exclusive == P_NONE) || (pages_params.exclusive == P_BOOT)) {
      if (boot_page_process(ch)!=0) {
        continue;
      }
      //}
      //if ((pages_params.exclusive == P_NONE) || (pages_params.exclusive == P_RECOVERY)) {
      if (recovery_page_process(ch)!=0) {
        continue;
      }
      //}

    //refresh();
    //wrefresh(windows[0]);
    update_panels();
    doupdate();
  }

  curs_set(1);
  deinit_net_page();
  deinit_boot_page();
  deinit_main_page();
  deinit_top_menu();
	endwin();			/* End curses mode		  */
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  if (pages_params.start_recovery) {
    execl("/bin/ash", "ash", pages_params.recovery, NULL);
  } else if (update_eeprom) {
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
