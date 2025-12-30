#include <stdio.h>
#include "bme280.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int bme280_init(int calib_temp, int calib_hum, int calib_press){

        int ret;
        int calibs[] = {calib_temp, calib_hum, calib_press};

        const char *paths[] = {PATH TEMP_CALIBRATION, PATH HUMIDITY_CALIBRATION, PATH PRESSURE_CALIBRATION};
        for(int i = 0; i < 3 ; i++){
                ret = write_sysfs_bme(paths[i], calibs[i]);
                if(ret < 0){
                        printf("couldn't set %s\n", paths[i]);
                        return ret;
                }
        }

        return 0;
}
int read_sysfs_long(const char *path, float *val){

        int fd = open(path, O_RDONLY | O_SYNC);

        if(fd < 0){
                printf("Error while opening %s file \n", path);
                return -errno;
        }
        char buf[SOME_BYTES];
        int ret = read(fd, buf, SOME_BYTES);
        if(ret < 0){
                printf("Error while reading from %s file\n", path);
                return -errno;
        }

        buf[ret] = '\0';
        if(memcmp(path, PATH PRESSURE_VALUE, strlen(path)) != 0){
                long tmp = strtol(buf, NULL, 10);
                *val = tmp/1000.0;
        }else {
                float tmp = strtof(buf, NULL);
                *val = tmp*10.0;
        }

        close(fd);

        return 0;
}

int write_sysfs_bme(const char *path, int calib){

        int fd = open(path, O_WRONLY | O_SYNC);

        if(fd < 0){
                printf("Error while opening %s file \n", path);
                return -errno;
        }
        char buf[SOME_BYTES];

        int len = snprintf(buf, 2, "%d", calib);
        if(len < 0)
                return -EINVAL;

        int ret = write(fd, buf, len);
        if(ret < 0){
                printf("Error while writing to %s file \n", path);
                return -errno;
        }
        close(fd);

        return 0;
}
