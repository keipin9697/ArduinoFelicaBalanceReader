// TwitterAccounts:@keipin9697
//コード参考ページ:ORBITSPACE様:http://www.orsx.net/blog/archives/3835
//接続・資料参考ページ:アトリエのどか様:http://www.atelier-nodoka.net/2012/05/kaomoji-sf-checker/
// ライブラリ提供:RCS620S Sony Felica Forum
#include <LiquidCrystal.h>
#include <Wire.h>
#include <RCS620S.h>

//システム&サービスコード定義
#define COMMAND_TIMEOUT           	400
#define POLLING_INTERVAL          	500
#define RCS620S_MAX_CARD_RESPONSE_LEN   30
#define CYBERNE_SYSTEM_CODE       	0x0003
#define COMMON_SYSTEM_CODE        	0xFE00
#define PASSNET_SERVICE_CODE      	0x090F
#define EDY_SERVICE_CODE          	0x170F
#define NANACO_SERVICE_CODE       	0x564F
#define WAON_SERVICE_CODE         	0x680B
#define HANICA_SYSTEM_CODE        	0x84A1
#define HANICA_SERVICE_CODE       	0x898F
 
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
RCS620S rcs620s;

//キャラクタロゴセットアップ
//交通系ICロゴ
byte train[8] = { 
	B01110,
	B00100, 
	B11111,
	B10101,
	B10101, 
	B11111,
	B11111,
	B01010 };

//楽天Edyロゴ
byte rakuten[8] = {
	B01110, 
	B11111, 
	B10001, 
	B10101, 
	B10011, 
	B10101, 
	B11111, 
	B01110 };

//WAONロゴ
byte waon[8] = { 
	B10101, 
	B10101, 
	B10101, 
	B01010, 
	B00000, 
	B01110, 
	B10001, 
	B01110 };
  
//nanacoロゴ
byte nanaco[8] = { 
	B01010, 
	B11111, 
	B11111, 
	B00011, 
	B00110, 
	B00110, 
	B01100, 
	B01100 };

//hanicaロゴ
byte hanica[8] = { 
	B01000, 
	B01110, 
	B01010, 
	B00000, 
	B11111, 
	B10001, 
	B11111, 
	B01010 };

void setup(){
  Serial.begin(115200);
  lcd.begin(16,2);
  lcd.createChar(1, train);
  lcd.createChar(2,rakuten);
  lcd.createChar(3,waon);
  lcd.createChar(4,nanaco);
  lcd.createChar(5,hanica);
 
  lcd.setCursor(0, 0);
  lcd.print("Felica CARD");
  lcd.setCursor(2, 1);
  lcd.print("CHECK SYSTEM");
  delay(7000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Build by");
  lcd.setCursor(0,1);
  lcd.print("@keipin9697");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WAON SF nanaco");
  lcd.setCursor(0,1);
  lcd.print("Edy hanica");
  delay(1000);
  lcd.clear();
  lcd.setCursor(5,0);
  lcd.print("Version");
  lcd.setCursor(3,1);
  lcd.print("2.2.1 Final");
  delay(2000);
}
 
void loop(){
  uint32_t balance;
  uint8_t buf[RCS620S_MAX_CARD_RESPONSE_LEN];
   
  rcs620s.timeout = COMMAND_TIMEOUT;
   
  if(rcs620s.polling(CYBERNE_SYSTEM_CODE)){
	// StoredFareRideSystem
	if(requestService(PASSNET_SERVICE_CODE)){
  	if(readEncryption(PASSNET_SERVICE_CODE, 0, buf)){
    	balance = buf[23];             	 
    	balance = (balance << 8) + buf[22];
    	printBalanceLCD(" \xba\xb3\xc2\xb3 IC", &balance);//コウツウIC
  	}
	}
  }
   
  // 共通領域
  else if(rcs620s.polling(COMMON_SYSTEM_CODE)){
	// Edy
	if(requestService(EDY_SERVICE_CODE)){
  	if(readEncryption(EDY_SERVICE_CODE, 0, buf)){
    	balance = buf[26];             	 
    	balance = (balance << 8) + buf[27];
    	printBalanceLCD(" \xd7\xb8\xc3\xdd Edy", &balance);//ラクテンEdy
  	}
	}
	 
	// nanaco
	if(requestService(NANACO_SERVICE_CODE)){
  	if(readEncryption(NANACO_SERVICE_CODE, 0, buf)){
    	balance = buf[17];             	 
    	balance = (balance << 8) + buf[18];
    	balance = (balance << 8) + buf[19];
    	balance = (balance << 8) + buf[20];
    	// 残高表示
    	printBalanceLCD(" nanaco", &balance);
  	}
	}
	 
	// waon
	if(requestService(WAON_SERVICE_CODE)){
  	if(readEncryption(WAON_SERVICE_CODE, 1, buf)){
    	balance = buf[17];             	 
    	balance = (balance << 8) + buf[20];
    	balance = (balance << 8) + buf[21];
    	balance = balance & 0x7FFFE0;  	 
    	balance = balance >> 5;       	 
    	// 残高表示
    	printBalanceLCD(" WAON", &balance);
  	}
	}
  }
 
  else if(rcs620s.polling(HANICA_SYSTEM_CODE)){
  	// hanica
	if(requestService(HANICA_SERVICE_CODE)){
  	if(readEncryption(HANICA_SERVICE_CODE, 0, buf)){
    	// Big Endianで入っているhanicaの残高を取り出す
    	balance = buf[26];              	// 15 byte目
    	balance = (balance << 8) + buf[27]; // 16 byte目
    	// 残高表示
    	printBalanceLCD(" hanica", &balance);
  	}
	}
   
    
  }
   
  else{
	lcd.clear();
	lcd.print("  TOUCH HERE!");
	lcd.setCursor(0,1);
	lcd.print("  YOUR IC CARD");
  }
   
  rcs620s.rfOff();
  delay(POLLING_INTERVAL);
}
 
// request service
int requestService(uint16_t serviceCode){
  int ret;
  uint8_t buf[RCS620S_MAX_CARD_RESPONSE_LEN];
  uint8_t responseLen = 0;
   
  buf[0] = 0x02;
  memcpy(buf + 1, rcs620s.idm, 8);
  buf[9] = 0x01;
  buf[10] = (uint8_t)((serviceCode >> 0) & 0xff);
  buf[11] = (uint8_t)((serviceCode >> 8) & 0xff);
 
  ret = rcs620s.cardCommand(buf, 12, buf, &responseLen);
   
  if(!ret || (responseLen != 12) || (buf[0] != 0x03) ||
  	(memcmp(buf + 1, rcs620s.idm, 8) != 0) || ((buf[10] == 0xff) && (buf[11] == 0xff))) {
	return 0;
  }
 
  return 1;
}
 
int readEncryption(uint16_t serviceCode, uint8_t blockNumber, uint8_t *buf){
  int ret;
  uint8_t responseLen = 0;
   
  buf[0] = 0x06;
  memcpy(buf + 1, rcs620s.idm, 8);
  buf[9] = 0x01; //
  buf[10] = (uint8_t)((serviceCode >> 0) & 0xff);
  buf[11] = (uint8_t)((serviceCode >> 8) & 0xff);
  buf[12] = 0x01;
  buf[13] = 0x80;
  buf[14] = blockNumber;
 
  ret = rcs620s.cardCommand(buf, 15, buf, &responseLen);
 
  if (!ret || (responseLen != 28) || (buf[0] != 0x07) ||
  	(memcmp(buf + 1, rcs620s.idm, 8) != 0)) {
	return 0;
  }
 
  return 1;
}
 
void printBalanceLCD(char *card_name, uint32_t *balance){
  char result[8];
  sprintf(result, "%u", *balance);
  lcd.clear();
  lcd.setCursor(0,0);
  
  //ICカードロゴ表示if文
  if(card_name==" \xba\xb3\xc2\xb3 IC"){
	lcd.write(1);
  }
  if(card_name==" \xd7\xb8\xc3\xdd Edy"){
	lcd.write(2);
  }
  if(card_name==" WAON"){
	lcd.write(3);
  }
  if(card_name==" nanaco"){
	lcd.write(4);
  }
  if(card_name==" hanica"){
	lcd.write(5);
  }
 
  lcd.print(card_name); //カード名表示
  lcd.print("\xbb\xde\xdd\xc0\xde\xb6"); //ザンダカ
  lcd.setCursor(7,1);
  lcd.print(result);//残高balance
  lcd.print("\xfc");//円マーク
  return;
}
