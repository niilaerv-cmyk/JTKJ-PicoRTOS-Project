#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <FreeRTOS.h>
#include <task.h>
#include <tkjhat/sdk.h>
#include <math.h>
#include <queue.h> //added for queue functionality
#include <string.h> 

#define READ_POSE_TASK_STACK_SIZE 2048
#define BUTTON_TASK_STACK_SIZE 512
#define BUFFER_SIZE 256
#define MESSAGE_QUEUE_LENGTH 3 //Queue for transmitting messages to the serial task, 
//and the maximum number of messages that can be queued at once is 3.

// Globaali muuttuja, johon tallennetaan laitteen asento
int orientation = 3;
// Global Variable where the message is stored
char Message[BUFFER_SIZE];
// Tracks the length of the message.
int len = 0;
// Counter for how many spaces have been pressed in a row.
int counter = 0;
// Queue handle for transmitting completed messages from the button task to the serial task.
QueueHandle_t messageQueue;

//funktioita, joita kutsutaan ReadPose-taskissa
void read_orientation(void);
void symbolhandler(char symbol);


// taskin toteutusfunktio, jossa kutsutaan kaikkia taskissa tarvittavia funktioita
void read_pose_task(void * pvParameters) {

    while(1) {

        if (ICM42670_start_with_default_values() != 0){
            printf("ICM-42670P could not initialize accelerometer or gyroscope\n");
        }
        read_orientation();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Task that handles detecting button presses and adding symbols to the message. Also detects
// when a message is complete and resets the message buffer.
void button_task(void *pvParameters) {
    (void)pvParameters;
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

       // If two spaces have been pressed in a row, send the message to the queue.
if (counter == 2) {
    // Remove trailing spaces
    while (len > 0 && Message[len - 1] == ' ') {
        len--;
    }
    Message[len] = '\0';

    // Only send a non-empty message
    if (len > 0) {
        char completedMessage[BUFFER_SIZE] = {0};

        // Copy the completed message
        memcpy(completedMessage, Message, len + 1);

        // Send the message to the Queue
        if (xQueueSend(messageQueue,
                       completedMessage,
                       portMAX_DELAY) == pdPASS) {

            printf("Message sent to Queue: %s\n", completedMessage);

            // Reset the original message buffer
            Message[0] = '\0';
            len = 0;
        }
    }
        else {
        // Reset an empty message
        Message[0] = '\0';
        len = 0;
    }
    counter = 0;
}
        // Update variables for last button presses if the buttons were pressed or not.
        last_button1_press = button1_press;
        last_button2_press = button2_press;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// SerialTask receives completed messages from ButtonTask and sends them to the workstation via USB.
static void serial_task(void *arg) {
    (void)arg;
    // Buffer for SerialTask itself to hold received messages from the Queue.
    char receivedMessage[BUFFER_SIZE];


    while (1) {
        // Wait for a message from the Queue. the message will be stored in buffer receivedMessage. 
        // SerialTask will wait until a message is available in the Queue by ButtonTask.
        // The maximum time to wait is portMAX_DELAY, which means it will wait indefinitely.
        if (xQueueReceive(messageQueue, receivedMessage, portMAX_DELAY) == pdPASS) {
            // Send the received message from buffer to the workstation via USB
            printf("%s\n", receivedMessage);
        }
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
    ICM42670_startAccel(
        ICM42670_ACCEL_ODR_100HZ,
        ICM42670_ACCEL_FSR_2G
    );
//Initialize HAT buttons 1 and 2
    init_button1();
    init_button2();
//Creat the message Queue
messageQueue = xQueueCreate(MESSAGE_QUEUE_LENGTH, BUFFER_SIZE);
// Check if the Queue was created successfully
if (messageQueue == NULL) {
    printf("Failed to create message queue!\n");
    return 1; // Exit the program with an error code
}    

TaskHandle_t hSerialTask;

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

// Create SerialTask

    BaseType_t result = xTaskCreate(serial_task,  // (en) Task function
                "Serial",              // (en) Name of the task 
                1024,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hSerialTask);         // (en) A handle to control the execution of this task
    
    // Check whether task creation succeeded
    if (result != pdPASS) {
        printf("Failed to create SerialTask!\n");
    }

//käynnistää taskit
vTaskStartScheduler();
return 0;
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

// Function that stores the symbols in to the global array Message and counts
// the character length of the message. Avoids buffer overflow by stopping when
// length equals BUFFER_SIZE - 1. Last position is reserved for \0.
void symbolhandler(char symbol) {
    if (len < (BUFFER_SIZE - 1)) {
        Message[len] = symbol;
        len++;
        Message[len] = '\0';
    }
}
