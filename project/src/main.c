#include <stdio.h>
#include <TKJHAT/sdk.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"

//funktioita, joita kutsutaan ReadPose -Funktiossa
int read_orientation(void);



int main(void) {

// picon i/o alustus
    stdio_init_all();
// IMU:n alustus
    init_ICM42670();

//
}

int read_orientation(void){
    NULL
}


    