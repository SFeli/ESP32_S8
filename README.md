# ESP32_S8
Evaluating the CO2-Sensor S8 from SenseAir.  https://senseair.com/products/size-counts/s8-5/  

Code for Arduino (.ino) with some routines to read all register and another code for esp-idf in C. (.c)

# Technical information:  
There is no standard library aviable now - since the functionality / documentation is poor.
  # The flow is very simple:  
  Send a request like {0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0XC5} and  
  wait for the response like {0xFE, 0X04, 0X02, 0X03, 0X88, 0XD5, 0XC5}.  
Where:  
| byte | example | description on request                     |   
|-----|---------|----------------------------------------------------------------------|
|  0   |  0xFE    | any Address  
|  1   |  0x04    | function code  (03 read hold reg / 04 read inp. register / 06 write) 
|  2, 3 |  0x00 0x03 | starting Address  
|  4, 5 |  0x00 0x01 | lenght of requested register  
|  6, 7 |  0xD5 0xC5 | CRC low / high

In the response the frist two byte are the same as in the request 0xFE and 0x04.  
The requested value is in byte 3 and 4 (0x03) and (0x88) with low-byte first. so 88 * 265 + 03.
.. followed by two byte for CRC.
