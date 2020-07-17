/* For Senseair S8 004-0-0053
Vorlage : https://github.com/liutyi/esp32-oled-senseair/blob/787e3901e96e5c13aa463619f402b9873a8df80c/wemos-simple_co2meter.ino
Vorlage : https://github.com/Alexey-Tsarev/SensorsBox/blob/0b3699d5d33254caaf5594c4bb6daf278c9c700c/src/SensorsBox-firmware/SenseAirS8.cpp
Vorlage: https://github.com/funnybrum/AQMv2/blob/master/src/S8.h    <- send and validate
//   Board = esp32dev
//   Anschlüsse Pin 21 und Pin 22
//   Kommunikationsformat mit SenseAir
//   byte Request[] = {0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0XC5};      lt. Doc von SenseAir TDE2067
//   0     FE    any Address
//   1     04    function code  (03 read hold reg / 04 read inp. register / 06 write)
//   2,3   00 03 starting Address 
//   4,5   00 01 lenght of requested register
//   6,7   D5 C5 CRC low / high
//   Response = {0xFE, 0X04, 0X02, 0X03, 0X88, 0XD5, 0XC5};
//   0     FE    same Address as in Request
//   1     04    same funcation as in Request
//   2,3   02 03 Value of response (eg. CO-Value) (depending on requested registers)
//   5,6   D5 C5 CRC Low / high
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/uart.h"            // UART - Communication
#include "string.h"

#define TXD2 21
#define RXD2 22
#define RTS2 -1
#define CTS2 -1

#define BUF_SIZE (1024)
unsigned long int ReadCRC, DataCRC;     // Control-ReturnCodes to compare Readed against Calculated

char CO2req[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};   // Request CO2-Value
char ABCreq[] = {0xFE, 0X03, 0X00, 0X1F, 0X00, 0X01, 0XA1, 0XC3};   // Request ABC-Interval in [d] 1f in Hex -> 31 dezimal
char FWreq[] = {0xFE, 0x04, 0x00, 0x1C, 0x00, 0x01, 0xE4, 0x03};    // FirmWare     
char ID_Hi[] = {0xFE, 0x04, 0x00, 0x1D, 0x00, 0x01, 0xB5, 0xC3};    // Sensor ID hi    
char ID_Lo[] = {0xFE, 0x04, 0x00, 0x1E, 0x00, 0x01, 0x45, 0xC3};    // Sensor ID lo    // in Hex 071dbfe4

void init()
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);            // 1. setting communication parameter
    uart_set_pin(UART_NUM_1, TXD2, RXD2, RTS2, CTS2);       // 2. setting communication pins
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);   // 3. driver installation
}

int send_Req(const char* data, int Req_len)
{
    const int txBytes = uart_write_bytes(UART_NUM_1, data, Req_len);
    return txBytes;
}

unsigned short int ModBus_CRC(unsigned char * buf, int len)
{
  unsigned short int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned short int)buf[pos];   // XOR byte into least sig. byte of crc
    for (int i = 8; i != 0; i--) {         // Loop over each bit
      if ((crc & 0x0001) != 0) {           // If the LSB is set
        crc >>= 1;                         // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;  
}  

static void get_info()
{
    uint8_t* vResponse = (uint8_t*) malloc(BUF_SIZE+1);
    send_Req(ABCreq, 8);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    int rxBytes = uart_read_bytes(UART_NUM_1, vResponse, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    ReadCRC = (uint16_t)vResponse[6] * 256 + (uint16_t)vResponse[5];
    DataCRC = ModBus_CRC(vResponse, 5);
    if (DataCRC == ReadCRC) {
        ESP_LOGI("Info", "ABC-Period %d ", vResponse[3] * 256 + vResponse[4]);
    }
        else          
    {
        ESP_LOGI("Error", "CRC-Fehler %ld ungleich %ld", ReadCRC, DataCRC);
    }
//  Read Sensor ID - Unique ID of the S8 Sensor like 071dbfe4
    send_Req(ID_Hi, 8);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    rxBytes = uart_read_bytes(UART_NUM_1, vResponse, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    if (DataCRC == ReadCRC) {
        ESP_LOGI("Info", "Sensor-ID %d%d ", vResponse[3] , vResponse[4]);
    }
        else          
    {
        ESP_LOGI("Error", "CRC-Fehler %ld ungleich %ld", ReadCRC, DataCRC);
    }
    send_Req(ID_Lo, 8);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    rxBytes = uart_read_bytes(UART_NUM_1, vResponse, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    ESP_LOGI("Info", "Sensor-ID %d%d ", vResponse[3] , vResponse[4]);
}

static void tx_task()
{
    static const char *TX_TASK_TAG = "Transfer_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        send_Req(CO2req, 8);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task()
{
    static const char *RX_TASK_TAG = "Receive_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
        // Configure a temporary buffer for the incoming data
    uint8_t* vResponse = (uint8_t*) malloc(BUF_SIZE+1);
    while (1) {
        // Read data from the UART
        const int rxBytes = uart_read_bytes(UART_NUM_1, vResponse, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            vResponse[rxBytes] = 0;
            ReadCRC = (uint16_t)vResponse[6] * 256 + (uint16_t)vResponse[5];
            DataCRC = ModBus_CRC(vResponse, 5);
            if (DataCRC == ReadCRC) {
                ESP_LOGI(RX_TASK_TAG, "CO2 %d ", vResponse[3] * 256 + vResponse[4]);
            } else 
            {
                ESP_LOGI(RX_TASK_TAG, "CRC-Fehler %ld ungleich %ld", ReadCRC, DataCRC);
            }
        }
    }
    free(vResponse);
}

void app_main()
{
    init();         // initialize the connection
    get_info();     // get general informations about the sensor
    xTaskCreate(rx_task, "receive_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "transfer_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}
