# Easy8266
ESP8266 custom firmware for easier GET and POST requests.

Should work with any ESP board with 512KB+ of flash memory.

## The serial protocol
Default serial port params:
- 115200 baud
- 8 data bits
- no parity
- one stop bit

### Buffers
There are 2 buffers in RAM:
- URL buffer used to store an internet address
- DATA buffer used to store POST data and configuration

### Load URL command
- Send byte 0x01
- Send an URL string like http://www.example.com/folder/?var1=value1&var2=value2

### Load DATA command
- Send byte 0x02
- Send a DATA string

### POST request
- Load URL like shown in a "Load URL command" section
- Load DATA like shown in a "Load DATA command" section
- Send byte 0x03
- Wait for response

In case of error you will receive:
- String "!ERR" (configurable)
- Http error code
- New line char "\n"

If the response is received successfully you will receive:
- String "!<<<" (configurable)
- 4-byte data length (MSB first) (configurable)
- The content (binary)
- String "!>>>" (configurable)
- CRC16 checksum of the content (configurable)

If you don't need stream start, data length and stream end markers please disable them in a "Params" section of the source file.

POST example in C:
```cpp
// API path
const char *url = "http://myserver/api/";

// Create request
char data[200];
sprintf_s(data, sizeof(data) - 1, "temperature=%d", temperature);

// Send
fprintf(com1, "\001%s\002%s\003", url, data);

```

### GET request
- Load URL like shown in a "Load URL command" section
- Send byte 0x04
- Wait for response

In case of error you will receive:
- String "!ERR" (configurable)
- Http error code
- New line char "\n"

If the response is received successfully you will receive:
- String "!<<<" (configurable)
- 4-byte data length (MSB first) (configurable)
- The content (binary)
- String "!>>>" (configurable)
- CRC16 checksum of the content (configurable)

If you don't need stream start, data length and stream end markers please disable them in a "Params" section of the source file.

GET example in C:
```cpp
// API path
const char *url = "http://myserver/api/";

// Create request
char data[200];
sprintf_s(data, sizeof(data) - 1, "temperature=%d", temperature);

// Send
fprintf(com1, "\001%s?%s\004", url, data);

```

### Configure command
- Send byte 0x02
- Send an ACCESS POINT 1 name (max 64 chars)
- Send byte 0x09 (tab)
- Send an ACCESS POINT 1 password (max 64 chars)
- Send byte 0x0D (carriage-return)
- Repeat steps 2-5 for all the access points
- Send byte 0x06

The configuration will be stored in a flash memory. After reset it will be reloaded so you don't need to send access point params on each reset.

Configuration example in C:
```cpp
// Main AP name and password
const char *name1 = "Keenetic-4367";
const char *pass1 = "#@%V$$%346o96";

// Battery-backed AP name and password
const char *name2 = "iPhone";
const char *pass2 = "uv46368$69q498689";

// Send
fprintf(com1, "\002%s\t%s\r%s\t%s\006", name1, pass1, name2, pass2);

```

### Check the connection status
- Send byte 0x07

If connected you will receive "+", and "-" otherwise.

## Compiling
Please use Arduino IDE with ESP8266 libraries installed.
After building a program select Sketch -> Export compiled Binary to create a binary file.

## Flashing ESP8266
- Connect ESP8266 RXD, TXD, GND pins to a computer using RS232 interface converter
- Connect GPIO0 pin to GND
- Reset ESP8266
- Using a third-party ESP8266 Flasher program write a binary file
- Disconnect GPIO0 pin from GND and reset the ESP8266

Please note that ESP8266's VCC and input voltage must not exceed 3.3V.

Now you can easily download files and send POST requests via almost any microcontroller.
