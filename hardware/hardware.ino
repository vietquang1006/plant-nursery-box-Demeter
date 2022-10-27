#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>

/*
   oke. han mach roi test toan bo thoi
   may bom thi lam the nao? het cong GPIO roi?
   se dung chan D4 de dk may bom, nhưng giờ mạch về rồi. không sửa được
   thì tìm cách câu dây thôi.
   kha nang cao se phai noi cung mot thiet bi nao do khac
*/
#define USE_SERIAL Serial
#define DHTTYPE DHT22
// SENSOR
#define DHTPIN 4    // D2
#define LIGHT_PIN 5 // D1
#define CO2_PIN 0   // D3
#define SOIL_PIN 15 // D8
// RELAY
#define BOM_PIN D4   // may bom
#define LED_PIN 14   // D5
#define QUAT1_PIN 12 // D6
#define QUAT2_PIN 13 // D7

// variable
bool blink_state = false;
float DHT_tem = 0;
float DHT_hum = 0;
float CO2 = 0;
float LIGHT = 0;
float SOIL = 0;
float tb_co2[5];
float tb_soil[5];
float tb_light[5];
const uint32_t connectTimeoutMs = 5000;
unsigned long lastMessage = 0;
unsigned long lastRead = 0;

DHT dht(DHTPIN, DHTTYPE);
ESP8266WiFiMulti wifiMulti;
SocketIOclient socketIO;

void setup()
{
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(CO2_PIN, OUTPUT);
    pinMode(SOIL_PIN, OUTPUT);
    pinMode(BOM_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(QUAT1_PIN, OUTPUT);
    pinMode(QUAT2_PIN, OUTPUT);
    Serial.begin(115200);
    //  Serial.setDebugOutput(true);
    Serial.println("***************HOP UOM MAM CAY***************");
    init_dht();
    if (WiFi.getMode() & WIFI_AP)
    {
        WiFi.softAPdisconnect(true);
    }
    wifiMulti.addAP("AmericanStudy T1", "66668888");
    //  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
    //  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

    while (wifiMulti.run() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    // server address, port and URL
    socketIO.begin("192.168.100.12", 3600, "/socket.io/?EIO=4"); // /socket.io/?EIO=4
    socketIO.onEvent(socketIOEvent);
}

int i = 0;
void loop()
{
    socketIO.loop();
    //  maintain_connection();

    // do doc moi cam bien het 100ms, nen 700 thoi la du 1000ms
    if (millis() - lastRead > 700)
    {
        readSensor();
        lastRead = millis();
    }

    // doan nay dung de gui cac tin nhan len server
    if (millis() - lastMessage > 5000)
    {
        sendData(CO2, LIGHT, SOIL, DHT_tem, DHT_hum);
        lastMessage = millis();
    }
}
// FUNCTION
float trungBinh(float *dulieu, float tb)
{
    for (int j = 0; j <= 5; j++)
    {
        tb += dulieu[j];
    }
    return tb = tb / 5;
}
void readSensor()
{
    tb_co2[i] = readCO2();
    tb_light[i] = readLIGHT();
    tb_soil[i] = readSOIL();
    DHT_tem = read_dht_tem();
    DHT_hum = read_dht_hum();
    i++;
    if (i == 5)
    {
        CO2 = trungBinh(tb_co2, CO2);
        LIGHT = trungBinh(tb_light, LIGHT);
        SOIL = trungBinh(tb_soil, SOIL);
        relayControl(LIGHT, DHT_tem, DHT_hum, SOIL);
        i = 0;
    }
}
void relayControl(float LIGHT, float DHT_tem, float DHT_hum, float SOIL)
{
    if (DHT_hum <= 60)
    {
        digitalWrite(QUAT1_PIN, HIGH); // can test lai xem no mo hay dong
        sendMessage("QUAT_on", "0");
    }
    else
    {
        digitalWrite(QUAT1_PIN, LOW);
        sendMessage("QUAT_off", "0");
    }
    if (LIGHT <= 400)
    {
        digitalWrite(LED_PIN, HIGH); // can test lai xem no mo hay dong
        sendMessage("LED_on", "0");
    }
    else
    {
        digitalWrite(LED_PIN, LOW);
        sendMessage("LED_off", "0");
    }
    if (DHT_tem >= 30)
    {
        digitalWrite(QUAT2_PIN, HIGH); // can test lai xem no mo hay dong
        sendMessage("PHUNSUONG_on", "0");
    }
    else
    {
        digitalWrite(QUAT2_PIN, LOW);
        sendMessage("PHUNSUONG_off", "0");
    }
    if (SOIL <= 50)
    {
        digitalWrite(BOM_PIN, HIGH); // can test lai xem no mo hay dong
        //    sendMessage("PHUNSUONG_on", "0");
    }
    else
    {
        digitalWrite(BOM_PIN, LOW);
        //    sendMessage("PHUNSUONG_off", "0");
    }
}

void sendMessage(String topic, String msg)
{
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    array.add(topic);
    array.add(msg);
    String output;
    serializeJson(doc, output);
    socketIO.sendEVENT(output);
}

void sendData(float CO2, float LIGHT, float SOIL, float DHT_tem, float DHT_hum)
{
    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    // ten topic la:
    array.add("message");
    // them noi dung tin nhan
    JsonObject param1 = array.createNestedObject();
    param1["CO2"] = String(CO2, 2);
    param1["LIGHT"] = String(LIGHT, 2);
    param1["SOIL"] = String(SOIL, 2);
    param1["DHT_TEM"] = String(DHT_tem, 2);
    param1["DHT_HUM"] = String(DHT_hum, 2);
    // JSON to String (serializion)
    String output;
    serializeJson(doc, output);
    // gui tin nhan di
    socketIO.sendEVENT(output);
    // in ra de quan sat
    Serial.println(output);
    CO2 = 0;
    SOIL = 0;
    LIGHT = 0;
}

// va test no
void sendMessageWhenUpdate()
{
    float LIGHT_new = readLIGHT();
    float SOIL_new = readSOIL();
    float DHT_tem_new = read_dht_tem();
    float DHT_hum_new = read_dht_hum();
    float CO2_new = readCO2();
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    // ten topic la:
    array.add("message");
    // them noi dung tin nhan
    JsonObject param1 = array.createNestedObject();
    param1["CO2"] = CO2_new;
    param1["LIGHT"] = LIGHT_new;
    param1["SOIL"] = SOIL_new;
    param1["DHT_TEM"] = DHT_tem_new;
    param1["DHT_HUM"] = DHT_hum_new;
    // JSON to String (serializion)
    String output;
    serializeJson(doc, output);
    socketIO.sendEVENT(output);
    Serial.println(output);
    CO2_new = 0;
    SOIL_new = 0;
    LIGHT_new = 0;
}

// web socket event
#define USE_SERIAL Serial
void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
    String text1 = (char *)payload;
    switch (type)
    {
    case sIOtype_DISCONNECT:
        USE_SERIAL.printf("[IOc] Disconnected!\n");
        break;
    case sIOtype_CONNECT:
        USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);
        // join default namespace (no auto join in Socket.IO V3)
        socketIO.send(sIOtype_CONNECT, "/");
        break;
    case sIOtype_EVENT:
        if (text1.startsWith("[\"UPDATE\""))
        {
            USE_SERIAL.println("updating");
            sendMessageWhenUpdate();
        }
        if (text1.startsWith("[\"LED_on\""))
        {
            USE_SERIAL.println("bat led");
            digitalWrite(LED_PIN, LOW);
        }
        if (text1.startsWith("[\"LED_off\""))
        {
            USE_SERIAL.println("tat led");
            digitalWrite(LED_PIN, HIGH);
        }
        if (text1.startsWith("[\"QUAT_on\""))
        {
            USE_SERIAL.println("bat quat");
            digitalWrite(QUAT1_PIN, LOW);
        }
        if (text1.startsWith("[\"QUAT_off\""))
        {
            USE_SERIAL.println("tat quat");
            digitalWrite(QUAT1_PIN, LOW);
        }
        if (text1.startsWith("[\"PHUNSUONG_on\""))
        {
            USE_SERIAL.println("bat phun suong");
            digitalWrite(QUAT2_PIN, LOW);
        }
        if (text1.startsWith("[\"PHUNSUONG_off\""))
        {
            USE_SERIAL.println("tat phun suong");
            digitalWrite(QUAT2_PIN, LOW);
        }
        break;
    case sIOtype_ACK:
        USE_SERIAL.printf("[IOc] get ack: %u\n", length);
        hexdump(payload, length);
        break;
    case sIOtype_ERROR:
        USE_SERIAL.printf("[IOc] get error: %u\n", length);
        hexdump(payload, length);
        break;
    case sIOtype_BINARY_EVENT:
        USE_SERIAL.printf("[IOc] get binary: %u\n", length);
        hexdump(payload, length);
        break;
    case sIOtype_BINARY_ACK:
        USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
        hexdump(payload, length);
        break;
    }
}

// wifi init
void init_wifi()
{
    if (WiFi.getMode() & WIFI_AP)
    {
        WiFi.softAPdisconnect(true);
    }
    wifiMulti.addAP("AmericanStudy T1", "66668888");
    //  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
    //  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

    while (wifiMulti.run() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// maintain connection
void maintain_connection()
{
    if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED)
    {
        socketIO.loop();
    }
    else
    {
        while (1)
        {
            delay(2000);
            Serial.println("Wait WIFI connection");
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.println("WIFI connected, wait 5 secs. ");
                delay(5000);
                Serial.println("WIFI connected, websocket subscribe NODE.");
                break;
            }
        }
    }
}

/* init dht*/
void init_dht()
{
    dht.begin();
    Serial.println("DHT init xong");
}
/* read dht humidity*/
float read_dht_hum()
{
    return dht.readHumidity();
}
/* read dht temperature*/
float read_dht_tem()
{
    return dht.readTemperature();
}
// read CO2
int readCO2()
{
    digitalWrite(CO2_PIN, HIGH);
    digitalWrite(SOIL_PIN, LOW);
    digitalWrite(LIGHT_PIN, LOW);
    int temp = analogRead(A0);
    delay(100);
    return temp;
}
// read LIGHT
float readLIGHT()
{
    digitalWrite(CO2_PIN, LOW);
    digitalWrite(SOIL_PIN, LOW);
    digitalWrite(LIGHT_PIN, HIGH);
    int temp = analogRead(A0);
    delay(100);
    return (1023 - exp(0.007251 * temp));
    //  return (1023 - temp) / 10.23;
}
// read SOIL
float readSOIL()
{
    digitalWrite(CO2_PIN, LOW);
    digitalWrite(SOIL_PIN, HIGH);
    digitalWrite(LIGHT_PIN, LOW);
    int temp = analogRead(A0);
    delay(100);
    return (100 - (temp * 100 / 1023));
}