// https://github.com/SWITCHSCIENCE/samplecodes/blob/master/9826_MA735_Digital_Angle_Sensor_Module/ma735_setting/ma735_setting.ino

#include <Arduino.h>
#include <SPI.h>

//const int MA735_CS = SS;
const int MA735_CS = 1;
SPISettings settings = SPISettings(10000000, MSBFIRST, SPI_MODE0);

uint32_t angle_interval = 0;
uint32_t angle_last = 0;

const int G_IN = 0;                // カムパルスセンサ(本体のピン表記は6)
uint32_t crank_interval = 0;
uint32_t crank_last = 0;
float Ne_deg = 0.0;                // 磁気エンコーダのクランク角(deg)
int8_t Ne_rev = 0;                 // クランク回転数(クランク角計算用)
bool G_Pulse = false;              // カムパルスセンサの立ち上がりフラグ
bool G_Pulse_Flag = false;         // カムパルスセンサの立ち上がりフラグ

// MA735磁気エンコーダSPIで角度を読み取る
float readMA735SPI() {
  static uint16_t _rd = 0;                  // 16bit(0-65535)の前回の角度

  SPI.beginTransaction(settings);
  digitalWrite(MA735_CS, LOW);
  uint16_t rd = SPI.transfer16(0);          // 16bit(0-65535)で現在の角度を取得
  digitalWrite(MA735_CS, HIGH);
  SPI.endTransaction();

  long diff_rd = rd - _rd;                  // クランク角の差分を計算
  if (diff_rd < -32767) {                   // クランク角の差分が-180°以上の場合(=正転でクランク角が上死点を超えた場合)
    Ne_rev++;                                 // 回転数をカウントアップ
    if ( G_Pulse_Flag) {                      // カムパルスセンサ立ち上がりフラグONの場合
      Ne_rev = 0;                               // クランク回転数をリセット
      G_Pulse_Flag = false;                     // カムパルスセンサの立ち上がりフラグをOFF
      Serial.println("G_Pulse_Flag: false");
    }
  }
  else if (diff_rd > 32767) {               // クランク角の差分が180°以上の場合(=逆転でクランク角が上死点を超えた場合)
    Ne_rev--;                                 // 回転数をカウントダウン
  }
  _rd = rd;                                 // 現在の角度を前回の角度に設定
  return (float)rd / 65535 * 360 + 360 * Ne_rev;  // 0-720(deg)に変換
}


// ヘルプ表示
void printHelp() {
  Serial.println("MA735 Register Settings available commands:");
  Serial.println("d");
  Serial.println("  print all registers");
  Serial.println("m interval");
  Serial.println("  Outputs the angle at interval.");
  Serial.println("r reg");
  Serial.println("  Reads the register at address 'reg'.");
  Serial.println("ne interval");
  Serial.println("  Outputs the crank angle at interval.");
  Serial.println("w reg val");
  Serial.println("  Writes 'val' to the register at address 'reg'.");
  Serial.println("z");
  Serial.println("  Set the current angle to the zero position.");
  Serial.println("cw");
  Serial.println("  Set the rotation direction to clockwise.");
  Serial.println("ccw");
  Serial.println("  Set the rotation direction to counterclockwise.");
  Serial.println("ppt pulsecount");
  Serial.println("  Set the number of pulses per rotation.");
}

// 8ビット2進数と16進数を表示
void printBinary8(uint8_t num) {
  Serial.print(" 0b");
  for (int i = 7; i >= 0; i--) {
    Serial.print((num >> i) & 1);
  }
  Serial.print(" 0x");
  Serial.println(num, HEX);
}

// 引数を解析して数値に変換
int parseArgument(String arg) {
  if (arg.startsWith("0x")) {
    return (int)strtol(arg.c_str(), NULL, 16);  // 16進数として解析
  }
  else if (arg.startsWith("0b")) {
    return (int)strtol(arg.substring(2).c_str(), NULL, 2);   // 2進数として解析
  }
  else {
    return arg.toInt();  // 10進数として解析
  }
}

// レジスタの読み取り
uint8_t readReg(uint8_t reg) {
  SPI.beginTransaction(settings);
  digitalWrite(MA735_CS, LOW);
  SPI.transfer16(0x4000 | (reg & 0x1f) << 8);
  digitalWrite(MA735_CS, HIGH);

  delayMicroseconds(100);

  digitalWrite(MA735_CS, LOW);
  uint16_t rd = SPI.transfer16(0);
  digitalWrite(MA735_CS, HIGH);
  SPI.endTransaction();
  return rd >> 8;
}

// レジスタの書き込み
uint8_t writeReg(uint8_t reg, uint8_t val) {
  SPI.beginTransaction(settings);
  digitalWrite(MA735_CS, LOW);
  SPI.transfer16(0x8000 | (reg & 0x1f) << 8 | val);
  digitalWrite(MA735_CS, HIGH);

  delay(20);

  digitalWrite(MA735_CS, LOW);
  uint16_t rd = SPI.transfer16(0);
  digitalWrite(MA735_CS, HIGH);
  SPI.endTransaction();
  return rd >> 8;
}

// 角度の読み取り
uint16_t readAngle() {
  SPI.beginTransaction(settings);
  digitalWrite(MA735_CS, LOW);
  uint16_t rd = SPI.transfer16(0);  // 角度の読み込み
  digitalWrite(MA735_CS, HIGH);
  SPI.endTransaction();
  return rd;
}

// コマンドの実行
void executeCommand(String command, String arg1, String arg2) {
  uint8_t regValue = 0;
  if (command == "d") {       // 全レジスタ表示
    Serial.print("0x00 Z[7:0]\n");
    regValue = readReg(0x0);
    printBinary8(regValue);

    Serial.print("0x01 Z[15:8]\n");
    regValue = readReg(0x1);
    printBinary8(regValue);

    Serial.print("0x02 BCT[7:0]\n");
    regValue = readReg(0x2);
    printBinary8(regValue);

    Serial.print("0x03 ETY:1 ETX:0\n");
    regValue = readReg(0x3);
    printBinary8(regValue);

    Serial.print("0x04 PPT[1:0]:6 ILIP[3:0]:2\n");
    regValue = readReg(0x4);
    printBinary8(regValue);

    Serial.print("0x05 PPT[9:2]\n");
    regValue = readReg(0x5);
    printBinary8(regValue);

    Serial.print("0x06 MGLT[2:0]:5 MGHT[2:0]:2\n");
    regValue = readReg(0x6);
    printBinary8(regValue);

    Serial.print("0x09 RD:7\n");
    regValue = readReg(0x9);
    printBinary8(regValue);

    Serial.print("0x0E FW[7:0]\n");
    regValue = readReg(0xE);
    printBinary8(regValue);

    Serial.print("0x10 HYS[7:0]\n");
    regValue = readReg(0x10);
    printBinary8(regValue);

    Serial.print("0x1B MGH:7 MGL:6\n");
    regValue = readReg(0x1B);
    printBinary8(regValue);
  }
  else if (command == "m") {      // 角度出力間隔設定
    angle_interval = parseArgument(arg1);
  }
  else if (command == "ne") {      // 角度出力間隔設定
    crank_interval = parseArgument(arg1);
  }
  else if (command == "w") {      // レジスタ書き込み
    uint8_t reg = parseArgument(arg1);
    uint8_t val = parseArgument(arg2);
    regValue = writeReg(reg, val);
    printBinary8(regValue);
  }
  else if (command == "r") {      // レジスタ読み込み
    uint8_t reg = parseArgument(arg1);
    regValue = readReg(reg);
    printBinary8(regValue);
  }
  else if (command == "z") {      // ゼロ位置設定
    // 現在の角度を取得
    int setangle = readAngle();
    Serial.print("angle: ");
    Serial.print(setangle);
    Serial.println(" -> 0");
    // 角度を16bitの2進数に変換
    String setangle_bin;
    setangle_bin = String(setangle, BIN);
    if(16 > setangle_bin.length()) {
      String returnStr = "";
      for(int i = 0; i < 16 - setangle_bin.length(); i++){
        returnStr += '0';
      }
      setangle_bin = returnStr + setangle_bin;
    }
    // 角度を8bit+8bitに分割してレジスタに書き込み
    uint8_t val = 128 * setangle_bin.substring(0, 1).toInt()
                + 64  * setangle_bin.substring(1, 2).toInt()
                + 32  * setangle_bin.substring(2, 3).toInt()
                + 16  * setangle_bin.substring(3, 4).toInt()
                + 8   * setangle_bin.substring(4, 5).toInt()
                + 4   * setangle_bin.substring(5, 6).toInt()
                + 2   * setangle_bin.substring(6, 7).toInt()
                + 1   * setangle_bin.substring(7, 8).toInt();
    Serial.print("0x01 Z[15:8]\n");
    regValue = writeReg(1, val);
    printBinary8(regValue);
    val         = 128 * setangle_bin.substring(8, 9).toInt()
                + 64  * setangle_bin.substring(9, 10).toInt()
                + 32  * setangle_bin.substring(10, 11).toInt()
                + 16  * setangle_bin.substring(11, 12).toInt()
                + 8   * setangle_bin.substring(12, 13).toInt()
                + 4   * setangle_bin.substring(13, 14).toInt()
                + 2   * setangle_bin.substring(14, 15).toInt()
                + 1   * setangle_bin.substring(15, 16).toInt();
    Serial.print("0x00 Z[7:0]\n");
    regValue = writeReg(0, val);
    printBinary8(regValue);  
  }
  else if (command == "cw") {     // 時計回りにセット
    uint8_t val = readReg(0x9);
    val &= 0x7F;                    // 0x80ビットをクリア(8bit目を0にする)
    Serial.print("0x09 RD:7\n");      
    regValue = writeReg(9, val);
    printBinary8(regValue);  
  }
  else if (command == "ccw") {    // 反時計回りにセット
    uint8_t val = readReg(0x9);
    val |= 0x80;                    // 0x80ビットをセット(8bit目を1にする)
    Serial.print("0x09 RD:7\n");
    regValue = writeReg(9, val);
    printBinary8(regValue);  
  }
  else if (command == "ppt") {      // 1回転当たりのパルス数設定
    // 現在の設定を読み込んで表示
    uint8_t val1 = readReg(0x5);    // 0x5レジスタの値を取得
    uint8_t val2 = readReg(0x4);    // 0x4レジスタの値を取得
    int val_now = 2 * ((val2 >> 6) / 2) + 1 * ((val2 >> 6) % 2) + val1 * 4 + 1; // 現在のパルス数設定を計算    
    Serial.print("ppt : ");
    Serial.print(val_now);
    Serial.print(" -> ");
    Serial.println(arg1);
    // 指定されたパルス数を10bitの2進数に変換
    int pulsecount = parseArgument(arg1) - 1;
    String pulsecount_bin;
    pulsecount_bin = String(pulsecount, BIN);
    if(10 > pulsecount_bin.length()) {
      String returnStr = "";
      for(int i = 0; i < 10 - pulsecount_bin.length(); i++){
        returnStr += '0';
      }
      pulsecount_bin = returnStr + pulsecount_bin;
    }
    // パルス数を8bit+2bitに分割してレジスタに書き込み
    val1 = 128 * pulsecount_bin.substring(0, 1).toInt()
         + 64  * pulsecount_bin.substring(1, 2).toInt()
         + 32  * pulsecount_bin.substring(2, 3).toInt()
         + 16  * pulsecount_bin.substring(3, 4).toInt()
         + 8   * pulsecount_bin.substring(4, 5).toInt()
         + 4   * pulsecount_bin.substring(5, 6).toInt()
         + 2   * pulsecount_bin.substring(6, 7).toInt()
         + 1   * pulsecount_bin.substring(7, 8).toInt(); 
    val2 &=00111111;                                       //上位2ビットをクリア（0に設定）
    val2 |= (pulsecount_bin.substring(8, 10).toInt() << 6);   // 新しい上位2ビットを設定
    Serial.print("0x04 PPT[1:0]:6 ILIP[3:0]:2\n");
    regValue = writeReg(4, val2);
    printBinary8(regValue);
    Serial.print("0x05 PPT[9:2]\n");
    regValue = writeReg(5, val1);
    printBinary8(regValue);
  }
  else {
    printHelp();

  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // SPI.setRX(9);  // (XIAO RP2040 pin 9 -> MISO)
  // SPI.setCS(7);  // (XIAO RP2040 pin 7 -> SS)
  // SPI.setSCK(8); // (XIAO RP2040 pin 8 -> SCK)
  // SPI.setTX(10); // (XIAO RP2040 pin 10 -> MOSI)
  SPI.begin();
  pinMode(MA735_CS, OUTPUT);  
  pinMode(G_IN, INPUT_PULLUP); // カムパルスセンサをプルアップ入力に設定

  digitalWrite(MA735_CS, HIGH);

  delay(5000);
  printHelp();
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  // シリアルから1行読み込む
    input.trim();                                 // 前後の空白を削除

    // 入力をスペースで分割
    int spaceIndex1 = input.indexOf(' ');
    int spaceIndex2 = input.indexOf(' ', spaceIndex1 + 1);

    String command = input.substring(0, spaceIndex1);
    String arg1 = (spaceIndex1 == -1) ? "" : input.substring(spaceIndex1 + 1, spaceIndex2);
    String arg2 = (spaceIndex2 == -1) ? "" : input.substring(spaceIndex2 + 1);

    // コマンドに応じた処理を実行
    executeCommand(command, arg1, arg2);
  }
  if (angle_interval) {
    uint32_t curr = millis();
    uint32_t diff = curr - angle_last;
    if (diff >= angle_interval) {
      Serial.println(readAngle());
      angle_last = curr;
    }
  }

  if (crank_interval) {
    uint32_t curr = millis();
    uint32_t diff = curr - crank_last;
    if (diff >= crank_interval) {
      Ne_deg = readMA735SPI();              // MA735磁気エンコーダSPIの値を取得
      Serial.println(Ne_deg, 1);            // クランク角を表示
      crank_last = curr;
    }
  }

  if (digitalRead(G_IN)) {  // カムパルスがONの場合
    if (!G_Pulse) {
      G_Pulse = true;
      G_Pulse_Flag = true;
      Serial.println("G_Pulse_Flag: true");
    }
  }
  else {                    // カムパルスがOFFの場合
    if (G_Pulse) {
      G_Pulse = false;
    }
  }
}