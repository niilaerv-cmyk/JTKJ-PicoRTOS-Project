#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <FreeRTOS.h>
#include <task.h>
#include "TKJHAT/sdk.h"
#include <math.h>


#define READ_POSE_TASK_STACK_SIZE 32
#define BUTTON_TASK_STACK_SIZE 512
#define BUFFER_SIZE 256

//globaali muuttuja, johon tallennetaan laitteen asento
int orientation = 3;
//Global Variable where the message is stored
char Message[BUFFER_SIZE] = "";
int len = 0;

//funktioita, joita kutsutaan ReadPose-taskissa
void read_orientation(void);
void update_orientation(void);
void symbolhandler(char symbol);


// taskin toteutusfunktio, jossa kutsutaan kaikkia taskissa tarvittavia funktioita
void read_pose_task(void * pvParameters) {

    while(1) {
        
        read_orientation();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Task that handles detecting button presses and adding symbols to the message. Also detects
// when a message is complete and resets the message buffer.
void button_task(void *pvParameters) {
    (void)pvParameters;
    // Counter for how many spaces pressed in a row.
    int counter = 0;
    // Variables that track if the buttons were pressed last loop.
    bool last_button1_press = false;
    bool last_button2_press = false;
    // Infinite loop checking button presses with 10 ms delay.
    while (1) {
        // Read if buttons are pressed
        bool button1_press = gpio_get(SW1_PIN);
        bool button2_press = gpio_get(SW2_PIN);

        // If button 1 pressed and it was NOT pressed last loop add a symbols to the message
        // based on orientation.
        if (button1_press && !last_button1_press) {
            printf("Button 1 pressed\n");
            counter = 0;
            if (orientation == 1) {
                symbolhandler('.');
            } 
            else if (orientation == 2) {
                symbolhandler('-');
            }
        }

        // If button 2 pressed and it was NOT pressed last loop add a space to the message.
        if (button2_press && !last_button2_press) {
            printf("Button 2 pressed\n");
            symbolhandler(' ');
            counter++;

        }

        // If two spaces have been pressed in a row start sending the message
        // and resets the message buffer and length.
        if (counter == 2) {
            printf("MESSAGE DONE, SENDING...\n");
            //HERE SOMETHING TO START THE MESSAGE SENDING FUNCTION
            Message[0] = '\0';
            len = 0;
            counter = 0;
        }

        // Update variables for last button presses if the buttons were pressed or not.
        last_button1_press = button1_press;
        last_button2_press = button2_press;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


int main(void) {

// picon i/o alustus
    stdio_init_all();
//Initialize TKJ HAT SDK
    init_hat_sdk();
// IMU:n alustus
    init_ICM42670();
// Alustetaan I2C IMU:n dataa varten
    init_i2c_default();
//asetetaan IMU:n näytteenottotaajuudeksi 100Hz ja valitaan mittausalueeksi +-2G, koska meillä kiinnostaa vain missä asennossa pico on.
    ICM42670_StartAccel(
        ICM42670_ACCEL_ODR_100HZ,
        ICM42670_ACCEL_FSR_2G
    );
//Initialize HAT buttons 1 and 2
    init_button1();
    init_button2();


// taski päivittää globaalin muuttujan, "asenot" arvoa 0.5 sekunnin välein 1 = pystyssä, 2 = vaaka, 3 = ei tunnistettavaa asentoa
xTaskCreate(
    read_pose_task,
    "IMU",
    READ_POSE_TASK_STACK_SIZE,
    NULL,
    2,
    NULL
);
// Task handles button presses and updating the message buffer based on current orientation.
xTaskCreate(
    button_task,
    "ButtonTask",
    BUTTON_TASK_STACK_SIZE,
    NULL,
    2,
    NULL
);

//käynnistää taskit
vTaskStartScheduler();

}


void read_orientation(void){
    float ax, ay, az;
    float gx, gy, gz;
    float temp;
 //Funktio odottaa parametreikseen muistiosoitteita 
    ICM42670_read_sensor_data(
        &ax, &ay, &az,
        &gx, &gy, &gz,
        &temp
//tarvitsemme kuitenkin vain arvoja ax, ay ja az, eli kiihtyvyyttä.
);
// tunnistetaan asento
    if (fabs(az)> 0.95f)
    {orientation = 1; }
    else if (fabs(ax)> 0.95f)
    {orientation = 2;}
    else
    {orientation = 3;} // eli asento ei tunnisetettu kummaksikkaan edellä olevista
}   

void update_orientation(void) {
    NULL
}

// Function that stores the symbols in to the global array Message and counts
// the character length of the message
void symbolhandler(char symbol) {
    if (len < (BUFFER_SIZE - 1)) {
        Message[len] = symbol;
        len++;
        Message[len] = '\0';
    }
}