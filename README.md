# ESP32_S8
Evaluating the CO2-Sensor S8 from SenseAir.  

Code for Arduino with some routines to read all register.  

# Technical information:  
There is no standard library aviable now - since the functionality / documentation is poor.
  # The flow is very simple:  
  Send a request like {0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0XC5} and  
  wait for the response like {0xFE, 0X04, 0X02, 0X03, 0X88, 0XD5, 0XC5}.  
Where:  
| byte | example | description                                                     |
|-----|---------|----------------------------------------------------------------------|
|  0   |  0xFE    | any Address  
|  1   |  0x04    | function code  (03 read hold reg / 04 read inp. register / 06 write)  
|  2, 3 |  0x00 0x03 | starting Address  
|  4, 5 |  0x00 0x01 | lenght of requested register  
|  6, 7 |  0xD5 0xC5 | CRC low / high

