#include <unistd.h>
#include <sys/reboot.h>

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
#include "pages.h"
#include "fru.h"

#define SHRED_GPIO_BASE 32
#define SHRED_NGPIO 8
#define TAG "MAIN"

FILE *logfile;
struct pages pages_params;
struct fru fru_back;

void win_show(WINDOW *win, wchar_t *label, int label_color);
void print_in_middle(WINDOW *win, int starty, int startx, int width, wchar_t *string, chtype color);

uint32_t read_shred(void) {
#ifdef REAL_DEVICES
  uint32_t val = 0;
  int ret = 0;
  uint32_t shred_val = 0;
  int i = 0;
  uint8_t buf[64] = {0};
  for (i=0;i<SHRED_NGPIO;i++) {
    log("Opening /sys/class/gpio/export\n");
    FILE *export = fopen("/sys/class/gpio/export", "w");
    if (export == NULL) {
      err("Failed to open /sys/class/gpio/export\n");
      return val;
    }
    ret = sprintf(buf, "%i", i+SHRED_GPIO_BASE);
    log("Exporting pin %s[len: %i]\n", buf, ret);
    fwrite(buf, sizeof(char), ret, export);
    fclose(export);
    sprintf(buf, "/sys/class/gpio/gpio%i/value", i+SHRED_GPIO_BASE);
    log("Opening %s\n", buf);
    FILE *gpio = fopen(buf, "r");
    if (gpio == NULL) {
      err("Failed to open %s\n", buf);
      fclose(export);
      return shred_val;
    }
    ret = fscanf(gpio, "%i", &val);
    log("gpio[%i] read returned %i, value: %i\n", i+SHRED_GPIO_BASE, ret, val);
    shred_val |= (val!=0?(1<<i):0);
    fclose(gpio);
  }
  return shred_val;
#else
  return FAKE_SHRED;
#endif
}

int main(void) {
  PANEL *top;
  int ch;
  int i = 0;
  int esc = 0;
  bool update_eeprom = false;
  logfile = fopen("/tmp/recovery-ui.log", "w");
  log("Started log\n");
  
  //setlocale(LC_ALL, "ru_RU.UTF-8");
  log("Parse FRU\n");
  fru_open_parse();
	initscr();			/* Start curses mode 		*/
  log("Start color\n");
  start_color();
	raw();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */
  cbreak();
  nodelay(stdscr, TRUE);
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  init_pair(2, COLOR_BLUE, COLOR_WHITE);
  init_pair(3, COLOR_WHITE, COLOR_BLUE);
  init_pair(4, COLOR_BLACK, COLOR_BLACK);
  init_pair(5, COLOR_GREEN, COLOR_WHITE);
  init_pair(6, COLOR_RED, COLOR_WHITE);
  curs_set(0);
  log("Init done\n");

  //getmaxyx(NULL, max_y, max_x);
  //printf("max_x: %i, max_y: %i\n", max_x, max_y);
  

  /* wchar_t label[80]; */
  /* windows[0] = newwin(LINES, COLS, 0, 0); */


  /* box(windows[0], 0, 0); */
  /* panels[0] = new_panel(windows[0]); */
  /* swprintf(label, 80, L"SETTINGS"); */
  /* win_show(windows[0], label, 1); */
  
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
  //doupdate();
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

  //unpost_menu(my_menu);
  //free_menu(my_menu);
  //for(i = 0; i < 3; i++)
  //free_item(my_items[i]);
  curs_set(1);
  deinit_net_page();
  deinit_boot_page();
  deinit_main_page();
  deinit_top_menu();
	endwin();			/* End curses mode		  */
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  if (pages_params.start_recovery) {
    execl("/bin/ash", "ash", pages_params.recovery);
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
    reboot(RB_AUTOBOOT);
  }

	return 0;
}


/* Show the window with a border and a label */
void win_show(WINDOW *win, wchar_t *label, int label_color) {
  //int startx, starty;
  int height;
  int width;

	//getbegyx(win, starty, startx);
	getmaxyx(win, height, width);
  (void)height;

	box(win, 0, 0);
	mvwaddch(win, 2, 0, ACS_LTEE); 
	mvwhline(win, 2, 1, ACS_HLINE, width - 2); 
	mvwaddch(win, 2, width - 1, ACS_RTEE); 
	
	print_in_middle(win, 1, 0, width, label, COLOR_PAIR(label_color));
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, wchar_t *string, chtype color) {
  int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = wcslen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);
  mvwins_wstr(win, y, x, string);
	wattroff(win, color);
	refresh();
}

void label(WINDOW *win, int starty, int startx, wchar_t *string, int color) {
  wattron(win, color);
  mvwins_wstr(win, starty, startx, string);
	wattroff(win, color);
}
