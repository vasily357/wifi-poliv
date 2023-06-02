// Светится, Режим AP (точки доступа)
// Два сенсора, два реле
#define Version "v.5.0b1-D1-R2 0423"

// Подкллючаемые библиотеки
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESPTemplateProcessor.h>
 
//Variables
int i = 0;
int statusCode;
const char* ssid = "text";
const char* passphrase = "text";
String st;
String content;
 
 
//Function Decalration
bool testWifi(void);
void launchWeb(void);
void launchWWW(void);
void setupAP(void);
 
//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

//Time Contriller Worck
//String CSec = "0";
//String CMin = "0";
//String CHou = "0";
String CTime = "0";

//Большие переменные для большого таймера, занимают много места в памяти.
unsigned long Time; //msec
unsigned long DisconectTimer = 65000; int N = 0;//Для автоотключения.
unsigned long PassTimer = 65000; int PassOK = 0;
//Для сенсоров
unsigned long SensorTimer = 7000; //msec
int SensorLongTime = 10; //min
int SensorPower1 = 4;
int SensorPower2 = 14;
int Sensor0 = A0;
int  S1;
int  S2;
//float S1t = 0;
//float S2t = 0;
int Low1 = 512; //Порог срабатывания реле 1, читается из eeprom
int Low2 = 512;

//Для реле
int In1 = 0;//Реле 1, pin
int In1On = 0;//Индикатор реле 1
int In1Time = 10; //Ручное или Авто sec
unsigned long In1Timer = 10000; //Таймер msec.
int In1LongTime = 11; //Время в минутах между включениями читается из eeprom
int In1AutoTime = 30; //Время автоматического включения, макс 60 сек.
int In1ManualTime = 60; //Время ручного включения, макс 240 сек.
int In1Stat = 0; //Статистика включений In1 с момента включения питания
int In1Invert = 0; 

int In2 = 2;//Реле 2, pin
int In2On = 0;//Индикатор реле 2
int In2Time = 10;
unsigned long In2Timer = 10000;//Таймер msec.
int In2LongTime = 11; //Время в минутах между включениями читается из eeprom
int In2AutoTime = 30; //Время автоматического включения, макс 60 сек.
int In2ManualTime = 60; //Время автоматического включения.
int In2Stat = 0; //Статистика включений In2 с момента включения питания
int In2Invert = 0;
 
void setup() {
  SPIFFS.begin();
  Serial.begin(115200); //Initialising if(DEBUG)Serial Monitor
  Serial.println();
  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(5);
  //pinMode(LED_BUILTIN, OUTPUT);
pinMode(In1, OUTPUT);
pinMode(In2, OUTPUT);
pinMode(SensorPower1, OUTPUT);
pinMode(SensorPower2, OUTPUT);
digitalWrite(In1, HIGH);
digitalWrite(In2, HIGH);
digitalWrite(SensorPower1, LOW);
digitalWrite(SensorPower2, LOW);
//digitalWrite(LED_BUILTIN, HIGH);
  Serial.println();
  Serial.println();
  Serial.println("Startup");

 if (analogRead (Sensor0) <= 24) {PassOK = 2; PassTimer = 1000 * 60 * 3; for (int i = 0; i < 33; ++i) {digitalWrite(LED_BUILTIN, LOW); delay(15); digitalWrite(LED_BUILTIN, HIGH); delay(200);} ;}; // Password reset
 
  //---------------------------------------- Read EEPROM for SSID and pass
  DataReadEEPROM();
  Serial.println("Reading EEPROM ssid");
  Serial.println(LED_BUILTIN);
 
  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
 
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
 
 
  WiFi.begin(esid.c_str(), epass.c_str());
  if (testWifi())
  {
    Serial.println();
    Serial.println("Succesfully Connected!!!");
    launchWWW();
    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }
 
  Serial.println();
  Serial.println("Waiting.");
  
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(100);
    loop2();
    server.handleClient();
  }
 
}
void loop() {
loop2();

server.handleClient();

} //end loop

void loop2()
{
if (millis() <= 2000) { PassTimer = 1000 * 60 * 3; SensorTimer = 7000; DisconectTimer = 1000*60*3; In1Timer = 14; In2Timer = 14;}; //На всякий случай обнуляшка таймеров, после обнуления таймера.
//digitalWrite(LED_BUILTIN, HIGH);
if (N == 1 && millis() >= DisconectTimer) {N = 0; Serial.print(" Connect SSID"); setup(); };
if (millis() >= PassTimer && (PassOK == 1 || PassOK == 2 || PassOK == 3)) { PassOK = 0; }; 
Sensor();
RelayIn1();
RelayIn2();
} //end loop2

//Sensor read code
void Sensor ()
{
if (millis() >= SensorTimer && digitalRead(SensorPower1) == LOW && digitalRead(SensorPower2) == LOW) {digitalWrite(SensorPower1, HIGH); SensorTimer = millis() + 1500;};
if (millis() >= SensorTimer && digitalRead(SensorPower1) == HIGH) { Serial.print("Sensor 1 Read: "); S1 = analogRead (Sensor0); delay(15); S1 = S1 + analogRead (Sensor0); delay(60); S1 = S1 + analogRead (Sensor0); S1 = S1 / 3 ; digitalWrite(SensorPower1, LOW); Serial.println(S1); digitalWrite(SensorPower2, HIGH); SensorTimer = millis() + 1500;};
if (millis() >= SensorTimer && digitalRead(SensorPower2) == HIGH) { Serial.print("Sensor 2 Read: "); S2 = analogRead (Sensor0); delay(15); S2 = S2 + analogRead (Sensor0); delay(60); S2 = S2 + analogRead (Sensor0); S2 = S2 / 3 ; digitalWrite(SensorPower2, LOW); Serial.println(S2); SensorTimer = millis() + 1000 * 60 * SensorLongTime;};
}

//In1 Play
void RelayIn1 ()
{
if (millis() >= In1Timer && S1 >= Low1 && S1 <= 1000 &&  millis() <= SensorTimer && In1On == 0 && In2On == 0) {Serial.println("In1 Start "); digitalWrite(In1, LOW); In1On = 1; In1Stat = In1Stat + 1; In1Timer = millis() + 1000 * In1Time; SensorTimer = millis() + 1000 * (In1Time / 3) * 2;};
if (millis() >= In1Timer && In1On == 1) {Serial.println("In1 Stop"); digitalWrite(In1, HIGH); In1On = 0; In1Timer = millis() + 1000 * 60 * In1LongTime; In1On = 0; In1Time = In1AutoTime; SensorTimer = millis() + 1000 * 5; In1On = 0;};
if (S1 <= Low1 && In1On == 1) {Serial.println("In1 Stop Level"); digitalWrite(In1, HIGH); In1Timer = millis() + 1000 * 60 * In1LongTime; In1Time = In1AutoTime; In1On = 0;};
}

//In2 Play
void RelayIn2 ()
{
if (millis() >= In2Timer && S2 >= Low2 && S2 <= 1000 && millis() <= SensorTimer && In1On == 0 && In2On == 0) {Serial.println("In2 Start "); digitalWrite(In2, LOW); In2On = 1; In2Stat = In2Stat + 1; In2Timer = millis() + 1000 * In2Time; SensorTimer = millis() + 1000 * (In2Time / 3) * 2;};
if (millis() >= In2Timer && In2On == 1) {Serial.println("In2 Stop"); digitalWrite(In2, HIGH); In2On = 0; In2Timer = millis() + 1000 * 60 * In2LongTime; In2On = 0; In2Time = In2AutoTime; SensorTimer = millis() + 1000 * 5; In2On = 0;};
if (S2 <= Low2 && In2On == 1) {Serial.println("In2 Stop Level"); digitalWrite(In2, HIGH); In2Timer = millis() + 1000 * 60 * In2LongTime; In2Time = In2AutoTime; In2On = 0;};
}

void In1ManualStart ()
{
  if (In1On == 0 && In2On == 0 && PassOK == 1) {
  In1Timer = millis();
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 1 Start. </a></h2></p>";
  };
  if (In2On == 1){
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 1 No Start. Worck Relay 2, Wait "; int X = (In2Timer - millis()) / 1000; content += X; content += " sec.</a></h2></p>";
  }
  if (In1On == 1) {
  In1Timer = millis();
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 1 Stop. </a></h2></p>";
  };
  if (PassOK == 0) {
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 1 No Start Press Password. </a></h2></p>";
  };
  content += "<head><META HTTP-EQUIV='Refresh' CONTENT='5; URL=/'></head>";
  server.send(200, "text/html", content);
}
void In2ManualStart ()
{
  if (In2On == 0 && In1On == 0 && PassOK == 1) {
  In2Timer = millis();
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 2 Start. </a></h2></p>";
  };
  if (In1On == 1){
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 2 No Start. Worck Relay 1, Wait ";  int X = (In1Timer - millis()) / 1000; content += X; content += " sec.</a></h2></p>";
  };
  if (In2On == 1) {
  In2Timer = millis();
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 2 Stop. </a></h2></p>";
  };
  if (PassOK == 0) {
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 2 No Start Press Password. </a></h2></p>";
  };
  content += "<head><META HTTP-EQUIV='Refresh' CONTENT='5; URL=/'></head>";
  server.send(200, "text/html", content);
}
//-------- Fuctions used for WiFi credentials saving and connecting to it which you do not need to change 
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}
 
void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  launchWWW();
  // Start the server
  server.begin();
  Serial.println("Server started");
}
 
void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
 
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP(Version, "");
  Serial.println("softap");
  launchWeb();
  Serial.println("S1ter");
  Serial.println(WiFi.status());
}
 
void WiFiSetUp()
{//digitalWrite(LED_BUILTIN, LOW);
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      WorkTime();
      content = "<!DOCTYPE HTML>\r\n<html><h2><span style='color: #003366;'><a title='Press click to home page.' href='/\'>WiFi Drinks Controller. </h2></span></a>";
      content += "AP IP: ";
      content += ipStr;
      //content += "<td><form method='POST' title='Clic thiz to disconect curent WiFi' action='/scan'><input type='submit' value='Scan WiFi'></form></td>";
      content += "<p>";
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><label>PASS: </label><input name='pass' length=64><input title='Clic thiz to save SSID ann Password to EEPROM' type='submit'></form>";
      content += "<p>";
      content += "<form method='POST' title='If you clic thiz to disconect curent WiFi, crate AP' action='/scan'><input type='submit' value='Scan WiFi'></form>";
      content += st;
      content += "</p>";
      content += "</html>";
      server.send(200, "text/html", content);
}

void scan ()
{//digitalWrite(LED_BUILTIN, LOW);
      if (WiFi.status() == 3) {
      content = "<!DOCTYPE HTML>\r\n<html><h3><a title='Clic thiz linc to after connect to default SSID' href='http://192.168.4.1'>Reconnect to default SSID: "; content += Version; content += " After connect, go to address http://192.168.4.1</a></h3>";
      content += "</html>";
      server.send(200, "text/html", content);
      delay(500);
      DisconectTimer = millis() + 1000 * 60 * 3; N = 1;
      }
      setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html> <h3><span style='color: #003300;'><a title='Clic thiz linc to updata home page' href='/WiFiSetUp\'>Scan complite. Press updata page.</a></span></h3>";
      content += "</html>";
      server.send(200, "text/html", content);
}

void setting ()
{
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
 
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
 
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content); 
}

void launchWWW()
{
  server.begin();//Запускаем сервер
  Serial.println("Server listening");
  Serial.print("WiFi.status:");
  Serial.println(WiFi.status());
      Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  server.on("/", RootPath);//Свяжем функцию обработчика с путем
  server.on("/sens", Sens);
  server.on("/info", Info);
  server.on("/PassReadEEPROM", PassReadEEPROM);
  server.on("/In1ManualStart", In1ManualStart);
  server.on("/In2ManualStart", In2ManualStart);
  server.on("/SetUp", SetUp);
  server.on("/SetUpOff", SetUpOff);
  server.on("/SensorSetUp", SensorSetUp);
  server.on("/RelaySetUp", RelaySetUp);
  server.on("/SensorSave", SensorSave);
  server.on("/RelaySave", RelaySave);
  server.on("/In1Invert", _In1Invert);
  server.on("/In2Invert", _In2Invert);
  server.on("/In1InvertSaveEEPROM", In1InvertSaveEEPROM);
  server.on("/In2InvertSaveEEPROM", In2InvertSaveEEPROM);
  server.on("/Low1SaveEEPROM", Low1SaveEEPROM);
  server.on("/Low2SaveEEPROM", Low2SaveEEPROM);
  server.on("/In1AutoTimeSaveEEPROM", In1AutoTimeSaveEEPROM);
  server.on("/In2AutoTimeSaveEEPROM", In2AutoTimeSaveEEPROM);
  server.on("/In1ManualTimeSaveEEPROM", In1ManualTimeSaveEEPROM);
  server.on("/In2ManualTimeSaveEEPROM", In2ManualTimeSaveEEPROM);
  server.on("/In1LongTimeSaveEEPROM", In1LongTimeSaveEEPROM);
  server.on("/In2LongTimeSaveEEPROM", In2LongTimeSaveEEPROM);
  server.on("/SensorLongTimeSaveEEPROM", SensorLongTimeSaveEEPROM);
  server.on("/WiFiSetUp", WiFiSetUp);
  server.on("/PsCh", PsCh);
  server.on("/PassSaveEEPROM", PassSaveEEPROM);
  server.on("/WiFiSetUp", WiFiSetUp); 
  server.on("/scan", scan);
  server.on("/setting", setting);
}

void SetUpOff () 
{
  if (PassOK == 1 || PassOK == 2 || PassOK == 3) { if (PassOK == 1) {PassOK = 2;}; if(PassOK == 3) {PassOK = 1;} if(PassOK == 2) {PassOK = 3;}; RootPath(); 
  } else
  {content = "404 not found";
    Serial.println("Sending 404");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(statusCode, "application/json", content); 
  }
}
void PsCh () 
{
  if (PassOK == 3) {PassOK = 2; RootPath(); 
  } else
  {content = "404 not found";
    Serial.println("Sending 404");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(statusCode, "application/json", content); 
  }
}
void SetUp ()
{
if (PassOK == 1) { PassOK = 3; RootPath(); };
      
}

const char* indexKeyProcessor(const String& key)
{
  char cstr[16];

  if (key == "CTime") return CTime;
  else if (key == "S1") return std::to_string(S1).c_str();
  else if (key == "S2") return S2;
  else if (key == "Low1") return Low1;
  else if (key == "Low2") return Low2;
  else if (key == "In1ManualTime") return In1ManualTime;
  else if (key == "In2ManualTime") return In2ManualTime;
  else if (key == "In1Stat") return In2ManualTime;
  else if (key == "In2Stat") return In2ManualTime;
  return "Key not found";
}

void RootPath()
{//digitalWrite(LED_BUILTIN, LOW);
  if (PassOK == 1) {
    WorkTime();
    // content = "";
    // server.send(200, "text/html", content);
    
    templateProcessor.processAndSend("/index.html", indexKeyProcessor);
    
    Serial.println("Send Home Page");
  };
// Press Pass
  if (PassOK == 0) {
  WorkTime();
  content = "<!DOCTYPE HTML>\r\n<html><h2><span style='color: #003366;'><a title='Press click to update page.' href='/\'>Enter the password for full functionality for five minutes. </h2></span></a>";
  content += CTime; 
  content += "<table style='border-collapse: collapse width: 95%;' 'height: 36px border=0'>";
  content += "<tr style=height: 18px>";
  content += "<td>Sensor 1: </td>";
  content += "<td> "; content += S1; content += "<td>";
  content += "<td>Sensor 2: </td>";
  content += "<td> "; content += S2; content += "<td>";
  content += "<td>"; content += "<form method='POST' action='/sens'><input title='Click and wait to sensor read.' type='submit' value='Sensor Read'></form></td>";
  content += "<td><form method='POST' action='/In1ManualStart'><input title='";
  content += "Relay 1 Manual stop.' type='submit' value='Relay 1 Stop'></form></td>";
  content += "<td><form method='POST' action='/In2ManualStart'><input title='";
  content += "Relay 2 Manual stop.' type='submit' value='Relay 2 Stop'></form></td>";
  content += "</tr>";
  content += "<p><form method='get' action='/PassReadEEPROM\'><label>Password: </label><input name='passOK' length=32><input title='Enter password.' type='submit'></form></P>";
  //content += "</html>";
  content += "<head><META HTTP-EQUIV=Refresh CONTENT=300 URL=/></head>";
  server.send(200, "text/html", content);
  Serial.println("Send pass Page");
  };
// Change pass
  if (PassOK == 2) {
  content = "<!DOCTYPE HTML>\r\n<html><h2><span style='color: #003366;'><a title='Press click to update page.' href='/\'>Pass Change Page. </h2></span></a>";
  content += "<table style='border-collapse: collapse width: 95%;' 'height: 36px border=0'>";
  content += "<tr style=height: 18px>";
  content += "<td>Sensor 1: </td>";
  content += "<td> "; content += S1; content += "<td>";
  content += "<td>Sensor 2: </td>";
  content += "<td> "; content += S2; content += "<td>";
  content += "</tr>";
  content += "<p><form method='get' action='/PassSaveEEPROM\'><label>New Password: </label><input name='npass' length=32><input title='Enter password.' type='submit'></form>";
  content += "<form method='POST' action='/SetUpOff'><input type='submit' value='Exit'></form></p></td>";
  content += "</html>";
  server.send(200, "text/html", content);
  Serial.println("Send new pass Page");
  };
// SetUp page
  if (PassOK == 3) {
      content = "<!DOCTYPE HTML>\r\n<html><h3><a title='Clic thiz linc to go home page.' href='/\'>Setup 1 page.</a></h3>";
      content += CTime;
      content += " "Version;
      content += "<table style='border-collapse: collapse;' 'width: 100%;' 'height: 50%; border=0'>";
      SensorSetUp();
      RelaySetUp();
      content += "<table style='border-collapse: collapse;' 'width: 100%;' 'height: 50%; border: 0px'>";
      content += "<td>"; content +=  "<form method='POST' action='/WiFiSetUp'><input type='submit' value='WiFi SetUp'></form></td>";
      content += "<td><form method='POST' action='/PsCh'><input type='submit' value='Pass change'></form></td>";
      content += "<td><form method='POST' action='/SetUpOff'><input type='submit' value='Exit SetUp'></form></td>";
      content += "</table>";
      content += "</html>";
      server.send(200, "text/html", content);
      Serial.println("Send SetUp Page");
  }
}

 void Info() 
{//digitalWrite(LED_BUILTIN, LOW);
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      //IPAddress ip = WiFi.softAPIP();
      content = "<!DOCTYPE HTML>\r\n<html> <h3><span style='color: #003300;'><a title='Clic thiz linc to go home page' href='/\'>Information page.</a></span></h3>";
      content += "<p>After connecting to your network, the device must receive addressing.</p>";
      content += "<p>MAC: e8-db-84-e1-1c-4f</p>";
      content += "Local IP: "; content += ipStr;
      content += "<p>Which IP address to get the device can also be seen by connecting via USB and configuring the Arduino IDE. The port speed is 115200.</p>";
      content += Version;
      content += "</html>";
      server.send(200, "text/html", content);
      Serial.println("Send Info Page");
}

void Sens ()
{
  Serial.println("Sensor Manual Sens.");
  SensorTimer = millis(); Sensor();
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Sensor resultat update. Wait 5sec. </a></h2></p>";
  content += "<head><META HTTP-EQUIV='Refresh' CONTENT='4; URL=/'></head>";
  server.send(200, "text/html", content);
}

 void SensorSetUp() 
 {//digitalWrite(LED_BUILTIN, LOW);
      content += "<tr style=height: 36px>";
      content += "<td>(0 max. ~0Om),(1024 min. ~100kOm)</td>";
      content += "<td></td>";
      content += "<td></td>";
      content += "<td>Indicator</td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td><form method='get' action='/SensorSave\'><label>Sensor 1: </label><input maxlength='4' size='8' name='Low_1'><input title='Save to power lost.' type='submit' value='Remember'></form></td>";
      content += "<td>"; content += Low1; content += "</td>";
      content += "<td>"; content += "<form method='POST' action='/Low1SaveEEPROM'><input title='Save to EEPROM. 'type='submit' value='Save'></form></td>";
      content += "<td>"; content += S1; content += "</td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td><form method='get' action='/SensorSave\'><label>Sensor 2: </label><input maxlength='4' size='8' name='Low_2'><input title='Save to power lost.' type='submit' value='Remember'></form></td>";
      content += "<td>"; content += Low2; content += "</td>";
      content += "<td>"; content += "<form method='POST' action='/Low2SaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "<td>"; content += S2; content += "</td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td><form method='get' action='/SensorSave\'><label>Interval:  </label><input maxlength='2' size='8' name='SenPolIn'><input title='Save to power lost.' type='submit' value='Remember'></form></td>";
      content += "<td>"; content += SensorLongTime; content += "</td>";
      content += "<td>"; content += "<form method='POST' action='/SensorLongTimeSaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "</tr>";
}
void RelaySetUp()
 {//digitalWrite(LED_BUILTIN, LOW);
      //content += "<table style='border-collapse: collapse width: 100%;' 'height: 72px border=1'>";
      content += "<tr style=height: 36px>";
      content += "<td>Auto play timer, max. 60 sec.</td>";
      content += "<td></td>";
      content += "<td></td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td><form method='get' action='/RelaySave\'><label>Relay 1: </label><input maxlength='2' size='8' name='In1AutoTime_1'><input title='Save to power lost.' type='submit' value='Remember'></form></td>";
      content += "<td>"; content += In1AutoTime; content += "</td>";
      content += "<td>"; content += "<form method='POST' action='/In1AutoTimeSaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td><form method='get' action='/RelaySave\'><label>Relay 2: </label><input maxlength='2' size='8' name='In2AutoTime_2'><input title='Save to power lost.' type='submit' value='Remember'></form></td>";
      content += "<td>"; content += In2AutoTime; content += "</td>";
      content += "<td>"; content += "<form method='POST' action='/In2AutoTimeSaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td>Manual play timer, max. 240 sec.</td>";
      content += "<td></td>";
      content += "<td></td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td><form method='get' action='/RelaySave\'><label>Relay 1: </label><input maxlength='3' size='8' name='In1ManualTime_1'><input title='Save to power lost.' title='Save to power lost.' type='submit' value='Remember'></form></td>";
      content += "<td>"; content += In1ManualTime; content += "</td>";
      content += "<td>"; content += "<form method='POST' action='/In1ManualTimeSaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td><form method='get' action='/RelaySave\'><label>Relay 2: </label><input maxlength='3' size='8' name='In2ManualTime_2'><input title='Save to power lost.' type='submit' value='Remember'></form></td>";
      content += "<td>"; content += In2ManualTime; content += "</td>";
      content += "<td>"; content += "<form method='POST' action='/In2ManualTimeSaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td>Off timer, max. 240 min.</td>";
      content += "<td></td>";
      content += "<td></td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td><form method='get' action='/RelaySave\'><label>Relay 1: </label><input maxlength='3' size='8' name='In1LongTime_1'><input title='Save to power lost.' type='submit' value='Remember'></form></td>";
      content += "<td>"; content += In1LongTime; content += "</td>";
      content += "<td>"; content += "<form method='POST' action='/In1LongTimeSaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "</tr>";
      content += "<tr style=height: 36px>";
      content += "<td><form method='get' action='/RelaySave\'><label>Relay 2: </label><input maxlength='3' size='8' name='In2LongTime_2'><input title='Save to power lost.' type='submit' value='Remember'></form></td>";
      content += "<td>"; content += In2LongTime; content += "</td>";
      content += "<td>"; content += "<form method='POST' action='/In2LongTimeSaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "</tr>";
      
      content += "<tr style=height: 36px>";
      if (In1Invert == 0) {
      content += "<td><form action='/In1Invert\'><label>Relay 1 No Inverted: </label><input title='Invert & Save to power lost.' type='submit' value='Remember'></form></td>";
      } else {
        content += "<td><form action='/In1Invert\'><label>Relay 1    Inverted: </label><input title='Invert & Save to power lost.' type='submit' value='Remember'></form></td>";
      };
      content += "<td>"; content += In1Invert; content += "</td>";
      content += "<td><form action='/In1InvertSaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "</tr>";

      content += "<tr style=height: 36px>";
      if (In2Invert == 0) {
      content += "<td><form action='/In2Invert\'><label>Relay 2 No Inverted: </label><input title='Invert & Save to power lost.' type='submit' value='Remember'></form></td>";
      } else {
        content += "<td><form action='/In2Invert\'><label>Relay 2    Inverted: </label><input title='Invert & Save to power lost.' type='submit' value='Remember'></form></td>";
      }
      content += "<td>"; content += In2Invert; content += "</td>";
      content += "<td><form action='/In2InvertSaveEEPROM'><input title='Save to EEPROM.' type='submit' value='Save'></form></td>";
      content += "</tr>";
      
      //content += "</table>";
      //server.send(200, "text/html", content);
}

void _In1Invert () {
   if (PassOK == 3) {
   if (In1Invert == 0) {In1Invert = 1;
   Serial.print("Relay 1 Invert");
   } else {In1Invert = 0;
   Serial.print("Relay 1 No Invert");
   };
   };
   content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 1 Invert. </a></h2></p>";
   content += "<head><META HTTP-EQUIV='Refresh' CONTENT='5; URL=/'></head>";
   server.send(200, "text/html", content);
}

void _In2Invert () {
   if (PassOK == 3) {
   if (In2Invert == 0) {In2Invert = 1;
   Serial.print("Relay 2 Invert");
   } else {In2Invert = 0;
   Serial.print("Relay 2 No Invert");
   };
   };
   content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Relay 2 Invert. </a></h2></p>";
   content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
   server.send(200, "text/html", content);
}

void SensorSave ()
{if (PassOK == 3) {
   String L1 = server.arg("Low_1");
   if(L1.toInt()) {
   Low1 = L1.toInt();
   if (Low1 >= 1024) {Low1 = 1024;};
   if (Low1 <= 0) {Low1 = 1;};
   };
   String L2 = server.arg("Low_2");
   if(L2.toInt()) {
   Low2 = L2.toInt();
   if (Low2 >= 1024) {Low2 = 1024;};
   if (Low2 <= 0) {Low2 = 1;};
   };
   String S = server.arg("SenPolIn");
   if(S.toInt()) {
   SensorLongTime = S.toInt();
   if (SensorLongTime >= 60) {SensorLongTime = 60;};
   if (SensorLongTime <= 0) {SensorLongTime = 1;};
   };
   RootPath();
};
}
void RelaySave ()
{if (PassOK == 3) {
   String I1AT = server.arg("In1AutoTime_1");
   if(I1AT.toInt()) {
   In1AutoTime = I1AT.toInt();
   if (In1AutoTime >= 60) {In1AutoTime = 60;};
   if (In1AutoTime <= 0) {In1AutoTime = 1;};
   };
   String I2AT = server.arg("In2AutoTime_2");
   if(I2AT.toInt()) {
   In2AutoTime = I2AT.toInt();
   if (In2AutoTime >= 60) {In2AutoTime = 60;};
   if (In2AutoTime <= 0) {In2AutoTime = 1;};
   };

   String I1MT = server.arg("In1ManualTime_1");
   if(I1MT.toInt()) {
   In1ManualTime = I1MT.toInt();
   if (In1ManualTime >= 240) {In1ManualTime = 240;};
   if (In1ManualTime <= 0) {In1ManualTime = 1;};
   };
   String I2MT = server.arg("In2ManualTime_2");
   if(I2MT.toInt()) {
   In2ManualTime = I2MT.toInt();
   if (In2ManualTime >= 240) {In2ManualTime = 240;};
   if (In2ManualTime <= 0) {In2ManualTime = 1;};
   };

   String I1LT = server.arg("In1LongTime_1");
   if(I1LT.toInt()) {
   In1LongTime = I1LT.toInt();
   if (In1LongTime >= 240) {In1LongTime = 240;};
   if (In1LongTime <= 0) {In1LongTime = 1;};
   };   
   String I2LT = server.arg("In2LongTime_2");
   if(I2LT.toInt()) {
   In2LongTime = I2LT.toInt();
   if (In2LongTime >= 240) {In2LongTime = 240;};
   if (In2LongTime <= 0) {In2LongTime = 1;};
   };
      
      RootPath(); 
};
}

void Low1SaveEEPROM ()
{if (PassOK == 3) {
  int bigNum;
  byte hi;
  byte low;
bigNum = Low1;
hi = highByte(bigNum); // старший байт
low = lowByte(bigNum); // младший байт
EEPROM.write(100, hi);  // записываем в ячейку 1 старший байт
EEPROM.write(101, low); // записываем в ячейку 2 младший байт

EEPROM.commit();
content += "<p>Sensor 1 threshold EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content);
};
}

void Low2SaveEEPROM () 
{if (PassOK == 3) {
  int bigNum;
  byte hi;
  byte low;
bigNum = Low2;
hi = highByte(bigNum);
low = lowByte(bigNum);
EEPROM.write(102, hi);
EEPROM.write(103, low);

EEPROM.commit();
content += "<p>Sensor 2 threshold EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content);
};
}

void In1AutoTimeSaveEEPROM ()
{if (PassOK == 3) {
  int bigNum;
  byte hi;
  byte low;
bigNum = In1AutoTime;
hi = highByte(bigNum);
low = lowByte(bigNum);
EEPROM.write(104, hi);
EEPROM.write(105, low);

EEPROM.commit();
content += "<p>Relay 1 Auto play time EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content); 
};
}

void In2AutoTimeSaveEEPROM ()
{if (PassOK == 3) {
 int bigNum;
  byte hi;
  byte low;
bigNum = In2AutoTime;
hi = highByte(bigNum);
low = lowByte(bigNum);
EEPROM.write(106, hi);
EEPROM.write(107, low);

EEPROM.commit();
content += "<p>Relay 2 Auto play time EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content); 
};
}

void In1ManualTimeSaveEEPROM ()
{if (PassOK == 3) {
 int bigNum;
  byte hi;
  byte low;
bigNum = In1ManualTime;
hi = highByte(bigNum);
low = lowByte(bigNum);
EEPROM.write(108, hi);
EEPROM.write(109, low);

EEPROM.commit();
content += "<p>Relay 1 Manual play time EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content);  
};
}

void In2ManualTimeSaveEEPROM ()
{if (PassOK == 3) {
  int bigNum;
  byte hi;
  byte low;
bigNum = In2ManualTime;
hi = highByte(bigNum);
low = lowByte(bigNum);
EEPROM.write(110, hi);
EEPROM.write(111, low);

EEPROM.commit();
content += "<p>Relay 2 Manual play time EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content);
};
}

void In1LongTimeSaveEEPROM ()
{if (PassOK == 3) {
  int bigNum;
  byte hi;
  byte low;
bigNum = In1LongTime;
hi = highByte(bigNum);
low = lowByte(bigNum);
EEPROM.write(112, hi);
EEPROM.write(113, low);

EEPROM.commit();
content += "<p>Relay 1 Off time EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content);
};
}

void In2LongTimeSaveEEPROM ()
{if (PassOK == 3) {
  int bigNum;
  byte hi;
  byte low;
bigNum = In2LongTime;
hi = highByte(bigNum);
low = lowByte(bigNum);
EEPROM.write(114, hi);
EEPROM.write(115, low);

EEPROM.commit();
content += "<p>Relay 2 Off time EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content);
};
}

void SensorLongTimeSaveEEPROM ()
{if (PassOK == 3) {
  int bigNum;
  byte hi;
  byte low;
bigNum = SensorLongTime;
hi = highByte(bigNum);
low = lowByte(bigNum);
EEPROM.write(116, hi);
EEPROM.write(117, low);

EEPROM.commit();
content += "<p>Sensor polling interval EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content);
};
}


void In1InvertSaveEEPROM ()
{if (PassOK == 3) {
  int bigNum;
  byte hi;
  byte low;
bigNum = In1Invert;
hi = highByte(bigNum); // старший байт
low = lowByte(bigNum); // младший байт
EEPROM.write(118, hi);  // записываем в ячейку 1 старший байт
EEPROM.write(119, low); // записываем в ячейку 2 младший байт

EEPROM.commit();
content += "<p>Relay 1 Invert EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content);
};
}

void In2InvertSaveEEPROM ()
{if (PassOK == 3) {
  int bigNum;
  byte hi;
  byte low;
bigNum = In2Invert;
hi = highByte(bigNum); // старший байт
low = lowByte(bigNum); // младший байт
EEPROM.write(120, hi);  // записываем в ячейку 1 старший байт
EEPROM.write(121, low); // записываем в ячейку 2 младший байт

EEPROM.commit();
content += "<p>Relay 1 Invert EEPROM Save.</p>";
content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
server.send(200, "text/html", content);
};
}

void PassSaveEEPROM ()
{if (PassOK == 2) {
        String Pass = server.arg("npass");
        if (Pass.length() > 0) {
        Serial.println("clearing eeprom new pass");
        for (int i = 0; i < 32; ++i) {
          EEPROM.write(200 + i, 0);
        }
        Serial.println(Pass[200 + i]);
        Serial.println("");
        Serial.println("writing eeprom new pass:");
        for (int i = 0; i < Pass.length(); ++i)
        {
          EEPROM.write(200 + i, Pass[i]);
          Serial.print("Wrote: ");
          Serial.println(Pass[i]);
        }
        EEPROM.commit();
      };
        content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Pass Save EEPROM. </a></h2></p>";
        content += "<head><META HTTP-EQUIV='Refresh' CONTENT='3; URL=/'></head>";
        server.send(200, "text/html", content);
        PassOK = 1; 
        } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(404, "application/json", content);
      };
}
void PassReadEEPROM ()
{
  Serial.println("Reading EEPROM pass");
  
  String p1 = server.arg("passOK");
  
  String p2 = "";
  for (int i = 0; i < 9; ++i) {
    p2 += char(EEPROM.read(200 + i));
  }
  Serial.print("pass:");
  Serial.println(p1);
  Serial.print("Pass:");
  Serial.println(p2);
  
  if (p1.compareTo(p2) == 0) { PassOK = 1; PassTimer = millis() + 1000 * 60 * 5;
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Pass OK. </a></h2></p>";
  content += "<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/'></head>";
  server.send(200, "text/html", content);
 } else { PassOK = 0;
  content += "<p><h2><a title='Clic thiz linc if not updata page auto.' href='/\'>Pass incorrect. </a></h2></p>";
  content += "<head><META HTTP-EQUIV='Refresh' CONTENT='11; URL=/'></head>";
  server.send(200, "text/html", content);
  delay(7000);
 };
}

void DataReadEEPROM () {
int a = EEPROM.read(100);
int b = EEPROM.read(101);
int c = word(a, b);
Low1 = c;
a = EEPROM.read(102);
b = EEPROM.read(103);
c = word(a, b);
Low2 = c;
a = EEPROM.read(104);
b = EEPROM.read(105);
c = word(a, b);
In1AutoTime = c;
a = EEPROM.read(106);
b = EEPROM.read(107);
c = word(a, b);
In2AutoTime= c;
a = EEPROM.read(108);
b = EEPROM.read(109);
c = word(a, b);
In1ManualTime= c;
a = EEPROM.read(110);
b = EEPROM.read(111);
c = word(a, b);
In2ManualTime= c;
a = EEPROM.read(112);
b = EEPROM.read(113);
c = word(a, b);
In1LongTime= c;
a = EEPROM.read(114);
b = EEPROM.read(115);
c = word(a, b);
In2LongTime= c;
a = EEPROM.read(116);
b = EEPROM.read(117);
c = word(a, b);
SensorLongTime= c;
a = EEPROM.read(118);
b = EEPROM.read(119);
c = word(a, b);
In1Invert= c;
a = EEPROM.read(120);
b = EEPROM.read(121);
c = word(a, b);
In2Invert= c;
}

void WorkTime ()
{
//Time = millis() / 1000;
// int D = 0;
int H = 0;
int M = 0;
int S = 0;

//if (millis()/1000/60/60<10) { D = 0;}
//D = millis()/1000/60/60/24;

if (millis()/1000/60/60<48) { H = 0;}
H = millis()/1000/60/60;

if (millis()/1000/60%60<59) { M = 0;}

M = (millis()/1000/60)%60;

if (millis()/1000%60<10) { S = 0; };
S = millis()/1000%60;

CTime = " Work Time: ";
//CTime += " d:";
//CTime += D;
CTime += " h:";
CTime += H;
CTime += " m:";
CTime += M;
CTime += " s:";
CTime += S;
Time = millis();
}
