#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Esp.h>
#include <Servo.h>

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
using ace_time::clock::NtpClock;
using ace_time::zonedbx2025::kZoneAmerica_Chicago;

static BasicZoneProcessor chicagoProcessor;
static NtpClock ntpClock;

ESP8266WebServer server(80);
Servo servo;

static bool fsOK;


FS* fileSystem = &LittleFS;
LittleFSConfig fileSystemConfig = LittleFSConfig();

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


  fileSystemConfig.setAutoFormat(false);
  fileSystem->setConfig(fileSystemConfig);

  fsOK = fileSystem->begin();
  Serial.println(fsOK ? F("Filesystem initialized.") : F("Filesystem init failed!"));
  if(!fsOK) {
    stop();
  }
  
  doFS();
  
  server.on("/api/open", []() {
    doOpen();
    sendOk();
  });
  server.on("/api/close", []() {
    doClose();
    sendOk();
  });
  server.on("/api/fs", []() {
    doFS();
    sendOk();
  });

  server.on("/", HTTP_POST, []() {
    server.send(200, "text/plain", "Upload handled"); // Send a basic response
  }, handleFileUpload); 

  server.on(UriRegex("/.*"), HTTP_DELETE, handleDelete);


  server.on(UriRegex("/.*"), HTTP_GET, handleGet);
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

void doFS() {
  Serial.println("Start doFS");
  FSInfo fs_info;
  fileSystem->info(fs_info);
  Serial.print("Total bytes ");
  Serial.println(fs_info.totalBytes);
  Serial.print("Used bytes ");
  Serial.println(fs_info.usedBytes);

  listDir("/",0);
  Serial.println("End doFS");
}

void listDir(String path, int indent) {
  Dir dir = fileSystem->openDir(path);
  while (dir.next()) {
    char fileType;
    Serial.printf("%*s", indent, "");
    
    if(dir.isDirectory()) {
      fileType = 'D';
    }
    if(dir.isFile()) {
      fileType = 'F';
    }   
    
    Serial.printf("%c - %s %u\n", fileType, dir.fileName().c_str(), dir.fileSize());

    if(dir.isDirectory()) {
      listDir(dir.fileName(),indent+2);
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
    String filename = deploymentFilename(upload.filename);
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

void stop() {
  Serial.println("STOPPING!");
  while(1);
}