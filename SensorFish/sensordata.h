#ifndef _SENSORDATA_H
#define _SENSORDATA_H

struct {
    unsigned long timestamp;
    float temperature;
    struct {
        float x, y, z;
    } accelerometer, gyroscope;
} sensor_readings;

#endif
