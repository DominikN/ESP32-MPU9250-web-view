#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <Husarnet.h>
#include <SparkFunMPU9250-DMP.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Wire.h>

/* =============== config section start =============== */

#if __has_include("credentials.h")
#include "credentials.h"
#else

// WiFi credentials
#define NUM_NETWORKS 2
// Add your networks credentials here
const char *ssidTab[NUM_NETWORKS] = {
    "wifi-ssid-one",
    "wifi-ssid-two",
};
const char *passwordTab[NUM_NETWORKS] = {
    "wifi-pass-one",
    "wifi-pass-two",
};

// Husarnet credentials
const char *hostName = "box3desp32";  // this will be the name of the 1st ESP32
                                      // device at https://app.husarnet.com

/* to get your join code go to https://app.husarnet.com
   -> select network
   -> click "Add element"
   -> select "join code" tab

   Keep it secret!
*/
const char *husarnetJoinCode = "xxxxxxxxxxxxxxxxxxxxxx";
const char *dashboardURL = "default";
#endif

#define IMU_SELECT 1  // 1 - BNO055 , 0 - MPU9250

/* =============== config section end =============== */

#if IMU_SELECT == 0

#define INTERRUPT_PIN_MPU 19
// Connected to "Wire" object - 22 (SCL) & 21 (SDA)
MPU9250_DMP mpu;

#else

#define INTERRUPT_PIN_BNO 35
#define RESET_PIN_BNO 34
#define I2C_SDA 33
#define I2C_SCL 32
TwoWire I2CBNO = TwoWire(1);
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &I2CBNO);

#endif

#define HTTP_PORT 8000
#define WEBSOCKET_PORT 8001

// you can provide credentials to multiple WiFi networks
WiFiMulti wifiMulti;

// HTTP server on port 8000
WebServer server(HTTP_PORT);

// WebSocket server
WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);

StaticJsonDocument<200> jsonDocTx;

extern const char index_html_start[] asm("_binary_src_index_html_start");
const String html = String((const char *)index_html_start);

bool wsconnected = false;

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                      size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: {
      wsconnected = false;
      Serial.printf("[%u] Disconnected\r\n", num);
    } break;
    case WStype_CONNECTED: {
      wsconnected = true;
      Serial.printf("\r\n[%u] Connection from Husarnet \r\n", num);
    } break;

    case WStype_TEXT: {
      Serial.printf("[%u] Text:\r\n", num);
      for (int i = 0; i < length; i++) {
        Serial.printf("%c", (char)(*(payload + i)));
      }
      Serial.println();
    } break;

    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

void onHttpReqFunc() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", html);
}

void taskWifi(void *parameter);
void taskStatus(void *parameter);

SemaphoreHandle_t mtx;

const signed char orientationDefault[9] = {0, 1, 0, 0, 0, 1, 1, 0, 0};

void setup() {
  Serial.begin(115200);

  mtx = xSemaphoreCreateMutex();
  xSemaphoreGive(mtx);

  xTaskCreatePinnedToCore(taskWifi,   /* Task function. */
                          "taskWifi", /* String with name of task. */
                          20000,      /* Stack size in bytes. */
                          NULL, /* Parameter passed as input of the task */
                          2,    /* Priority of the task. */
                          NULL, /* Task handle. */
                          0);   /* Core where the task should run */

  xTaskCreatePinnedToCore(taskStatus,   /* Task function. */
                          "taskStatus", /* String with name of task. */
                          20000,        /* Stack size in bytes. */
                          NULL, /* Parameter passed as input of the task */
                          3,    /* Priority of the task. */
                          NULL, /* Task handle. */
                          0);   /* Core where the task should run */
}

void taskWifi(void *parameter) {
  uint8_t stat = WL_DISCONNECTED;

  /* Configure Wi-Fi */
  for (int i = 0; i < NUM_NETWORKS; i++) {
    wifiMulti.addAP(ssidTab[i], passwordTab[i]);
    Serial.printf("WiFi %d: SSID: \"%s\" ; PASS: \"%s\"\r\n", i, ssidTab[i],
                  passwordTab[i]);
  }

  while (stat != WL_CONNECTED) {
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
    delay(100);
  }

  Serial.printf("WiFi connected\r\n", (int)stat);
  Serial.printf("IP address: ");
  Serial.println(WiFi.localIP());

  /* Start Husarnet */
  Husarnet.selfHostedSetup(dashboardURL);
  Husarnet.join(husarnetJoinCode, hostName);
  Husarnet.start();

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  /* Confgiure HTTP server */
  server.on("/", HTTP_GET, onHttpReqFunc);
  server.on("/index.html", HTTP_GET, onHttpReqFunc);
  server.begin();

  while (1) {
    while (WiFi.status() == WL_CONNECTED) {
      if (xSemaphoreTake(mtx, 5) == pdTRUE) {
        webSocket.loop();
        server.handleClient();
        xSemaphoreGive(mtx);
      }
      delay(5);
    }
    Serial.printf("WiFi disconnected, reconnecting\r\n");
    delay(500);
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
  }
}

void taskStatus(void *parameter) {
  String output;
  unsigned short fifoCnt;
  inv_error_t result;

#if IMU_SELECT == 0
  pinMode(INTERRUPT_PIN_MPU, INPUT_PULLUP);

  if (mpu.begin() != INV_SUCCESS) {
    while (1) {
      Serial.println("Unable to communicate with MPU-9250");
      Serial.println("Check connections, and try again.");
      Serial.println();
      delay(5000);
    }
  }

  mpu.enableInterrupt();
  mpu.setIntLevel(INT_ACTIVE_LOW);
  mpu.setIntLatched(INT_LATCHED);

  mpu.dmpBegin(DMP_FEATURE_6X_LP_QUAT |   // Enable 6-axis quat
                   DMP_FEATURE_GYRO_CAL,  // Use gyro calibration
               10);                       // Set DMP FIFO rate to 10 Hz
  mpu.dmpSetOrientation(orientationDefault);
#endif  // IMU_SELECT == 0 // MPU9250

#if IMU_SELECT == 1
  pinMode(INTERRUPT_PIN_BNO, INPUT_PULLUP);
  pinMode(RESET_PIN_BNO, OUTPUT);

  digitalWrite(RESET_PIN_BNO, 0);
  delay(100);
  digitalWrite(RESET_PIN_BNO, 1);

  I2CBNO.begin(I2C_SDA, I2C_SCL, 100000);

  if (!bno.begin()) {
    while (1) {
      Serial.print("No BNO055 detected");
      delay(1000);
    }
  }
#endif  // IMU_SELECT == 1 // BNO055

  while (1) {
#if IMU_SELECT == 0
    if (digitalRead(INTERRUPT_PIN_MPU) == LOW) {
      fifoCnt = mpu.fifoAvailable();

      if (fifoCnt > 0) {
        result = mpu.dmpUpdateFifo();

        if (result == INV_SUCCESS) {
          mpu.computeEulerAngles();
          output = "";

          float q0 = mpu.calcQuat(mpu.qw);
          float q1 = mpu.calcQuat(mpu.qx);
          float q2 = mpu.calcQuat(mpu.qy);
          float q3 = mpu.calcQuat(mpu.qz);

          Serial.printf("Qmpu=[%f,%f,%f,%f]\r\n", q0, q1, q2, q3);
          Serial.printf("---------------------\r\n");
          // rootTx["roll"] = mpu.roll;
          // rootTx["pitch"] = mpu.pitch;
          // rootTx["yaw"] = mpu.yaw;

          jsonDocTx.clear();
          jsonDocTx["q0"] = q0;
          jsonDocTx["q1"] = q1;
          jsonDocTx["q2"] = q2;
          jsonDocTx["q3"] = q3;
          serializeJson(jsonDocTx, output);

          //          Serial.print(F("Sending: "));
          //          Serial.println(output);

          if (wsconnected == true) {
            if (xSemaphoreTake(mtx, 5) == pdTRUE) {
              webSocket.sendTXT(0, output);
              xSemaphoreGive(mtx);
            }
          }
        }
      } else {
        Serial.println("false interrupt");
        delay(20);
      }
    } else {
      delay(20);
    }
#endif  // IMU_SELECT == 0 // MPU9250
#if IMU_SELECT == 1
    if (digitalRead(INTERRUPT_PIN_BNO) == LOW) {
      output = "";

      imu::Quaternion quat = bno.getQuat();
      Serial.printf("Qbno=[%f,%f,%f,%f]\r\n", quat.w(), quat.x(), quat.y(),
                    quat.z());

      jsonDocTx.clear();
      jsonDocTx["q0"] = quat.w();
      jsonDocTx["q1"] = quat.x();
      jsonDocTx["q2"] = quat.y();
      jsonDocTx["q3"] = quat.z();
      serializeJson(jsonDocTx, output);

      // Serial.print(F("Sending: "));
      // Serial.println(output);

      if (wsconnected == true) {
        if (xSemaphoreTake(mtx, 5) == pdTRUE) {
          webSocket.sendTXT(0, output);
          xSemaphoreGive(mtx);
        }
      }
      delay(100);
    } else {
      delay(100);
    }
#endif  // IMU_SELECT == 1 // BNO055
  }
}

void loop() {
  Serial.printf("loop() running on core %d\r\n", xPortGetCoreID());
  while (1) {
    Serial.printf("[RAM: %d]\r\n", esp_get_free_heap_size());
    delay(1000);
  }
}
