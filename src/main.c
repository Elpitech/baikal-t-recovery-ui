#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <menu.h>
#include "common.h"

#define SHRED_GPIO_BASE 32
#define SHRED_NGPIO 8
#define TAG "MAIN"

WINDOW *windows[3];
PANEL *panels[3];
FILE *logfile;

void win_show(WINDOW *win, char *label, int label_color);
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
  MENU *my_menu;
  int ch;
  int i = 0;
  uint32_t shred;
  int esc = 0;
  logfile = fopen("/tmp/recovery-ui.log", "w");
  log("Started log\n");
  
  shred = read_shred();
  log("shred: 0x%08x\n", shred);
  setlocale(LC_ALL, "ru_RU.UTF-8");

	initscr();			/* Start curses mode 		*/
  log("Start color\n");
  start_color();
	raw();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */
  cbreak();
  init_pair(1, COLOR_CYAN, COLOR_BLACK);

  int max_x = COLS;
  int max_y = LINES;
  //getmaxyx(NULL, max_y, max_x);
  //printf("max_x: %i, max_y: %i\n", max_x, max_y);

  ITEM *it1 = new_item("it11", "it12");
  ITEM *it2 = new_item("it21", "it22");
  ITEM *it3 = new_item("it31", "it32");
  ITEM *my_items[] = {it1, it2, it3};
  my_menu = new_menu((ITEM **)my_items);


  wchar_t label[80];
  windows[0] = newwin(LINES, COLS, 0, 0);

  set_menu_win(my_menu, windows[0]);
  set_menu_sub(my_menu, derwin(windows[0], 6, 68, 3, 1));
	set_menu_format(my_menu, 5, 3);
	set_menu_mark(my_menu, " * ");

  box(windows[0], 0, 0);
  panels[0] = new_panel(windows[0]);
  swprintf(label, 80, L"SETTINGS");
  win_show(windows[0], label, 1);
  
  post_menu(my_menu);
	wrefresh(windows[0]);
  update_panels();
    
  doupdate();
	while(esc <= 2) {
    ch = getch();
    if (ch != 0x1b) {
      esc = 0;
    }
    switch (ch) {
    case 0x1b: // escape
      esc ++;
      break;
    case 9:
      top = (PANEL *)panel_userptr(top);
      top_panel(top);
      break;
    case KEY_DOWN:
      menu_driver(my_menu, REQ_DOWN_ITEM);
      break;
    case KEY_UP:
      menu_driver(my_menu, REQ_UP_ITEM);
      break;
    case KEY_LEFT:
      menu_driver(my_menu, REQ_LEFT_ITEM);
      break;
    case KEY_RIGHT:
      menu_driver(my_menu, REQ_RIGHT_ITEM);
      break;
    case KEY_NPAGE:
      menu_driver(my_menu, REQ_SCR_DPAGE);
      break;
    case KEY_PPAGE:
      menu_driver(my_menu, REQ_SCR_UPAGE);
      break;
    }
    wrefresh(windows[0]);
    update_panels();
    doupdate();
    
  }

  unpost_menu(my_menu);
  free_menu(my_menu);
  for(i = 0; i < 3; i++)
    free_item(my_items[i]);
	endwin();			/* End curses mode		  */
	return 0;
}


/* Show the window with a border and a label */
void win_show(WINDOW *win, char *label, int label_color)
{	int startx, starty, height, width;

	getbegyx(win, starty, startx);
	getmaxyx(win, height, width);

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

	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);
  mvwins_wstr(win, y, x, string);
	wattroff(win, color);
	refresh();
}
