#include <FS.h>
#include <LittleFS.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

///////////////////////////////////////////////////////////////////////
// Params
///////////////////////////////////////////////////////////////////////

// Access point name max length and password length
#define AP_NAME_LENGTH    64
#define AP_PASS_LENGTH    64

// Number of access points
#define AP_COUNT          2

// Buffer sizes
#define URL_SIZE          2000
#define DATA_SIZE         18000
#define CHUNK_SIZE        128

// Config file name on a flash disk
const char *config_file_name = "/config.txt";

#define STREAM_START      "!<<<"
#define STREAM_END        "!>>>"

const int stream_send_size = 1;
const int stream_send_crc = 1;

///////////////////////////////////////////////////////////////////////
// Const data
///////////////////////////////////////////////////////////////////////
const unsigned short crc_init = 0xFFFFu;

const unsigned short crc16_table[256] = {
  0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
  0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
  0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
  0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
  0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
  0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
  0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
  0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
  0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
  0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
  0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
  0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
  0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
  0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
  0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
  0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
  0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
  0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
  0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
  0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
  0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
  0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
  0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
  0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
  0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
  0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
  0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
  0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
  0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
  0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
  0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
  0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78
};

enum state_t
{
  s_idle, s_loading_url, s_loading_data, s_get, s_post
};



///////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////
ESP8266WiFiMulti WiFiMulti;

char data[DATA_SIZE];
int data_n = 0;

char url[URL_SIZE];
int url_n = 0;

char buf[CHUNK_SIZE];
int buf_n = 0;

state_t state = s_idle;

int connected = 0;


///////////////////////////////////////////////////////////////////////
// CRC functions
///////////////////////////////////////////////////////////////////////

unsigned short crc16_update(unsigned char data, unsigned short crc)
{
  return crc16_table[((crc ^ data) & 255)] ^ ((crc >> 8) & 0xff);
}

unsigned short crc16(const void *data, int len)
{
  unsigned short res = crc_init;
  const unsigned char *p = (const unsigned char *)data;
  while (len--)
  {
    res = crc16_update(*p++, res);
  }
  return res;
}


///////////////////////////////////////////////////////////////////////
// File tools
///////////////////////////////////////////////////////////////////////

size_t load_file(const char *name, char *buffer, size_t buffer_size)
{
  if (name == NULL)
  {
    return 0;  
  }

  if (buffer == NULL)
  {
    return 0;  
  }

  if (buffer_size == 0)
  {
    return 0;  
  }
  
  File file = LittleFS.open(name, "r");
  if (!file)
  {
    return 0;
  }

  size_t count = 0;

  while (count < buffer_size - 1 && file.available())
  {
    buffer[count++] = file.read();  
  }

  buffer[count] = 0;

  file.close();
  
  return count;
}

int is_file_same(const char *name, const char *buffer, int count)
{
  if (name == NULL)
  {
    return 0;
  }
  
  File file = LittleFS.open(name, "r");
  if (!file)
  {
    return 0;
  }

  int same = 1;

  for (int i = 0; i < count; i++)
  {
    if (!file.available())
    {
      same = 0;
      break;
    }

    if (file.read() != buffer[i])
    {
      same = 0;
      break;
    }
  }

  if (same && file.available())
  {
    same = 0;
  }

  file.close();
  
  return same;
}

int save_file(const char *name, const char *buffer, int count)
{
  if (name == NULL)
  {
    return 0;  
  }

  if (buffer == NULL)
  {
    return 0;  
  }

  if (count <= 0)
  {
    return 0;  
  }

  // Do we need to save?
  if (is_file_same(name, buffer, count))
  {
    return 1;
  }
  
  File file = LittleFS.open(name, "w");
  if (!file)
  {
    return 0;
  }

  file.write(buffer, count);

  file.close();
  
  return 1;
}


///////////////////////////////////////////////////////////////////////
// Configuration functions
///////////////////////////////////////////////////////////////////////

const char *extract_value(const char *str, char delim, char *buffer, size_t buffer_size)
{
  if (str == NULL)  
  {
    return str;
  }

  if (buffer_size == 0)
  {
    return str;
  }

  size_t index = 0;

  while (*str != delim && *str != 0)
  {
    if (index < buffer_size - 1)
    {
      buffer[index++] = *str;
    }
    str++;
  }

  buffer[index] = 0;

  if (*str == delim)
  {
    str++;  
  }

  return str;
}

void apply_config(const char *str)
{
  char name[AP_NAME_LENGTH + 1];
  char pass[AP_PASS_LENGTH + 1];

  const char *next;

  WiFiMulti.cleanAPlist();

  for (int i = 0; i < AP_COUNT; i++)
  {
    next = extract_value(str, '\t', name, sizeof(name));

    if (next == str)
    {
      return;  
    }

    str = next;

    next = extract_value(str, '\r', pass, sizeof(pass));

    if (next == str)
    {
      return;  
    }

    WiFiMulti.addAP(name, pass);
  }
}

int load_config()
{
  if (!load_file(config_file_name, data, sizeof(data)))
  {
    return 0;
  }
  
  apply_config(data);
  
  return 1;
}

void save_config()
{
  save_file(config_file_name, data, strlen(data));
}


///////////////////////////////////////////////////////////////////////
// Setup
///////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Blink 5 times at startup
  for (int i = 0; i < 10; i++)
  {
    digitalWrite(LED_BUILTIN, i & 1 ? HIGH : LOW);
    delay(500);
  }

  WiFi.mode(WIFI_STA);

  LittleFS.begin();

  load_config();
  
  url[0] = 0;
  data[0] = 0;
}


///////////////////////////////////////////////////////////////////////
// Serial protocol receiver
///////////////////////////////////////////////////////////////////////

void serial_byte_received(unsigned char b)
{
  unsigned short crc;
  
  if (b == 1)
  {
    // Load URL command
    state = s_loading_url;
    url[0] = 0;
    url_n = 0;
  }
  else if (b == 2)
  {
    // Load Data command
    state = s_loading_data;
    data[0] = 0;
    data_n = 0;
  }
  else if (b == 3)
  {
    // POST command
    state = s_post;
  }
  else if (b == 4)
  {
    // GET command
    state = s_get;
  }
  else if (b == 5)
  {
    // Get URL and Data CRC command
    crc = crc16(url, strlen(url));
    Serial.write(crc >> 8);
    Serial.write(crc & 0xFF);

    crc = crc16(data, strlen(data));
    Serial.write(crc >> 8);
    Serial.write(crc & 0xFF);
  }
  else if (b == 6)
  {
    // Write Data content to config file and reload config
    int len = strlen(data);
    if (len > 4 && len < 2000)
    {
      save_config();
      load_config();
    }
    state = s_idle;
    url[0] = 0;
    url_n = 0;
    data[0] = 0;
    data_n = 0;
  }
  else if (b == 7)
  {
    // Check if connected
    Serial.write(connected ? '+' : '-');  
  }
  else
  {
    switch (state)
    {
      case s_loading_url:
        if (url_n < sizeof(url) - 1)
        {
          url[url_n++] = b;
          url[url_n] = 0;
        }
        break;
      case s_loading_data:
        if (data_n < sizeof(data) - 1)
        {
          data[data_n++] = b;
          data[data_n] = 0;
        }
        break;
    }
  }
}


///////////////////////////////////////////////////////////////////////
// Main loop
///////////////////////////////////////////////////////////////////////

void loop()
{
  unsigned short crc;

  // Check if we have bytes received via COM port
  while (Serial.available() > 0)
  {
    int b = Serial.read();

    serial_byte_received(b);
  }

  // Indicate connection status
  connected = WiFiMulti.run() == WL_CONNECTED;

  digitalWrite(LED_BUILTIN, !connected);

  // Process GET and POST requests
  if ((state == s_post || state == s_get) && connected)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    if (strstr(url, "http") != NULL)
    {
      WiFiClient client;
      HTTPClient http;

      http.begin(client, url);
    
      int httpCode;
      
      if (state == s_post)
      {
        httpCode = http.POST(data);
      }
      else
      {
        httpCode = http.GET();
      }

      if (httpCode > 0)
      {
        if (httpCode == HTTP_CODE_OK)
        {
          // get length of document (is -1 when Server sends no Content-Length header)
          int len = http.getSize();
          
          // get tcp stream
          WiFiClient * stream = &client;

          Serial.print(STREAM_START);

          if (stream_send_size)
          {
            Serial.write(len >> 24u);
            Serial.write(len >> 16u);
            Serial.write(len >> 8u);
            Serial.write(len & 0xFFu);
          }
          
          crc = crc_init;

          // read all data from server
          while (http.connected() && (len > 0 || len == -1))
          {
            int c = stream->readBytes(buf, std::min((size_t)len, sizeof(buf)));

            if (c > 0)
            {
              for (int i = 0; i < c; i++)
              {
                crc = crc16_update(buf[i], crc);
              }
              Serial.write(buf, c);
            }
            else
            {
              break;  
            }

            if (len > 0)
            {
              len -= c;
            }
          }
          Serial.print(STREAM_END);

          if (stream_send_crc)
          {
            Serial.write(crc >> 8);
            Serial.write(crc & 0xFF);
          }
        }
      }
      
      http.end();
    }

    state = s_idle;
    url[0] = 0;
    data[0] = 0;
  }

  delay(200);
}
