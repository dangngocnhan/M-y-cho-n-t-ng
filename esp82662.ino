#include <FirebaseESP8266.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <SoftwareSerial.h>
#define RX_PIN D1 // Chân RX của cổng nối tiếp mềm
#define TX_PIN D2 // Chân TX của cổng nối tiếp mềm
SoftwareSerial mySerial(RX_PIN, TX_PIN); // Tạo một đối tượng cổng nối tiếp mềm

//#define WIFI_SSID "Try"
//#define WIFI_PASSWORD "nnnnnnnnn"

#define WIFI_SSID "VIETTEL-pass_len_hoi_a_hoan_nha"
#define WIFI_PASSWORD "Khongchodau"


#define FIREBASE_HOST "project5-choantudong-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "gy5OCNaewcaAbHhKW6EH5jqqgRlFSI3aYCloqED5"

FirebaseData firebaseData;
// WifiClient client;
String path = "/";
FirebaseJson json;


byte check_valiable_manual;
byte check_valiable_manual_tmp = check_valiable_manual;
byte module = 0;        byte check_module = 0;
byte module_manual = 0; byte check_module_manual = 0;
byte module_auto = 0;   byte check_module_auto = 0;
byte module_auto_x = 0; byte check_module_auto_x = 0;       
bool stopfor = 0;       bool check_stopfor = 0;  

unsigned int foodAmout_manual;   /* lượng thức ăn cho mỗi lần ăn manual*/        
int check_footAmout_manual = foodAmout_manual;
unsigned int remaining_food; /* lượng thức ăn còn lại trong bồn */   
unsigned int check_remaining_food = remaining_food;

struct lich_trinh{
   byte gio;
   byte phut;
   unsigned int food; // 0-65535
   bool state;
};
typedef lich_trinh Lich_Trinh;
Lich_Trinh DS_LichTrinh[9];
//={
//    {13,58, 100,0},
//    {13,59, 222,0}, 
//    {8,30, 4022,0}, 
//    {9,30, 5022,0},  
//};
    
int aSoPT = 0; // số phần tử thực của DS
int soPT_tmp = aSoPT; // số phần tử thay đổi trên firebase để kiểm tra
bool check_update_LT = false;  // kiểm tra xem có sự kiện updata không
bool check_update_LT_tmp = check_update_LT;  // kiểm tra update
String dsID_str = ""; 
int *arrID; 

int checkSendData;
int checkGetData;

String str_json = "";
// Tạo một đối tượng JsonDocument có dung lượng 200 byte
StaticJsonDocument<200> doc_send;
StaticJsonDocument<200> doc_get;

void setup() {
  checkSendData = 0;
  checkGetData = 0;
  mySerial.begin(9600);
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  if (!Firebase.beginStream(firebaseData, path)) {
    Serial.println("REASON: " + firebaseData.errorReason());
    Serial.println();
  } 
  
  Serial.print("\nConnected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

/////////////// FIRE BASE
  Firebase.setInt(firebaseData, "//remaining_food", remaining_food); 
          
  getDataToFirebase_(); 
  delay(100);
  send_dsLichTrinh_toArduino();
}

void loop() {
    
    // đọc dữ liệu
    read_data_manual_(); 
    hienthiSerial_LT_(); 
    add_Del_LichTrinh_Firebase_();
    update_LicTrinh_();
    delay(500);
}

String readFromUART() {
  String data = "";
  while (mySerial.available()) {
    char c = mySerial.read();
    if (c == '\n') {
      break;
    }
    data += c;
  }
  return data;
}

void send_dsLichTrinh_toArduino(){ 
    byte batdau = 0;
    while(batdau != 9){ 
        String jsoncheck = readFromUART();   Serial.println("jsoncheck: " + jsoncheck);
        deserializeJson(doc_get, jsoncheck);
        batdau = doc_get["batdau"];
        jsoncheck = "";
        doc_get.clear();
        Serial.println("batdau: " + String(batdau) + "....");
        delay(500);
    }
    byte check_ok = 0;
    for(byte i = 0; i < aSoPT; i++){ 
        check_ok = 0; 
        ///
        while(check_ok != 9){  
            str_json = "{\"aSoPT\":"+String(aSoPT)+"";  
            str_json = str_json + ",\"index\":"+String(i)+"";   
            String strtmp = ",\"L_" + String(i) + "\":["
                                        + String(DS_LichTrinh[i].gio) + "," 
                                        + String(DS_LichTrinh[i].phut)+ ","
                                        + String(DS_LichTrinh[i].food)+ ","
                                        + String(DS_LichTrinh[i].state)+"]";
            str_json = str_json + strtmp;  
            str_json = str_json + ",\"zo\":"+String(1)+"}"; 
            
            deserializeJson(doc_send, str_json);
            serializeJson(doc_send, mySerial);
            mySerial.println();
    
            serializeJson(doc_send, Serial);
            Serial.println();
    
            str_json = "";
            doc_send.clear(); 
            
        ///////////////////////////////////////////
        
            String jsoncheck = readFromUART();
            deserializeJson(doc_get, jsoncheck);
            check_ok = doc_get["check_ok"];
            jsoncheck = "";
            doc_get.clear(); 
            Serial.println("check_ok: " + String(check_ok) + "...."); 
            
            delay(500);
        }
    } 
    Serial.println("Send Lich Trình OKkkk Nhé!");       
}

Lich_Trinh Data_LichTrinh_New(String LT_new){ // tách lấy dữ liệu lịch trình mới
    Lich_Trinh LichTrinh_new;  
    byte LT_number = 0;
    String str = ""; 
    for(int i = 0; i <= LT_new.length(); i++){ 
        if(LT_new.charAt(i) == ',' || i == LT_new.length()){
            if(LT_number == 0)
                LichTrinh_new.gio = str.toInt();
            if(LT_number == 1)
                LichTrinh_new.phut = str.toInt();
            if(LT_number == 2)
                LichTrinh_new.food = str.toInt();
            if(LT_number == 3)
                LichTrinh_new.state = str.toInt();
            
            LT_number++;
            str = "";
        }else {
            str = str + LT_new.charAt(i); 
        }
    }
    return  LichTrinh_new;
}

int * tach_dsID(String dsID_str){
    static int arr_ID[10];  byte arrID_number = 0; // mảng danh sách ID
    String strID = ""; // id tạm
    for(int i = 0; i <= dsID_str.length(); i++){ 
        if(dsID_str.charAt(i) == ',' || i == dsID_str.length()){
            arr_ID[arrID_number] = strID.toInt();
            arrID_number++;
            strID = "";
        }else {
            strID = strID + dsID_str.charAt(i); 
        }
    }   
    return arr_ID;
}

void Data_Manual_New_(){ // tách lấy dữ liệu lịch trình mới  
    String data_manual_str = "";
    if(Firebase.getString(firebaseData, "//valiableData/data_manual"))
        data_manual_str = firebaseData.stringData();
    byte index = 0;
    String str = ""; 
    for(int i = 0; i <= data_manual_str.length(); i++){
        if(data_manual_str.charAt(i) == ',' || i == data_manual_str.length()){
            if(index == 0)
                module = str.toInt();
            if(index == 1)
                module_auto = str.toInt();
            if(index == 2)
                module_auto_x = str.toInt();
            if(index == 3)
                module_manual = str.toInt();
            if(index == 4)
                foodAmout_manual = str.toInt();
            
            index++;
            str = "";
        }else {
            str = str + data_manual_str.charAt(i); 
        }
    }
}

void read_data_manual_(){
    if(Firebase.getInt(firebaseData, "//check_valiable_manual"))
        check_valiable_manual_tmp = firebaseData.intData();
    if (check_valiable_manual != check_valiable_manual_tmp) { 
        Data_Manual_New_(); // đọc data từ firebase và cập nhập vào các biến
        check_valiable_manual = check_valiable_manual_tmp;
    }
}

void getDataToFirebase_(){  
//    if(checkGetData == 0){ 
        if(Firebase.getInt(firebaseData, "//check_valiable_manual"))
            check_valiable_manual = firebaseData.intData(); 
        Data_Manual_New_(); // dữ liệu manual

        if(Firebase.getInt(firebaseData, "//aSoPT"))  
            aSoPT = firebaseData.intData();  
        if(Firebase.getString(firebaseData, "//dsID"))  
            dsID_str = firebaseData.stringData();
        if(Firebase.getInt(firebaseData, "//check_update_LT"))  
            check_update_LT = firebaseData.intData();
        
             
        arrID = tach_dsID(dsID_str);  // con trỏ dsID 
        
        for (byte i = 0; i < aSoPT; i++) {
            String path_LT = "//DS_LichTrinh/DS_LichTrinh_" + String(*(arrID+i)) + "/LichTrinh";
            String LT_new_str = "";
            if(Firebase.getString(firebaseData, path_LT))   
                LT_new_str = firebaseData.stringData(); 
            DS_LichTrinh[i] = Data_LichTrinh_New(LT_new_str);
        }

        soPT_tmp = aSoPT;
        check_update_LT_tmp = check_update_LT; 
        check_valiable_manual_tmp = check_valiable_manual;
//    }  
}

void hienthiSerial_LT_(){
    Serial.println("LT : ");
    for (byte i = 0; i < aSoPT; i++) {
        Serial.println(String(DS_LichTrinh[i].gio) + ":" 
            + String(DS_LichTrinh[i].phut) + "---" 
            + String(DS_LichTrinh[i].food) + "---" 
            + String(DS_LichTrinh[i].state));
    } 
    Serial.println("soPT: " + String(aSoPT));
    Serial.println("dsID: " + dsID_str); 
 
    Serial.print("arrID : ");
    for(int i = 0; i < aSoPT; i++){
        Serial.print(" " + String(*(arrID+i)) + "--");
    } Serial.println("\n"); ///////////// 

    Serial.println("module: " + String(module));
    Serial.println("module_auto: " + String(module_auto));
    Serial.println("module_auto_x: " + String(module_auto_x));
    Serial.println("module_manual: " + String(module_manual));
    Serial.println("foodAmout_manual: " + String(foodAmout_manual));
    Serial.println("");
}

void sapxep_DS_LichTrinh_(){
    for(int i = 0; i < aSoPT - 1; i++){
        for(int j = i + 1; j < aSoPT; j++){
            if(DS_LichTrinh[i].gio > DS_LichTrinh[j].gio 
            || (DS_LichTrinh[i].gio == DS_LichTrinh[j].gio && DS_LichTrinh[i].phut > DS_LichTrinh[j].phut)){ 
                Lich_Trinh tmp = DS_LichTrinh[i];
                DS_LichTrinh[i] = DS_LichTrinh[j];
                DS_LichTrinh[j] = tmp;        
            } 
        }
    }
}

 // khi esp doc duoc lenh add lich trinh tu fire base 
void Change_DSLichTrinh_toArrduino(byte action){  
    byte batdau = 0;
    while(batdau != 9){  
        //gửi data có lịch trình co arduino biết
        str_json = "{\"actionLT\":"+String(action)+"}";  
        mySerial.println(str_json);
        str_json = "";

        //chờ pản hồi các nhận của arduino
        String jsoncheck1 = readFromUART();  
            Serial.println("jsoncheck bat dau: " + jsoncheck1);
        deserializeJson(doc_get, jsoncheck1);
        batdau = doc_get["batdau"];
        jsoncheck1 = ""; doc_get.clear();
        Serial.println("batdau: " + String(batdau) + "....");
        delay(500);
    }
    send_dsLichTrinh_toArduino();
}

void add_Del_LichTrinh_Firebase_(){  
    if(Firebase.getInt(firebaseData, "//aSoPT")){
        soPT_tmp = firebaseData.intData();
    }
    if (aSoPT == soPT_tmp) {  }
    else { 
        if(Firebase.getString(firebaseData, "//dsID"))  // cập nhật lại dsID
            dsID_str = firebaseData.stringData(); 
        arrID = tach_dsID(dsID_str);  // con trỏ dsID 
        
        String LT_new_str = "";   // đọc lịch trình cần thêm/ xóa
        if(Firebase.getString(firebaseData, "//action"))
                LT_new_str = firebaseData.stringData(); 
        Lich_Trinh LichTrinh_new = Data_LichTrinh_New(LT_new_str);
        
        if(aSoPT < soPT_tmp){ // thêm phần tử 
             
            DS_LichTrinh[aSoPT] = LichTrinh_new;  
            aSoPT = soPT_tmp;
            sapxep_DS_LichTrinh_();
            Change_DSLichTrinh_toArrduino(1); // gui LT moi den arduino
        }
        else if (aSoPT > soPT_tmp){  // day la xoa phan tu   
            
            for(byte i = 0; i < aSoPT; i++){
                if(DS_LichTrinh[i].gio == LichTrinh_new.gio && 
                                               DS_LichTrinh[i].phut == LichTrinh_new.phut){
                    for (byte j = i; j < aSoPT; j++){
                        DS_LichTrinh[j] = DS_LichTrinh[j+1];
                    }
                    break;
                } 
            } 
            aSoPT = soPT_tmp;
            sapxep_DS_LichTrinh_();
            Change_DSLichTrinh_toArrduino(3);
        }
    }
}

void update_LicTrinh_(){
    // Update Lich trinh
    if(Firebase.getInt(firebaseData, "//check_update_LT"))
        check_update_LT_tmp = firebaseData.intData();  
    if(check_update_LT_tmp != check_update_LT){
        String LT_new_str = "";  
        if(Firebase.getString(firebaseData, "//action"))
                LT_new_str = firebaseData.stringData(); 
                
        Lich_Trinh LichTrinh_new = Data_LichTrinh_New(LT_new_str);
        
        for(int i = 0; i < aSoPT; i++){
            if(DS_LichTrinh[i].gio == LichTrinh_new.gio && 
                                              DS_LichTrinh[i].phut == LichTrinh_new.phut){
                DS_LichTrinh[i].food = LichTrinh_new.food;
                DS_LichTrinh[i].state = LichTrinh_new.state;
                break;
            } 
        }
        check_update_LT = check_update_LT_tmp; 
        Change_DSLichTrinh_toArrduino(2);
    } 
}


void sendToFirebase_() {
  if(checkSendData == 0){
  Firebase.setInt(firebaseData, "//aSoPT", aSoPT);
  for (int i = 0; i < aSoPT; i++) {
//        String path_gio = "//DS_LichTrinhTest/DS_LichTrinhTest_" + String(DS_LichTrinh[i].gio) + ":" + String(DS_LichTrinh[i].phut) + "/gio";
//        String path_phut = "//DS_LichTrinhTest/DS_LichTrinhTest_" + String(DS_LichTrinh[i].gio) + ":" + String(DS_LichTrinh[i].phut) + "/phut";
//        String path_food = "//DS_LichTrinhTest/DS_LichTrinhTest_" + String(DS_LichTrinh[i].gio) + ":" + String(DS_LichTrinh[i].phut) + "/food";
//        String path_state = "//DS_LichTrinhTest/DS_LichTrinhTest_" + String(DS_LichTrinh[i].gio) + ":" + String(DS_LichTrinh[i].phut) + "/state";
        String path_gio = "//DS_LichTrinh/DS_LichTrinh_" + String(i) + "/gio";
        String path_phut = "//DS_LichTrinh/DS_LichTrinh_" + String(i) + "/phut";
        String path_food = "//DS_LichTrinh/DS_LichTrinh_" + String(i) + "/food";
        String path_state = "//DS_LichTrinh/DS_LichTrinh_" + String(i) + "/state";
        Firebase.setInt(firebaseData, path_gio, DS_LichTrinh[i].gio);
        Firebase.setInt(firebaseData, path_phut, DS_LichTrinh[i].phut);
        Firebase.setInt(firebaseData, path_food, DS_LichTrinh[i].food);
        Firebase.setInt(firebaseData, path_state, DS_LichTrinh[i].state);
  }
  checkSendData = 1;
  }
}
