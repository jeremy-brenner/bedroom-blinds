#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Esp.h>
#include <Servo.h>

#include <AceTime.h>
#include <AceTimeClock.h>

#include "wifi.h"
#include "schedule.h"
#include "servo.h"

using namespace ace_time;
using namespace ace_time::clock;
using ace_time::clock::NtpClock;
using ace_time::zonedbx2025::kZoneAmerica_Chicago;

static BasicZoneProcessor chicagoProcessor;
static NtpClock ntpClock;

ESP8266WebServer server(80);
Servo servo;

bool haveWifi;

#define OPEN_ANGLE MIDDLE_ANGLE+DISTANCE
#define CLOSE_ANGLE MIDDLE_ANGLE-DISTANCE

int lastOpenDay=0;
int lastCloseDay=0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  
  servoMove(MIDDLE_ANGLE);

  connectToWifi();
  ntpClock.setup(); 
  if (!ntpClock.isSetup()) {
    Serial.println(F("Something went wrong."));
    return;
  }

  server.on("/api/open", []() {
    doOpen();
    sendOk();
  });
  server.on("/api/close", []() {
    doClose();
    sendOk();
  });

  server.onNotFound(sendNotFound);
  server.begin();
}

void loop() {
  acetime_t nowSeconds = ntpClock.getNow();

  TimeZone tz = TimeZone::forZoneInfo(&kZoneAmerica_Chicago, &chicagoProcessor);
  ZonedDateTime zdt = ZonedDateTime::forEpochSeconds(nowSeconds, tz);

  if( zdt.hour() == CLOSE_HOUR && zdt.day() != lastCloseDay ) {
    doClose();
    lastCloseDay = zdt.day();
  }
  if( zdt.hour() == OPEN_HOUR && zdt.day() != lastOpenDay ) {
    doOpen();
    lastOpenDay = zdt.day();
  }

  server.handleClient();
  delay(2000);
}

void doOpen() {
  Serial.println("doOpen()");
  servoMove(OPEN_ANGLE);
  servoMove(MIDDLE_ANGLE);
}

void doClose() {
  Serial.println("doClose()");
  servoMove(CLOSE_ANGLE);
  servoMove(MIDDLE_ANGLE);
}

void servoMove(int angle) {
  servo.attach(SERVO_PIN);
  servo.write(angle);
  delay(500);
  servo.detach();  
}

void connectToWifi() {
  Serial.println("Configuring access point...");
  WiFi.mode(WIFI_STA);
  delay(1000);
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (!haveWifi) {
    Serial.print(".");
    checkWifi();
    delay(1000);
  }
}

void checkWifi() {
  if (WiFi.status() == WL_CONNECTED && !haveWifi) {
    haveWifi = true;
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  if (WiFi.status() != WL_CONNECTED && haveWifi) {
    haveWifi = false;
    Serial.println("WiFi disconnected");
  }
}

void sendBadRequest() {
  server.send(400, "text/plain", "400: Bad Request");
}

void sendNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

void sendOk() {
  server.send(200, "text/plain", "OK");
}


