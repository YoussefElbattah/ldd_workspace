#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "lcd.h"

int lcd_init(void){

        int ret;
        ret = lcd_return_home();
        if (ret < 0) return ret;
        usleep(2000);

        ret = lcd_clear();
        if(ret < 0){
                printf("Couldn't clear the screen\n");
                return ret;
        }
        usleep(2000);

        char msg[SOME_BYTES];
        snprintf(msg, SOME_BYTES, "Wheater station");
        ret = lcd_send_message(msg);
        if(ret < 0){
                printf("Couldn't send message to LCD\n");
                return ret;
        }

        ret = lcd_set_cursor(2, 1);
        if(ret < 0){
                printf("Couldn't set cursor of LCD\n");
                return ret;
        }

        snprintf(msg, SOME_BYTES, "Initializing...");
        ret = lcd_send_message(msg);
        if(ret < 0){
                printf("Couldn't send message to LCD\n");
                return ret;
        }

        return 0;
}

int lcd_send_cmd(const char *cmd)
{
        int fd = open(PATH LCD_CMD, O_WRONLY | O_SYNC);

        if(fd < 0){
                printf("Error while opening %s file \n", LCD_CMD);
                return -errno;
        }

        int count = write(fd, cmd, strlen(cmd));
        if(count < 0){
                printf("Error while writing to %s file\n", LCD_CMD);
                return -errno;
        }

        close(fd);
        return count;
}

int lcd_clear(void){
        return lcd_send_cmd(LCD_CMD_DIS_CLEAR);
}

int lcd_return_home(void){
        return lcd_send_cmd(LCD_CMD_DIS_RETURN_HOME);
}
int lcd_scroll(uint8_t direction){
        char *dir = (direction == RIGHT_SCROLL) ? "right" : "left";

        int fd = open(PATH LCD_SCROLL, O_WRONLY | O_SYNC);

        if(fd < 0){
                printf("Error while opening %s file \n", LCD_SCROLL);
                return -errno;
        }

        int count = write(fd, dir, strlen(dir));
        if(count < 0){
                printf("Error while writing to %s file\n", LCD_SCROLL);
                return -errno;
        }

        close(fd);

        return count;
}

int lcd_send_message(const char *message)
{
        int fd = open(PATH LCD_TEXT, O_WRONLY | O_SYNC);

        if(fd < 0){
                printf("Error while opening %s file \n", LCD_TEXT);
                return -errno;
        }
        int count = write(fd, message, strlen(message));
        if(count < 0){
                printf("Error while writing to %s file\n", LCD_TEXT);
                return -errno;
        }

        close(fd);

        return count;
}

int lcd_set_cursor(uint8_t row, uint8_t col)
{
        int fd = open(PATH LCD_CURSOR, O_WRONLY | O_SYNC);

        if(fd < 0){
                printf("Error while opening %s file \n", LCD_CURSOR);
                return -errno;
        }
        char cursor[SOME_BYTES];
        snprintf(cursor, 5, "%d,%d\n", row, col);
        int count = write(fd, cursor, strlen(cursor));
        if(count < 0){
                printf("Error while writing to %s file\n", LCD_CURSOR);
                return -errno;
        }

        close(fd);

        return count;
}
int lcd_get_cursor(uint8_t *row, uint8_t *col)
{
        int fd = open(PATH LCD_CURSOR, O_RDONLY | O_SYNC);

        if(fd < 0){
                printf("Error while opening %s file \n", LCD_CURSOR);
                return errno;
        }
        char cursor[SOME_BYTES];

        int count = read(fd, cursor, SOME_BYTES);
        if(count < 0){
                printf("Error while writing to %s file\n", LCD_CURSOR);
                return -errno;
        }

        char *comma;
        comma = strchr(cursor, ',');
        if(comma == NULL){
                printf("No comma detected \n");
                return -EINVAL;
        }

        *row = strtol(cursor + 1, NULL, 10);
        *col = strtol(comma + 1, NULL , 10);
        
        close(fd);

        return count;
}

