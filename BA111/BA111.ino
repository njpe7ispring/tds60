#include <SoftwareSerial.h>
//#include <ESPeasySerial.h>

const byte rxPin = 2;
const byte txPin = 3;

SoftwareSerial BA111_Serial(rxPin, txPin);// 创建软件串口对象(RX, TX)
SoftwareSerial portOne(2, 3);

void setup() {
  //pinMode(14, LOW);
  // 硬件串口初始化
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  Serial.begin(9600);
  // 初始化串口
  //Serial.println("ceshi1.0");
  delay(1000);
  BA111_Uart_init();
  //Serial.println("ceshi1.2");
  delay(50);
  BA111_calibration();
  //Serial.println("ceshi1.3");

}

void loop() {
   if (portOne.isListening()) { 
        Serial.println("portOne is listening!");
    }
  BA111_Getsensordata();
  delay(1000);
  BA111_ParseData();
  delay(1000);

}


//BA111初始化
void BA111_Uart_init()
{
  BA111_Serial.begin(9600);
  delay(10);
}

//BA111校准函数，此函数可实现TDS传感器校准，务必按照手册参数调整
void BA111_calibration()
{
  Serial.println("开始校准");
  BA111_Calibration(); 
  BA111_Check_callback();
  //Serial.println("开始校准 4.1");
  BA111_Set_res(); 
  BA111_Check_callback();
  BA111_Set_B_NTC();
  BA111_Check_callback();
  Serial.println("校准结束"); 
}

//BA111检测TDS指令
void BA111_Getsensordata()
{

  String Detection_data = "A000000000A0"; 
  BA111_sendHexData(Detection_data);
  BA111_ParseData();

}

//BA111基线校准
void BA111_Calibration()
{
  //Serial.println("ceshi2.1");
  String Calibration_data = "A600000000A6";
  BA111_sendHexData(Calibration_data);
  //Serial.println("ceshi2.2 BA111_Calibration end");
}

//BA111设置 NTC 常温电阻值
//默认000186A0 10K电阻
void BA111_Set_res()
{
  String Set_res = "A3000186A0CA";
  BA111_sendHexData(Set_res);
}

//BA111设置 NTC B 值
//默认0F0A
void BA111_Set_B_NTC()
{
  String Set_B_NTC = "A50F0A0000BE";
  BA111_sendHexData(Set_B_NTC);
}

//BA111返回判断
void BA111_Check_callback()
{
  while(true)
  {
    //Serial.println("BA111_Check_callback ceshi 5.1");
    if(BA111_Check_DataReceived())
    {
      Serial.println("设置成功 ");
      break;
    }  
  }
}

//BA111发送16进制数据
void BA111_sendHexData(String hexString) {
    //Serial.println("ceshi3.1");
  int hexStringLength = hexString.length();
  //Serial.println("ceshi3.2");
  Serial.println(hexStringLength);
  if (hexStringLength % 2 != 0) {
    // 确保16进制字符串长度为偶数
    hexString = "0" + hexString;
    hexStringLength++;
    
  }
    for (int i = 0; i < hexStringLength; i += 2) {
      //Serial.println("ceshi3.3");
    // 从16进制字符串中提取一对字符
    String hexPair = hexString.substring(i, i + 2);
    // 将16进制字符串转换为字节
    byte hexValue = (byte)strtol(hexPair.c_str(), NULL, 16);
   // Serial.println(hexValue);

    // 发送字节
    BA111_Serial.write(hexValue);
  }
}

/*检测设置的返回数据是否正常
 * 设置成功返回AC 00 00 00 00 AC
 * 上述指令执行异常情况下返回
 * *AC XX 00 00 00 AE
 * 异常代码 XX：
 * * 01：命令帧异常
 * * 02：忙碌中
 * * 03：校正失败
 * * 04：检测温度超出范围
*/
// 接收返回结果

bool BA111_Check_DataReceived() {
  //Serial.println("BA111_Check_DataReceived ceshi 6.1");

  if(BA111_Serial.available() >= 6) {
    //Serial.println("BA111_Check_DataReceived ceshi 6.2");
    
    byte resp[6];
    for (int i=0; i<6; i++) {
      resp[i] = BA111_Serial.read();
      //Serial.println("BA111_Check_DataReceived ceshi 6.22");
    }
    
    if (resp[0] == 0xAC && 
        resp[1] == 0x00 &&
        resp[2] == 0x00 &&
        resp[3] == 0x00 &&  
        resp[4] == 0x00 &&
        resp[5] == 0xAC) {
          //Serial.println("BA111_Check_DataReceived ceshi 6.23");
       return true;    
    }else if(resp[0] == 0xAC)
    {// Serial.println("BA111_Check_DataReceived ceshi 6.3");
      switch(resp[1]) 
      {
        case 0x01:Serial.println("BA111命令帧异常");break;
        case 0x02:Serial.println("BA111设备忙");break;  
        case 0x03:Serial.println("BA111校准失败");break; 
        case 0x04:Serial.println("BA111检测温度超出范围");break;
        default: Serial.println("未知失败");
      }
        return false;         
    }
    // return false;
    
  }
  return false;

}

// 读取并解析数据
void BA111_ParseData() {

  if (BA111_Serial.available() > 0) {

    byte start = BA111_Serial.read();
    
    if (start == 0xAA) {  

      byte tdsHi = BA111_Serial.read();
      byte tdsLo = BA111_Serial.read();
      int tdsValue = (tdsHi<<8) + tdsLo;

      byte tempHi = BA111_Serial.read();
      byte tempLo = BA111_Serial.read();  
      int tempAdc = (tempHi<<8) + tempLo;
      float temp = tempAdc / 100.0;

      byte checksum = BA111_Serial.read();
      byte sum = start + tdsHi + tdsLo + tempHi + tempLo;
      if ((sum & 0xFF) == checksum) {
        
        Serial.print("TDS: ");
        Serial.print(tdsValue);
        Serial.print("ppm   Temp: ");
        Serial.print(temp);
        Serial.println("°C");
      }
    }
  }
}
