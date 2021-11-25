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
