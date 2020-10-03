#ifndef TEMP_MODEL_H
#define TEMP_MODEL_H

#define NUM_THERMISTOR 20

typedef struct{
    float tm_readings_V[NUM_THERMISTOR];
    float temps_C[NUM_THERMISTOR];
} TempModel_t;

#endif // TEMP_MODEL_H