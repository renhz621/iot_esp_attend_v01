//一次提交
#include "./esppl_functions.h"
// 以下加入连接
//如果测试一直卡在MQTT连接上，请使用PubSubClient库的2.7版本（资源文件夹下有压缩包文件），目前测试最新版2.8无法连接
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define WIFI_SSID "TP-LINK_E138"       //wifi名称
#define WIFI_PASSWORD "1234567890"   //wifi密码
#define device_id "664d954cecb67c02e4270249_myNodeId"       //注册设备的ID和密钥
#define secret "1234567890"    
//MQTT三元组
#define ClientId "664d954cecb67c02e4270249_myNodeId_0_0_2024052400"
#define Username "664d954cecb67c02e4270249_myNodeId"
#define Password "01e4374a0acf8981708cac0ca017c02837554a2b8153b9083723551cabf45784"
#define MQTT_Address "d47d8dfc0b.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define MQTT_Port 1883
#define Iot_link_Body_Format "{\"services\":[{\"service_id\":\"smokeDetector\",\"temperature\":{%s" //注意修改自己的服务ID
//{"services":[{"service_id":"Dev_data","properties":{"temp": 39}}]}
//设备属性上报
#define Iot_link_MQTT_Topic_Report "$oc/devices/664d954cecb67c02e4270249_myNodeId/sys/properties/report"
//接收平台下发的命令
#define Iot_link_MQTT_Topic_Commands "$oc/devices/664d954cecb67c02e4270249_myNodeId/sys/commands/#" 
//设备响应平台的命令 没用到
#define Iot_link_MQTT_Topic_CommandsRes "$oc/devices/664d954cecb67c02e4270249_myNodeId/sys/commands/response/request_id="
WiFiClient myesp8266Client;
PubSubClient myclient(myesp8266Client);
int data_temp=1;
long lastMsg = 0;

#define LEDPIN 0
//以上加入连接


/*
 * Define you friend's list size here
 */
#define LIST_SIZE 4
/*
 * This is your friend's MAC address list
 */
 //0xae,0x74,0x2b,0x99,0x56,0x98
 //0x88,0x25,0x93,0x00,0xE1,0x38 TP
 //0x10,0x60,0x4B,0x78,0x86,0x82 PC
 //0x3C,0xA5,0x81,0x1E,0xB0,0xF2 VIVO
 //0x74,0x59,0x09,0xFB,0xA8,0xFC navi mac
 //0x52,0x0F,0x93,0xE3,0x27,0x10  iQOO-Neo5
/*  本机地址?
        macaddr[1] = 0xba;
        macaddr[2] = 0x7a;
        macaddr[3] = 0xb1;
        macaddr[4] = 0xe0;
        macaddr[5] = 0x42;
        */
uint8_t friendmac[LIST_SIZE][ESPPL_MAC_LEN] = {
   {0x3C,0xA5,0x81,0x1E,0xB0,0xF2}
  ,{0x74,0x59,0x09,0xFB,0xA8,0xFC}
  ,{0x74,0x59,0x09,0xFB,0xA8,0xFC}
  ,{0x00,0x59,0x09,0xFB,0xA8,0xFC}
  };
/*
 * This is your friend's name list
 * put them in the same order as the MAC addresses
 */
String friendname[LIST_SIZE] = {
   "Friend vivo"
  ,"Friend nova","iQOO-Neo5 ","Friend 2"
  };

bool maccmp(uint8_t *mac1, uint8_t *mac2) {
  for (int i=0; i < ESPPL_MAC_LEN; i++) {
    if (mac1[i] != mac2[i]) {
      return false;
    }
  }
  return true;
}

void cb(esppl_frame_info *info) {
   Serial.printf(" here is channel :%d)", info->channel); //新增 打印 channel
  for (int i=0; i<LIST_SIZE; i++) {
    if (maccmp(info->sourceaddr, friendmac[i]) || maccmp(info->receiveraddr, friendmac[i])) {
      Serial.printf("\n%s is here! :)", friendname[i].c_str());
    }
  }
}

////

void setup() {
//WIFI连接成功后，

///////////加入连接
  // put your setup code here, to run once:
  Serial.begin(19200);
  pinMode(LEDPIN,OUTPUT);
  digitalWrite(LEDPIN,HIGH);  //LED低电平触发,高电平熄灭
  WIFI_Init();//连接有错，则死循环
//没有指定channel 试扫描


  //MQTT_Init();  把初始化应放在loop中
  ///////
  //原有
  delay(500);
  //Serial.begin(115200);
  esppl_init(cb);//设置回调函数
  //原有
}

void loop() {
  esppl_sniffing_start();//入esppl_function 置esppl_sniffing_enabled=1
  //定时5分钟扫描
      //esppl_set_channel(6);
      Serial.printf("curent channel is \n");
      while (esppl_process_frames()) { //list不为0，则delay(),直到回调处理完输出，退出while 改换下一频点;
        //
      }
      delay(5000);


////////////////// 上传信息
  if (!myclient.connected()){
     Serial.printf("curent IN mqTT");
    MQTT_Init();
  } 
  else myclient.loop();
  long now = millis();
  if (now - lastMsg > 5000) { //5秒上传一次
    lastMsg = now;
    MQTT_POST();
    data_temp++;  //模拟温度值
  }
//////////////
}

//////////
//连接wifi
void WIFI_Init() //没有连接上则循环连接 不返回
{
    WiFi.mode(WIFI_STA);//模式
    WiFi.begin(WIFI_SSID,WIFI_PASSWORD);//开始连接WIFI名，密码
    while(WiFi.status()!=WL_CONNECTED) //循环连接 不返回
    {
      delay(1000);
      Serial.println("WiFi Not Connect");
    }
    Serial.println("WiFi Connected OK!");
}
///////////////2

//**********************函数1
//连接MQTT
void MQTT_Init()
{
  myclient.setServer(MQTT_Address,MQTT_Port);     //设置云地址和端口            
  while(!myclient.connected())                    // 执行连接，返回？
  {
    if(myclient.connect(ClientId,Username,Password)){
      Serial.print("mqtt ok\n");
    } else{
           Serial.print(myclient.state());
           delay(5000);
    }
  }
   myclient.setCallback(callback);//设定回调方式，当ESP8Iot_link_Body_Format266收到订阅消息时会调用此方法
  boolean res = myclient.subscribe(Iot_link_MQTT_Topic_Commands); //连接成功时订阅主题commands  
  if(res != true){                                  //订阅错误
     Serial.printf("mqtt subscribe topic [%s] fail\n", Iot_link_MQTT_Topic_Commands);
  }
  Serial.printf("mqtt subscribe topic [%s] ok\n", Iot_link_MQTT_Topic_Commands);
}
////
void MQTT_POST()
{
  char properties[32];
  char jsonBuf[128];
  sprintf(properties,"\"temp\":%d}}]}",data_temp);
  sprintf(jsonBuf,Iot_link_Body_Format,properties);

  myclient.publish(Iot_link_MQTT_Topic_Report, jsonBuf);
  Serial.println(Iot_link_MQTT_Topic_Report);
  Serial.println(jsonBuf);
  Serial.println("MQTT Publish OK!");
}
//////////////////3
void callback(char* topic, byte* payload, unsigned int length)
{
  String recdata="";
  Serial.printf("接收到订阅的消息:主题为：");
  Serial.println(topic); 
  Serial.printf("数据内容：");
  for(int i=0;i<length;i++)
  {
    recdata+=(char)payload[i];
  }
  Serial.println(recdata);
  //解析JSON数据
  DynamicJsonDocument jsonBuffer(1024);
  deserializeJson(jsonBuffer,recdata);
  JsonObject obj = jsonBuffer.as<JsonObject>();
  String com = obj["paras"];
  Serial.printf("解析命令:");
  Serial.println(com);
  deserializeJson(jsonBuffer,com);
  obj = jsonBuffer.as<JsonObject>();
  String ledcom = obj["led"];
  Serial.printf("解析LED命令:");
  Serial.println(ledcom);
  //解析request id，设备响应时的topic需要包含命令的request id，且会动态变化
  char *p=topic;
  String request_id="";
  int i=0;
  while((*p++)!='\0')//获取topic长度
  {
    i++;
  }
  topic+=(i-36);    //移动指针位置
  for(int j=i-36;j<i;j++)//末36位为request id
    request_id+=*topic++;
  Serial.println("request_id："+request_id);
  Serial.println("命令设备响应");
  String param="{}";
  myclient.publish((Iot_link_MQTT_Topic_CommandsRes+request_id).c_str(),param.c_str());
  if(ledcom=="0")
  {
    digitalWrite(LEDPIN,HIGH);  //LED低电平触发,高电平熄灭
    Serial.println("关灯");  
  }
  else if(ledcom=="1")
  {
    digitalWrite(LEDPIN,LOW);  //LED低电平触发,高电平熄灭
    Serial.println("开灯");    
  }  
}
/////////////////3END
