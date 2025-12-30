#include <stdio.h>
#include "lcd.h"
#include <string.h>
#include <unistd.h>
#include "bme280.h"


int main(int argc, char **argv){
	const char *paths[] = {PATH TEMP_VALUE, PATH HUMIDITY_VALUE, PATH PRESSURE_VALUE};
        int ret;
        ret = lcd_init();
        if(ret < 0){
                printf("Couldn't initialize LCD\n");
                return ret;
        }

        sleep(2);

        ret = lcd_clear();
        if(ret < 0)
                return ret;

        float val[3];
        char msg[SOME_BYTES];
        while(1){

                for(int i = 0 ; i < 3 ; i++){
                        int ret = read_sysfs_long(paths[i], &val[i]);
                        if(ret)
                                return ret;
                }
                snprintf(msg, sizeof(msg), "T:%4.1fC H:%3.0f%%", val[0], val[1]);
                printf("msg = %s\n", msg);
                ret = lcd_send_message(msg);
                if(ret < 0)
                        return ret;

                lcd_set_cursor(2,1);

                snprintf(msg, sizeof(msg), "P:%6.1f hPa", val[2]);
                      
                printf("msg = %s\n", msg);
                ret = lcd_send_message(msg);
                if(ret < 0)
                        return ret;

                sleep(1);

                lcd_set_cursor(1, 1);
        }
}
