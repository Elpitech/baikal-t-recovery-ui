#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <wchar.h>
#include "common.h"

#define SHRED_GPIO_BASE 32
#define SHRED_NGPIO 8
#define TAG "MAIN"

WINDOW *windows[3];
PANEL *panels[3];
FILE *logf;

uint32_t read_shred(void) {
#ifdef REAL_DEVICES
  uint32_t val = 0;
  int ret = 0;
  uint32_t shred_val = 0;
  int i = 0;
  uint8_t buf[64] = {0};
  FILE *gpio = NULL;
  FILE *export = fopen("/sys/class/gpio/export", "w");
  for (i=0;i<SHRED_NGPIO;i++) {
    ret = sprintf(buf, "%i\n", i+SHRED_GPIO_BASE);
    fwrite(buf, sizeof(char), ret, export);
    usleep(100);
    sprintf(buf, "/sys/class/gpio/gpio%i", i+SHRED_GPIO_BASE);
    gpio = fopen(buf, "r");
    ret = fscanf(gpio, "%i", &val);
    log("gpio[%i] read returned %i, value: %i\n", i+SHRED_GPIO_BASE, ret, val);
    fclose(gpio);
  }
  fclose(export);
#else
  return FAKE_ID;
#endif
}

int main(void) {
  PANEL *top;
  int ch;
  int i = 0;
  uint32_t shred;
  logf = fopen("/tmp/recovery-ui.log", "w");
  log("Started log\n");
  
  shred = read_shred();
  log("shred: 0x%08x\n", shred);
  setlocale(LC_ALL, "ru_RU.UTF-8");

	initscr();			/* Start curses mode 		*/
	raw();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */

  int max_x = COLS;
  int max_y = LINES;
  //getmaxyx(NULL, max_y, max_x);
  printf("max_x: %i, max_y: %i\n", max_x, max_y);

  wchar_t label[80];
  for (i=0; i<3; i++) {
    windows[i] = newwin(LINES, COLS, 0, 0);
    box(windows[i], 0, 0);
    panels[i] = new_panel(windows[i]);
    swprintf(label, 80, L"Окно %i", i);
    win_show(windows[i], label, i+1);
  }
  update_panels();
  doupdate();

	while((ch = getch()) != KEY_F(1)) {
    switch (ch) {
    case 9:
      top = (PANEL *)panel_userptr(top);
      top_panel(top);
      break;
    }
    update_panels();
    doupdate();
    
  }
  
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

void print_in_middle(WINDOW *win, int starty, int startx, int width, wchar_t *string, chtype color)
{	int length, x, y;
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
  //wchar_t str[80] = L"Окноооо";
  mvwins_wstr(win, y, x, string);
	//mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}
