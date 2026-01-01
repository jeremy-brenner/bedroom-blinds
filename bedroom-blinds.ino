#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Esp.h>
#include <Servo.h>
#include <AceWire.h> // TwoWireInterface
#include <Wire.h> // TwoWire, Wire
#include <AceTime.h>
#include <AceTimeClock.h>

#include <ArduinoJson.h>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>
#include <LittleFS.h>

#include "wifi.h"
#include "schedule.h"
#include "servo.h"
#include "deployment.h"

using namespace ace_time;
using namespace ace_time::clock;

using ace_time::clock::SystemClockLoop;
using ace_time::clock::NtpClock;
using ace_time::clock::DS3231Clock;
using ace_time::DateStrings;
using ace_time::zonedbx2025::kZoneAmerica_Chicago;
using WireInterface = ace_wire::TwoWireInterface<TwoWire>;

WireInterface wireInterface(Wire);
DS3231Clock<WireInterface> dsClock(wireInterface);
NtpClock ntpClock;
SystemClockLoop systemClock(&ntpClock /*reference*/, &dsClock /*backup*/);

static BasicZoneProcessor chicagoProcessor;

ESP8266WebServer server(80);
Servo servo;

static bool fsOK;
bool haveWifi;
DynamicJsonDocument schedule(200);

FS* fileSystem = &LittleFS;
LittleFSConfig fileSystemConfig = LittleFSConfig();

int lastOpenDay=0;
int lastCloseDay=0;

String scheduleFile = String(DEPLOYMENT_DIR) + String("/schedule.json");

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  
  Wire.begin();
  wireInterface.begin();
  dsClock.setup();
  connectToWifi();
  ntpClock.setup();
  systemClock.setup();

  servoMove(MIDDLE_ANGLE);

  fileSystemConfig.setAutoFormat(false);
  fileSystem->setConfig(fileSystemConfig);

  fsOK = fileSystem->begin();
  Serial.println(fsOK ? F("Filesystem initialized.") : F("Filesystem init failed!"));
  if(!fsOK) {
    stop();
  }
  
  ensureSchedule(scheduleFile);
  readSchedule();

  server.on("/api/open", []() {
    doOpen();
    sendOk();
  });

  server.on("/api/close", []() {
    doClose();
    sendOk();
  });

  server.on("/api/readSchedule", []() {
    readSchedule();
    sendOk();
  });

  server.on("/api/ls", []() {
    server.send(200, "text/plain", doLS());
  });

  server.on(UriRegex("/.*"), HTTP_POST, []() {
    server.send(200, "text/plain", "Upload handled"); // Send a basic response
  }, handleFileUpload); 

  server.on(UriRegex("/.*"), HTTP_DELETE, handleDelete);


  server.on(UriRegex("/.*"), HTTP_GET, handleGet);
  server.onNotFound(sendNotFound);
  server.begin();
}

int getScheduleTime(String direction, String day) {
  return schedule[direction][day].isNull() ? schedule[direction]["Default"] : schedule[direction][day];
}

void loop() {
  systemClock.loop();
 
  acetime_t nowSeconds = systemClock.getNow();

  TimeZone tz = TimeZone::forZoneInfo(&kZoneAmerica_Chicago, &chicagoProcessor);
  ZonedDateTime zdt = ZonedDateTime::forEpochSeconds(nowSeconds, tz);

  String today = DateStrings().dayOfWeekShortString(zdt.dayOfWeek());

  int openTime = getScheduleTime("open", today);
  int openHour = openTime/100;
  int openMinute = openTime%100;

  if( zdt.hour() == openHour && zdt.minute() == openMinute && zdt.day() != lastOpenDay ) {
    doOpen();
    lastOpenDay = zdt.day();
  }

  int closeTime = getScheduleTime("close", today);
  int closeHour = closeTime/100;
  int closeMinute = closeTime%100;

  if( zdt.hour() == closeHour && zdt.minute() == closeMinute && zdt.day() != lastCloseDay ) {
    doClose();
    lastCloseDay = zdt.day();
  }
  server.handleClient();
  delay(200);
}

void connectToWifi() {
  Serial.println("Configuring access point...");
  WiFi.mode(WIFI_STA);
  delay(1000);
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (int i = 0; i <= 60; i++) {
    Serial.print(".");
    checkWifi();
    if (haveWifi) {
      break; 
    }
    delay(1000);
  }
  if (!haveWifi) {
    Serial.println("Wifi not connected, rebooting.");
    ESP.restart();
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


String doLS() {
  DynamicJsonDocument doc(2048);
  FSInfo fs_info;
  fileSystem->info(fs_info);

  JsonObject fsInfoJson = doc.createNestedObject("fsInfo");
  fsInfoJson["totalBytes"] = String(fs_info.totalBytes);
  fsInfoJson["usedBytes"] = String(fs_info.usedBytes);

  JsonObject root = doc.createNestedObject("root");
  JsonArray entries = root.createNestedArray("entries");

  listDir("/", entries);

  String status;
  serializeJson(doc, status);
  return status;
}

void readSchedule() {
  File scheduleFileJson = fileSystem->open(scheduleFile, "r");

  DeserializationError error = deserializeJson(schedule, scheduleFileJson);
  scheduleFileJson.close();

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
  } else {
    Serial.println("JSON parsed successfully:");
    // Print the content to the Serial port (pretty printed)
    serializeJsonPretty(schedule, Serial); 
  }
}

void listDir(String path, JsonArray entries) {
  Dir dir = fileSystem->openDir(path);
  while (dir.next()) {   
    JsonObject entry = entries.createNestedObject(); 
    entry["name"] = String(dir.fileName());
    entry["type"] = dir.isDirectory() ? String("Directory") : String("File");
    if(dir.isFile()) {
      entry["size"] = String(dir.fileSize());
    }
    if(dir.isDirectory()) {
      String subpath = path + dir.fileName() + String('/');    
      JsonArray entries = entry.createNestedArray("entries");
      listDir(subpath,entries);
    }
  }
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

void sendBadRequest() {
  server.send(400, "text/plain", "400: Bad Request");
}

void sendNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

void sendOk() {
  server.send(200, "text/plain", "OK");
}


void handleGet() { 
  handleFileRead(server.uri());
}

void handleFileRead(String path) {
  Serial.println("Trying to serve");
  path = deploymentFilename(path);
  Serial.println(path);
  if (path.endsWith("/")) {
    path += "index.html";
  }

  String contentType = mime::getContentType(path);

  if (!fileSystem->exists(path)) {
    // File not found, try gzip version
    path = path + ".gz";
  }
  if (fileSystem->exists(path)) {
    Serial.println("exists");
    File file = fileSystem->open(path, "r");
    if (server.streamFile(file, contentType) != file.size()) { Serial.println("Sent less data than expected!"); }
    file.close();
  } else {
    Serial.println("no exists");
  }
}

void handleDelete() { 
  handleFileDelete(server.uri());
}

void handleFileDelete(String path) {
  Serial.println("Trying to delete");
  if (fileSystem->exists(path)) {
    if(fileSystem->remove(path)) {
      sendOk();
    }else{
      sendBadRequest();
    }
  } else {
    Serial.println("no exists");
  }
}

String deploymentFilename(String filename) {
  return filename.startsWith("/") ? String(DEPLOYMENT_DIR) + filename : String(DEPLOYMENT_DIR )+ String('/') + filename;
}

File uploadFile;

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = deploymentFilename(server.uri() + String('/') + upload.filename);
    Serial.print("Handle File Upload: ");
    Serial.println(filename);
     uploadFile = fileSystem->open(filename, "w"); 
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.println("File size: " + String(upload.totalSize));
      server.send(200, "text/plain", "File successfully uploaded");
    } else {
      server.send(500, "text/plain", "Failed to create file");
    }
  }
}

void ensureSchedule(String scheduleFile) {
  if(!fileSystem->exists(scheduleFile)) {
    Serial.print(scheduleFile);
    Serial.println(" does not exist, creating.");
    DynamicJsonDocument doc(256);
    JsonObject open = doc.createNestedObject("open"); 
    open["Default"] = DEFAULT_OPEN_HOUR*100;
    open["Sun"] =  (char*)0;
    open["Mon"] =  (char*)0;
    open["Tue"] =  (char*)0;
    open["Wed"] =  (char*)0;
    open["Thu"] =  (char*)0;
    open["Fri"] =  (char*)0;
    open["Sat"] =  (char*)0;
    JsonObject close = doc.createNestedObject("close"); 
    close["Default"] = DEFAULT_CLOSE_HOUR*100;
    close["Sun"] =  (char*)0;
    close["Mon"] =  (char*)0;
    close["Tue"] =  (char*)0;
    close["Wed"] =  (char*)0;
    close["Thu"] =  (char*)0;
    close["Fri"] =  (char*)0;
    close["Sat"] =  (char*)0;
    File scheduleFileHandle = fileSystem->open(scheduleFile, "w"); 
    serializeJson(doc, scheduleFileHandle);
    scheduleFileHandle.close();
  }
}

void stop() {
  Serial.println("STOPPING!");
  while(1);
}