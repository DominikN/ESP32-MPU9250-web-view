#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Husarnet.h>
#include <ArduinoJson.h>
#include <SparkFunMPU9250-DMP.h>

/* =============== config section start =============== */

#define INTERRUPT_PIN 19

// WiFi credentials
#define NUM_NETWORKS 2
// Add your networks credentials here
const char* ssidTab[NUM_NETWORKS] = {
  "wifi-ssid-one",
  "wifi-ssid-two",
};
const char* passwordTab[NUM_NETWORKS] = {
  "wifi-pass-one",
  "wifi-pass-two",
};

// Husarnet credentials
const char* hostName = "box3desp32";  //this will be the name of the 1st ESP32 device at https://app.husarnet.com

/* to get your join code go to https://app.husarnet.com
   -> select network
   -> click "Add element"
   -> select "join code" tab

   Keep it secret!
*/
const char* husarnetJoinCode = "xxxxxxxxxxxxxxxxxxxxxx";

/* =============== config section end =============== */

#define HTTP_PORT 8000
#define WEBSOCKET_PORT 8001

WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);
HusarnetServer server(HTTP_PORT);

StaticJsonBuffer<200> jsonBufferTx;
JsonObject& rootTx = jsonBufferTx.createObject();

const char* html =
#include "html.h"
  ;

bool wsconnected = false;

MPU9250_DMP imu;

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      {
        wsconnected = false;
        Serial.printf("[%u] Disconnected\r\n", num);
      }
      break;
    case WStype_CONNECTED:
      {
        wsconnected = true;
        Serial.printf("\r\n[%u] Connection from Husarnet \r\n", num);
      }
      break;

    case WStype_TEXT:
      {
        Serial.printf("[%u] Text:\r\n", num);
        for (int i = 0; i < length; i++) {
          Serial.printf("%c", (char)(*(payload + i)));
        }
        Serial.println();
      }
      break;

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

void taskWifi( void * parameter );
void taskHTTP( void * parameter );
void taskStatus( void * parameter );

SemaphoreHandle_t sem;
SemaphoreHandle_t mtx;

const signed char orientationDefault[9] = { 0, 1, 0, 0, 0, 1, 1, 0, 0 };

void setup()
{
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  if (imu.begin() != INV_SUCCESS) {
    while (1) {
      Serial.println("Unable to communicate with MPU-9250");
      Serial.println("Check connections, and try again.");
      Serial.println();
      delay(5000);
    }
  }

  imu.enableInterrupt();
  imu.setIntLevel(INT_ACTIVE_LOW);
  imu.setIntLatched(INT_LATCHED);

  imu.dmpBegin(DMP_FEATURE_6X_LP_QUAT | // Enable 6-axis quat
               DMP_FEATURE_GYRO_CAL, // Use gyro calibration
               10); // Set DMP FIFO rate to 10 Hz
  imu.dmpSetOrientation(orientationDefault);

  sem = xSemaphoreCreateCounting( 10, 0 );
  mtx = xSemaphoreCreateMutex();
  xSemaphoreGive( mtx );

  xTaskCreatePinnedToCore(
    taskWifi,          /* Task function. */
    "taskWifi",        /* String with name of task. */
    20000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    2,                /* Priority of the task. */
    NULL,             /* Task handle. */
    0);               /* Core where the task should run */

  xTaskCreatePinnedToCore(
    taskHTTP,          /* Task function. */
    "taskHTTP",        /* String with name of task. */
    20000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    2,                /* Priority of the task. */
    NULL,             /* Task handle. */
    0);               /* Core where the task should run */

  xTaskCreatePinnedToCore(
    taskStatus,          /* Task function. */
    "taskStatus",        /* String with name of task. */
    20000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    2,                /* Priority of the task. */
    NULL,             /* Task handle. */
    1);               /* Core where the task should run */
}

void taskWifi( void * parameter ) {
  bool initialSetup = 0;
  while (1) {
    for (int i = 0; i < NUM_NETWORKS; i++) {
      delay(500);
      Serial.println();
      Serial.print("Connecting to ");
      Serial.print(ssidTab[i]);
      WiFi.begin(ssidTab[i], passwordTab[i]);
      for (int j = 0; j < 10; j++) {
        if (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
        } else {
          if (initialSetup == 0) {
            server.begin();

            Husarnet.join(husarnetJoinCode, hostName);
            Husarnet.start();

            webSocket.begin();
            webSocket.onEvent(onWebSocketEvent);

            initialSetup = 1;
          }

          Serial.println("done");
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());

          while (WiFi.status() == WL_CONNECTED) {
            xSemaphoreTake(sem, 5);

            if (xSemaphoreTake(mtx, 5) == pdTRUE) {
              webSocket.loop();
              xSemaphoreGive( mtx );
            }
          }
        }
      }
    }
  }
}

void taskHTTP( void * parameter )
{
  String header;

  while (1) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    delay(10);

    HusarnetClient client = server.available();

    if (client) {
      Serial.println("New Client.");
      String currentLine = "";
      Serial.printf("connected: %d\r\n", client.connected());
      while (client.connected()) {

        if (client.available()) {
          char c = client.read();
          Serial.write(c);
          header += c;
          if (c == '\n') {
            if (currentLine.length() == 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              client.println(html);
              break;
            } else {
              currentLine = "";
            }
          } else if (c != '\r') {
            currentLine += c;
          }
        }
      }

      header = "";

      client.stop();
      Serial.println("Client disconnected.");
      Serial.println("");
    } else {
      delay(200);
    }
  }
}

void taskStatus( void * parameter )
{
  String output;
  unsigned short fifoCnt;
  inv_error_t result;

  portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;

  while (1) {
    if ( digitalRead(INTERRUPT_PIN) == LOW ) {

      taskENTER_CRITICAL(&myMutex);
      fifoCnt = imu.fifoAvailable();
      taskEXIT_CRITICAL(&myMutex);

      if ( fifoCnt > 0) {
        taskENTER_CRITICAL(&myMutex);
        result = imu.dmpUpdateFifo();
        taskEXIT_CRITICAL(&myMutex);

        if ( result == INV_SUCCESS) {
          imu.computeEulerAngles();
          output = "";

          float q0 = imu.calcQuat(imu.qw);
          float q1 = imu.calcQuat(imu.qx);
          float q2 = imu.calcQuat(imu.qy);
          float q3 = imu.calcQuat(imu.qz);

          //rootTx["roll"] = imu.roll;
          //rootTx["pitch"] = imu.pitch;
          //rootTx["yaw"] = imu.yaw;
          rootTx["q0"] = q0;
          rootTx["q1"] = q1;
          rootTx["q2"] = q2;
          rootTx["q3"] = q3;
          rootTx.printTo(output);

          //Serial.print(F("Sending: "));
          //Serial.println(output);

          if (wsconnected == true) {
            if (xSemaphoreTake(mtx, 0) == pdTRUE) {
              webSocket.sendTXT(0, output);
              xSemaphoreGive( sem );
              xSemaphoreGive( mtx );
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
  }
}

void loop()
{
  Serial.printf("loop() running on core %d\r\n", xPortGetCoreID());
  while (1) {
    delay(5000);
  }
}
