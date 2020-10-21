# ESP32-MPU9250-web-view

**_Visualizing MPU9250 3D orientation data on the web page hosted by ESP32. Using websockets and JSON to send orientation data (quaternion or Euler angles) to the web browser. Visualisation is done using Three.js library. Written using Arduino framework._**

MPU9250 is one of the most popular IMU (Inertial Measurement Unit) available on the market. It combines not only 3D accelerometer, 3D gyro and 3D compass but also DMP (Digital Motion Processor). Thanks to DMP we can read orientation data in the form of Euler angles or quaternions directly from the chip.

A web page with a 3D cube visualizing orientation of MPU9250 is hosted by ESP32. Real time data from MPU9250 is sent to a web browser through a websocket. That data is then used by Three.js library (https://threejs.org/) to visualize orientation of the cube in a real time. The web page is available both in LAN and through the internet thanks to Husarnet (https://husarnet.com/).

To run the example, follow the next steps.

# Connect MPU9250 to ESP32

Interface between ESP32 and MPU9250 is as follows:
```
ESP32 <-> MPU9250

P22   <-> SCL
P21   <-> SDA
P19   <-> INT

GND   <-> GND
```

Remember also to provide appropriate power supply both for ESP32 and MPU9250.

# Configure Platform IO IDE

- Clone the project repo on your disk and open it from Visual Studio Code with platformio installed
- Open **ESP32-MPU9250-web-view.ino** file
- modify line 39 with your Husarnet `join code` (get on https://app.husarnet.com)
- modify lines 18 - 27 to add your Wi-Fi network credentials
- upload project to your ESP32 board.

If you face any issues in the process, please visit this site: https://docs.husarnet.com/docs/begin-esp32-platformio#get-platformio .

# Open a web page hosted by ESP32

There are two options:

1. log into your account at https://app.husarnet.com, find `box3desp32` device that you just connected and click `web UI` button. You can also click `box3desp32` element to open "Element settings" and select `Make the Web UI public` if you want to have a publically available address. In such a scenerio Husarnet proxy servers are used to provide you a web UI. Works with any web browser: desktop and mobile.

2. P2P option - add your laptop to the same Husarnet network as ESP32 board. In that scenerio no proxy servers are used and you connect to ESP32 with a very low latency directly with no port forwarding on your router. Currenly only Linux client is available, so open your Linux terminal and type:

- `$ curl https://install.husarnet.com/install.sh | sudo bash` to install Husarnet.
- `$ husarnet join XXXXXXXXXXXXXXXXXXXXXXX mylaptop` replace XXX...X with your own `join code` (see point 4), and click enter.

At this stage your ESP32 and your laptop are in the same VLAN network. The best hostname support is in Mozilla Firefox web browser (other browsers also work, but you have to use IPv6 address of your device that you will find at https://app.husarnet.com) and type:
`http://box3desp32:8000`
You should see a web UI to controll your ESP32 now.

