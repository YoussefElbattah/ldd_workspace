#ifndef __LCD_H
#define __LCD_H

#include <unistd.h>
#include <stdint.h>

#define LCD_CMD         "lcdcmd"
#define LCD_TEXT        "lcdtext"
#define LCD_CURSOR      "lcdxy"
#define LCD_SCROLL      "lcdscroll"

#define PATH            "/sys/class/bone_lcd/lcd_16x2/"

#define RIGHT_SCROLL    0
#define LEFT_SCROLL     1

#define SOME_BYTES      100

#define LCD_CMD_4DL_2N_5X8F             "0x28"
#define LCD_CMD_DON_CURON               "0x0E"
#define LCD_CMD_INCADD                  "0x06"
#define LCD_CMD_DIS_CLEAR               "0x01"
#define LCD_CMD_DIS_RETURN_HOME         "0x02"

int lcd_init(void);
int lcd_send_cmd(const char *cmd);
int lcd_scroll(uint8_t direction);
int lcd_send_message(const char *message);
int lcd_set_cursor(uint8_t row, uint8_t col);
int lcd_get_cursor(uint8_t *row, uint8_t *col);
int lcd_clear(void);
int lcd_return_home(void);
#endif 

