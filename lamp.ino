#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <AutoConnectCredential.h>
#include <PageBuilder.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

#define CREDENTIAL_OFFSET 0
#define LED_PIN 13
#define LED_COUNT 24
WebServer Server;
AutoConnect Portal(Server);
WiFiClient MQTTclient;
PubSubClient client(MQTTclient);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

String viewCredential(PageArgument&);
String delCredential(PageArgument&);
long lastReconnectAttempt = 0;
int bright = 255;
int last_brightness_before_darkness = 0;
long lastTouch = -1;
long currentTouch = -1;
boolean activeTouch = false;
boolean faded_to_zero = false;
boolean lamp_off = false;

const char* mqttServer = "mqtt.pndsn.com";
const int mqttPort = 1883;
const char* clientID = "pub-c-998558f2-4c6e-455d-b627-43a07aaf1d54/sub-c-3796a684-2c34-11eb-ae78-c6faad964e01/deni";
const char* deni_send_channel = "deni_send_channel";
const char* april_send_channel = "april_send_channel";

int colorID = 0;
const int colors[16][3] = {
  {204, 0, 0},
  {255, 87, 33}, 
  {10, 219, 201},   
  {148, 0, 212},   
  {255, 233, 186},
  {255, 75, 150},
  {20, 99, 245},
  {255, 140, 0},
  {255, 70, 5},
  {128, 0, 255},
  {204, 0, 204},
  {0, 0, 150},
  {30, 128, 128},
  {128, 255, 150},
  {0, 240, 0},
  {166, 41, 41}
};

static const char PROGMEM html[] = R"*lit(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
  <style>
  html {
  font-family:Helvetica,Arial,sans-serif;
  -ms-text-size-adjust:100%;
  -webkit-text-size-adjust:100%;
  }
  .menu > a:link {
    position: absolute;
    display: inline-block;
    right: 12px;
    padding: 0 6px;
    text-decoration: none;
  }
  </style>
</head>
<body>
<div class="menu">{{AUTOCONNECT_MENU}}</div>
<form action="/del" method="POST">
  <ol>
  {{SSID}}
  </ol>
  <p>Enter deleting entry:</p>
  <input type="number" min="1" name="num">
  <input type="submit">
</form>
</body>
</html>
)*lit";

static const char PROGMEM autoconnectMenu[] = { AUTOCONNECT_LINK(BAR_24) };

PageElement elmList(html,
  {{ "SSID", viewCredential },
   { "AUTOCONNECT_MENU", [](PageArgument& args) {
                            return String(FPSTR(autoconnectMenu));} }
  });
PageBuilder rootPage("/", { elmList });
PageElement elmDel("{{DEL}}", {{ "DEL", delCredential }});
PageBuilder delPage("/del", { elmDel });

String viewCredential(PageArgument& args) {
  AutoConnectCredential  ac(CREDENTIAL_OFFSET);
  station_config_t  entry;
  String content = "";
  uint8_t  count = ac.entries();

  for (int8_t i = 0; i < count; i++) { 
    ac.load(i, &entry);

    content += String("<li>") + String((char *)entry.ssid) + String("</li>");
  }

  return content;
}

String delCredential(PageArgument& args) {
  AutoConnectCredential  ac(CREDENTIAL_OFFSET);
  if (args.hasArg("num")) {
    int8_t  e = args.arg("num").toInt();
    Serial.printf("Request deletion #%d\n", e);
    if (e > 0) {
      station_config_t  entry;

      int8_t  de = ac.load(e - 1, &entry);  // A base of entry num is 0.
      if (de > 0) {
        Serial.printf("Delete for %s ", (char *)entry.ssid);
        Serial.printf("%s\n", ac.del((char *)entry.ssid) ? "completed" : "failed");

        Server.sendHeader("Location", String("http://") + Server.client().localIP().toString() + String("/"));
        Server.send(302, "text/plain", "");
        Server.client().flush();
        Server.client().stop();

        delPage.cancel();
      }
    }
  }
  return "";
}

void callback(char* topic, byte* payload, unsigned int length) {
    String payload_buff;
    for (int i=0;i<length;i++)
      payload_buff = payload_buff+String((char)payload[i]);
    Serial.print(payload_buff);
    Serial.println(" <- current color");
    colorID = payload_buff.toInt();
    uint32_t newColor = strip.Color(bright*colors[colorID][0] / 255,bright*colors[colorID][1] / 255,bright*colors[colorID][2] / 255);
    for(int i=0; i<strip.numPixels(); i++) 
        strip.setPixelColor(i, newColor);
    strip.show();
}

boolean reconnect() {
    if (client.connect(clientID))
        client.subscribe(april_send_channel);
    return client.connected();
}

void fade_in_out() {
  if(faded_to_zero) {
    for(int brightness = 0; brightness <= 255; brightness++) {
      uint32_t newColor = strip.Color(brightness , brightness , brightness);
      for(int i=0; i<strip.numPixels(); i++) 
          strip.setPixelColor(i, newColor);
      strip.show();
      delay(2000 / 255);
    }
  }

  if(!faded_to_zero) { 
    for(int brightness = 255; brightness >= 0; brightness--) {
      uint32_t newColor = strip.Color(brightness , brightness , brightness);
      for(int i=0; i<strip.numPixels(); i++) 
          strip.setPixelColor(i, newColor);
      strip.show();
      delay(2000 / 255);
    }
  }
}

void fade_in_out_orange() {
  if(faded_to_zero) {
    for(int brightness = 0; brightness <= 255; brightness++) {
      uint32_t newColor = strip.Color(brightness, bright*94/255, bright*19/255);
      for(int i=0; i<strip.numPixels(); i++) 
          strip.setPixelColor(i, newColor);
      strip.show();
      delay(2000 / 255);
    }
  }

  if(!faded_to_zero) { 
    for(int brightness = 255; brightness >= 0; brightness--) {
      uint32_t newColor = strip.Color(brightness, bright*94/255, bright*19/255);
      for(int i=0; i<strip.numPixels(); i++) 
          strip.setPixelColor(i, newColor);
      strip.show();
      delay(2000 / 255);
    }
  }
}

void flash_twice() {
      for(int i = 0; i < 2; i++) {
        uint32_t newColor = strip.Color(255, 255, 255);
        for(int i=0; i<strip.numPixels(); i++) 
            strip.setPixelColor(i, newColor);
        strip.show();
        delay(250);
        newColor = strip.Color(0, 0, 0);
        for(int i=0; i<strip.numPixels(); i++) 
            strip.setPixelColor(i, newColor);
        strip.show();
        delay(250);
      }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  rootPage.insert(Server);    // Instead of Server.on("/", ...);
  delPage.insert(Server);     // Instead of Server.on("/del", ...);

  AutoConnectConfig Config("wifi-lamp-ap", "a22ss8n7l06l8w");
  Config.boundaryOffset = CREDENTIAL_OFFSET;
  Portal.config(Config);

  if (Portal.begin())
    Serial.println("WiFi connected: " + WiFi.localIP().toString());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  strip.begin();
  strip.show();
  strip.setBrightness(255);
  flash_twice();
}

void loop() {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Currently not connected");
    faded_to_zero = !faded_to_zero;
    fade_in_out();
  }
  Portal.handleClient();
  if (!client.connected()) {
      long now = millis();
      if (now - lastReconnectAttempt > 5000) { // Try to reconnect.
          lastReconnectAttempt = now;
          if (reconnect())
              lastReconnectAttempt = 0;
      }
  } else { // Connected.
      long timeSinceBoardStart = millis();
      int touchValue = touchRead(T7);
      if (touchValue < 40) {
          if (activeTouch) {
              activeTouch = false;
              currentTouch = timeSinceBoardStart;
          }
      } else {
          if (!activeTouch) {
              activeTouch = true;
              lastTouch = timeSinceBoardStart;
              int diff =  lastTouch - currentTouch;
              if (diff > 50 && diff <= 450) {
                if (bright != 0) {
                  Serial.print("This was a tap! - ");
                  Serial.println(diff);
                  colorID = (colorID+1)%16;
                  String ColorIDString = String(colorID);
                  uint32_t newColor = strip.Color(bright*colors[colorID][0] / 255,bright*colors[colorID][1] / 255,bright*colors[colorID][2] / 255);
                  for(int i=0; i<strip.numPixels(); i++)
                      strip.setPixelColor(i, newColor);
                  strip.show();
                  client.publish(deni_send_channel, ColorIDString.c_str()); // Publish message.
                }
              } else if (diff > 450 && diff < 3000) {
                  if(lamp_off) {
                    bright = last_brightness_before_darkness;
                    lamp_off = false;
                  } else bright = bright - 35;
                  if (bright == -25) bright = 0;
                  if (bright == -35) bright = 255;
                  uint32_t newColor = strip.Color(bright*colors[colorID][0] / 255,bright*colors[colorID][1] / 255,bright*colors[colorID][2] / 255);
                  for(int i=0; i<strip.numPixels(); i++)
                    strip.setPixelColor(i, newColor);
                  strip.show();
                  Serial.print("This was a long press! - ");
                  Serial.println(diff);
              } else if (diff >= 3000) {
                  if(bright == 0) {
                    bright = last_brightness_before_darkness;
                    lamp_off = false;
                  } else {
                    last_brightness_before_darkness = bright;
                    bright = 0;
                    lamp_off = true;
                  }
                  uint32_t newColor = strip.Color(bright*colors[colorID][0] / 255,bright*colors[colorID][1] / 255,bright*colors[colorID][2] / 255);
                  for(int i=0; i<strip.numPixels(); i++)
                    strip.setPixelColor(i, newColor);
                  strip.show();
                  Serial.print("Turning it off! - ");
                  Serial.println(diff);
              }
          }
        }  
      }
  if(!client.loop())
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("Server unreachable");
      faded_to_zero = !faded_to_zero;
      fade_in_out_orange();
    }
  delay(50);
}
