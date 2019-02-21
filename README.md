# ESP32-MPU9250-web-view

**_Visualizing MPU9250 3D orientation data on the web page hosted by ESP32. Using websockets and JSON to send orientation data (quaternion or Euler angles) to the web browser. Visualisation is done using Three.js library. Written using Arduino framework._**

MPU9250 is one of the most popular IMU (Inertial Measurement Unit) available on the market. It combines not only 3D accelerometer, 3D gyro and 3D compass but also DMP (Digital Motion Processor). Thanks to DMP we can read orientation data in the form of Euler angles or quaternions directly from the chip.

A web page with a 3D cube visualizing orientation of MPU9250 is hosted by ESP32. Real time data from MPU9250 is sent to a web browser through a websocket. That data is then used by Three.js library (https://threejs.org/) visualize orientation of the cube in a real time. The web page is available both in LAN and through the internet thanks to Husarnet (https://husarnet.com/).

# Connect MPU9250 to ESP32

```
ESP32 <-> MPU9250

P22   <-> SCL
P21   <-> SDA
P19   <-> INT
```

# Configure Arduino IDE

To run the project, at first you need to configure your Arduino IDE:

**1. Install Husarnet package for ESP32:**

- open `File -> Preferences`
- in a field **Additional Board Manager URLs** add this link: `https://files.husarion.com/arduino/package_esp32_index.json`
- open `Tools -> Board: ... -> Boards Manager ...`
- Search for `esp32-husarnet by Husarion`
- Click Install button

**2. Select ESP32 dev board:**

- open `Tools -> Board`
- select **_ESP32 Dev Module_** under "ESP32 Arduino (Husarnet)" section

**3. Install ArduinoJson library:**

- open `Tools -> Manage Libraries...`
- search for `ArduinoJson`
- select version `5.13.3`
- click **install** button

**4. Install arduinoWebSockets library (Husarnet fork):**

- download https://github.com/husarnet/arduinoWebSockets as a ZIP file (this is Husarnet compatible fork of arduinoWebSockets by Links2004 (Markus) )
- open `Sketch -> Include Library -> Add .ZIP Library ...`
choose **arduinoWebSockets-master.zip** file that you just downloaded and click open button

**5. Install SparkFun_MPU-9250-DMP_Arduino_Library :**

- download https://github.com/sparkfun/SparkFun_MPU-9250-DMP_Arduino_Library as a ZIP file 
- open `Sketch -> Include Library -> Add .ZIP Library ...`
choose **SparkFun_MPU-9250-DMP_Arduino_Library-master.zip** file that you just downloaded and click **open** button


# Program ESP32 using Arduino IDE

- Open **ESP32-MPU9250-web-view.ino** project
- modify line 33 with your Husarnet `join code` (get on https://app.husarnet.com)
- modify lines 12 - 21 to add your Wi-Fi network credentials
- upload project to your ESP32 board.

# Open a web page hosted by ESP32

There are two options:

1. log into your account at https://app.husarnet.com, find `box3desp32` device that you just connected and click `web UI` button. You can also click `box3desp32` element to open "Element settings" and select `Make the Web UI public` if you want to have a publically available address. In such a scenerio Husarnet proxy servers are used to provide you a web UI. Works with any web browser: desktop and mobile.

2. P2P option - add your laptop to the same Husarnet network as ESP32 board. In that scenerio no proxy servers are used and you connect to ESP32 with a very low latency directly with no port forwarding on your router. Currenly only Linux client is available, so open your Linux terminal and type:

- `$ curl https://install.husarnet.com/install.sh | sudo bash` to install Husarnet.
- `$ husarnet join XXXXXXXXXXXXXXXXXXXXXXX mylaptop` replace XXX...X with your own `join code` (see point 4), and click enter.

At this stage your ESP32 and your laptop are in the same VLAN network. The best hostname support is in Mozilla Firefox web browser (other browsers also work, but you have to use IPv6 address of your device that you will find at https://app.husarnet.com) and type:
`http://box3desp32:8000`
You should see a web UI to controll your ESP32 now.

