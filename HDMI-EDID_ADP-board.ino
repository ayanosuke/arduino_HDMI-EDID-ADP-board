#include <Wire.h>
#include <scpiparser.h>
#include <Arduino.h>
#include <avr/pgmspace.h>

#define EEPROM_I2C_ADDRESS 0x50

struct scpi_parser_context ctx;

scpi_error_t identify(struct scpi_parser_context* context, struct scpi_token* command);
scpi_error_t get_eeprom(struct scpi_parser_context* context, struct scpi_token* command);
scpi_error_t get_chksum(struct scpi_parser_context* context, struct scpi_token* command);
//scpi_error_t set_voltage(struct scpi_parser_context* context, struct scpi_token* command);

//uncomment accordingly:
//int maxaddress=128-1;             //24C01    -> 1024 bit    -> 128 byte
int maxaddress = 256 - 1;         //24C02    -> 2048 bit    -> 256 byte
//int maxaddress=512-1;             //24C04    -> 4096 bit    -> 512 byte
//int maxaddress=1024-1;            //24C08    -> 8192 bit    -> 1024 byte
//int maxaddress=2048-1;            //24C16    -> 16384 bit   -> 2048 byte
//int maxaddress=4096-1;            //24C32    -> 32768 bit   -> 4096 byte
//int maxaddress=8192-1;            //24C64    -> 65536 bit   -> 8192 byte
//int maxaddress=16384-1;           //24C128   -> 131072 bit  -> 16384 byte
//int maxaddress=32768-1;           //24C256   -> 262144 bit  -> 32768 byte
//unsigned int maxaddress=65536-1;  //24C512   -> 524288 bit  -> 65536 byte

int address = 0;
unsigned int value = 0;
unsigned int check_value = 0;

unsigned char EDID[256];
char VER[] = "HDMI EDID 24C02 Writer/Reader Ver0.3";




void setup(void)
{
  struct scpi_command* source;
  struct scpi_command* measure;

  /* First, initialise the parser. */
  scpi_init(&ctx);

  scpi_register_command(ctx.command_tree, SCPI_CL_SAMELEVEL, "*IDN?", 5, "*IDN?", 5, identify);
  scpi_register_command(ctx.command_tree, SCPI_CL_SAMELEVEL, "EPR?", 4, "EPR?", 4, get_eeprom);
  scpi_register_command(ctx.command_tree, SCPI_CL_SAMELEVEL, "DUMP?", 5, "DUMP?", 4, dump_eeprom);
  scpi_register_command(ctx.command_tree, SCPI_CL_SAMELEVEL, "EPW", 3, "EPW", 3, write_eeprom);
  scpi_register_command(ctx.command_tree, SCPI_CL_SAMELEVEL, "CHK?", 4, "CHK?", 4, get_chksum);
  scpi_register_command(ctx.command_tree, SCPI_CL_SAMELEVEL, "MEM", 3, "MEM", 3, set_memory);
  //  scpi_register_command(ctx.command_tree, SCPI_CL_SAMELEVEL, "?", 1, "?", 1, help); // 有効にすると動作全体がおかしい

  memset(&EDID, 0x00, sizeof(EDID));

  Serial.begin(9600);

  Wire.begin();
}

void loop()
{
  char line_buffer[256];
  unsigned char read_length;

  Serial.println(VER);
  Serial.println();
  
  while (1)
  {
    /* Read in a line and execute it. */
    read_length = Serial.readBytesUntil('\n', line_buffer, 256);
    if (read_length > 0)
    {
      //Serial.println(line_buffer);
      scpi_execute_command(&ctx, StrToUpper(line_buffer), read_length);
    }
  }
}

//--------------------------------------------------------------------------------------
// 関数:char *StrToUpper(char *)
// 機能:文字列中の英小文字を大文字に変換
// http://www1.cts.ne.jp/~clab/hsample/Point/Point11.html
//--------------------------------------------------------------------------------------
char *StrToUpper(char *s)
{
  char *p;                     /* 作業用ポインタ */
  /* for (  )ループの初期化の式で、pは文字列の
     先頭アドレスを指すように初期化される */
  for (p = s; *p; p++)         /* pがヌルでなければ */
    *p = toupper(*p);        /* pの指す中身を大文字に変換 */
  return (s);                  /* 文字列の先頭アドレスを返す */
}

//--------------------------------------------------------------------------------------
// 関数:identify
// 機能:バージョンを表示します
//--------------------------------------------------------------------------------------
scpi_error_t identify(struct scpi_parser_context* context, struct scpi_token* command)
{
  scpi_free_tokens(command);

  //Serial.println(VER);
  return SCPI_SUCCESS;
}

//--------------------------------------------------------------------------------------
// 関数:get_eeprom
// 機能:EPR?コマンドの処理。EEP-ROMからデータを読み込みます。
//--------------------------------------------------------------------------------------
scpi_error_t get_eeprom(struct scpi_parser_context* context, struct scpi_token* command)
{
  char foo[10];
  char txt[20] = "          ";
  int i = 0;

  Serial.println("");
  Serial.println("CMD:EPR?");
  Serial.println("ADDRESS:+0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F  0123456789ABCDEF");

  for (address = 0 ; address <= maxaddress ; address++)
  {

    if (address % 16 == 0)
    {
      if (address != 0) { // address:0000の時に空行を出力させない為に
        Serial.print(" "); Serial.println(txt);
      }
      sprintf(foo, "   %04X:", address);
      Serial.print(foo);
      i = 0;
    }
    check_value = readEEPROM(EEPROM_I2C_ADDRESS, address);
    EDID[address] = check_value;
    sprintf(foo, "%02X ", check_value);
    Serial.print(foo);

    if ( check_value >= 32 && check_value <= 126)
    {
      sprintf(foo, "%c", check_value);

    }
    else
      foo[0] = '.';

    txt[i] = foo[0];
    i++;

  }

  Serial.print(" "); Serial.println(txt); //最後のキャラクタ表示

  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

//--------------------------------------------------------------------------------------
// 関数:dump_eeprom
// 機能:DUMPコマンドの処理。メモリデータを表示します。
//--------------------------------------------------------------------------------------
scpi_error_t dump_eeprom(struct scpi_parser_context* context, struct scpi_token* command)
{
  char foo[10];
  char txt[20] = "          ";
  int i = 0;

  Serial.println("");
  Serial.println("CMD:DUMP");
  Serial.println("ADDRESS:+0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F  0123456789ABCDEF");

  for (address = 0 ; address <= maxaddress ; address++)
  {

    if (address % 16 == 0)
    {
      if (address != 0) { // address:0000の時に空行を出力させない為に
        Serial.print(" "); Serial.println(txt);
      }
      sprintf(foo, "   %04X:", address);
      Serial.print(foo);
      i = 0;
    }
    check_value = EDID[address];
    sprintf(foo, "%02X ", check_value);
    Serial.print(foo);

    if ( check_value >= 32 && check_value <= 126)
    {
      sprintf(foo, "%c", check_value);

    }
    else
      foo[0] = '.';

    txt[i] = foo[0];
    i++;

  }

  Serial.print(" "); Serial.println(txt); //最後のキャラクタ表示

  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

//--------------------------------------------------------------------------------------
// 関数:write_eeprom
// 機能:EPWコマンドの処理。メモリデータをEEP-ROMに書き込みます。
//--------------------------------------------------------------------------------------
scpi_error_t write_eeprom(struct scpi_parser_context* context, struct scpi_token* command)
{
  unsigned char foo;
  Serial.println("");
  Serial.println("CMD:EPW");

  for (address = 0 ; address <= maxaddress ; address++)
  {
    writeEEPROM(EEPROM_I2C_ADDRESS, address, EDID[address]);
    foo = readEEPROM(EEPROM_I2C_ADDRESS, address);

    if (address != 0) {
      if (address % 16 == 0)      //16キャラクタで改行
        Serial.println("");
    }

    if (foo == EDID[address] )
      Serial.print("o");
    else
      Serial.print("x");
  }

  Serial.println("");
  Serial.println("verify ok");

  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

//--------------------------------------------------------------------------------------
// 関数:get_chksum
// 機能:EDIDのチェックサムを演算して表示します。
// 全てのデータを加算して1byte取り出して、256-加算値=chksum
// http://www.odtechno.co.jp/odm/20090814/
//--------------------------------------------------------------------------------------
scpi_error_t get_chksum(struct scpi_parser_context* context, struct scpi_token* command)
{
  unsigned int foo = 0;
  unsigned char var = 0;
  char txt[20] = "          ";

  Serial.println("");
  Serial.println("CMD:CHK?");

  for (address = 0 ; address <= 128 - 1 - 1 ; address++) { // 0x00〜0x7Eまでを加算
    foo = foo + EDID[address];
  }

  var = (unsigned char)foo; // int から char に型変換して下位1バイトを取り出す

  var = 256 - var;

  sprintf(txt, "chksum 007F:%02X", var);
  Serial.println(txt);
  
  EDID[0x007F]=var;

  foo = 0;
  for (address = 128 ; address <= 256 - 1 - 1 ; address++) { // 0x80〜0xFEまでを加算
    foo = foo + EDID[address];
  }

  var = (unsigned char)foo; // int から char に型変換して下位1バイトを取り出す

  var = 256 - var;

  sprintf(txt, "chksum 00FF:%02X", var);
  Serial.println(txt);
  
  EDID[0x00FF]=var;

 
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

//--------------------------------------------------------------------------------------
// 関数:set_memory
// 機能:指定したアドレスからバッファを書き換えます。
// 使用例：MEM 00:00,01,02,03,04,05,06
//        アドレス00から00,01,02,03,04,05,06に変更します。
//--------------------------------------------------------------------------------------
scpi_error_t set_memory(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args;
  struct scpi_numeric setmem_a;
  int i;
  int fcnt;
  int address;
  unsigned char data;

  String txt;
  String stxt;

  const char *tmp;

  Serial.println("");
  Serial.println("CMD:MEM");

  args = command;

  while (args != NULL && args->type == 0)
  {
    args = args->next;
  }

  txt = args->value;
  //Serial.print(txt);
  stxt = txt.substring(0, 4);         // aaaa: アドレス抽出
  tmp = stxt.c_str();                 // よくわからないからStringからchar型に変換
  address =  strtoul(tmp, NULL, 16);  // 16進数を10進数に変換
  Serial.print(stxt); Serial.print(":");

  fcnt = args->length;
//Serial.println((fcnt - 4) / 3);

  for (i = 0 ; i <= (fcnt - 4) / 3 - 1 ; i++) { // 文字長から無理くり書き込むバイト数を算出
    stxt = txt.substring(5 + i * 3, 5 + i * 3 + 2); // 5,8,11,14...47,50
    tmp = stxt.c_str();
    EDID[address + i] = (unsigned char)strtoul(tmp, NULL, 16);
    Serial.print(stxt);
    Serial.print(" ");
  }
  Serial.println("");

  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/*
//--------------------------------------------------------------------------------------
// 関数:help
// 機能:コマンド一覧を表示します。
// http://www.musashinodenpa.com/arduino/ref/index.php?f=0&pos=1824
//--------------------------------------------------------------------------------------
scpi_error_t help(struct scpi_parser_context* context, struct scpi_token* command)
{

  Serial.println("");
  Serial.println(VER);
  Serial.println("");

  Serial.println("*IDN? :基板名とバージョンを表示します。");
  Serial.println("EPR?  :EEP-ROMからデータを読み込みます。");
  Serial.println("DUMP? :EEP-ROMから読み込んだデータを表示します。");
  Serial.println("MEM   :MEM aaaa:x1 x2 x3 ... x15 x16  aaaaアドレス x1 データをセットします。");
  Serial.println("CHK?  :チェックサムを生成します。");
  Serial.println("EPW   :読み込んでいるデータを書き込みます。");
  Serial.println("?:コマンド一覧を表示。");
  Serial.println("(Arduino IDEのシリアルモニタでは一部文字化けします)");
  Serial.println("");

  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}
*/



void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data )
{
  if (maxaddress <= 255)
  {
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress));
    Wire.write(data);
    Wire.endTransmission();
  }

  if (maxaddress >= 511)
  {
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8));   // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
  }

  delay(5);
}

byte readEEPROM(int deviceaddress, unsigned int eeaddress )
{
  byte rdata = 0xFF;

  if (maxaddress <= 255)
  {
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress));
    Wire.endTransmission();
  }

  if (maxaddress >= 511)
  {
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8));   // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
  }

  Wire.requestFrom(deviceaddress, 1);

  if (Wire.available()) rdata = Wire.read();

  return rdata;
}
