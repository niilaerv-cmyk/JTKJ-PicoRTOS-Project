#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <FreeRTOS.h>
#include <task.h>
#include "TKJHAT/sdk.h"

//globaali muuttuja, johon tallennetaan laitteen asento
int orientation = 3;

#define READ_POSE_TASK_STACK_SIZE 32


//funktioita, joita kutsutaan ReadPose-taskissa
int read_orientation(void);
void update_orientation(void);

// taskin toteutusfunktio, jossa kutsutaan kaikkia taskissa tarvittavia funktioita
void read_pose_task(void * pvParameters)

    Whlie(1) {
        read_orientation();
        /*tähän vielä muita funktioita
            -
            -
            -
        */

        vTaskDelay(pdMS_TO_TICKS(500));
    }



int main(void) {

// picon i/o alustus
    stdio_init_all();
// IMU:n alustus
    init_ICM42670();


// taski päivittää globaalin muuttujan, "asenot" arvoa 0.5 sekunnin välein 1 = pystyssä, 2 = vaaka, 3 = ei tunnistettavaa asentoa
xTaskCreate(
    read_pose_task,
    "IMU"
    READ_POSE_TASK_STACK_SIZE,
    NULL,
    1,
    NULL
);
//käynnistää taskit
vTaskStartScheduler();
    
}


int read_orientation(void){
    NULL

}   

void update_orientation(void) {
    NULL
}