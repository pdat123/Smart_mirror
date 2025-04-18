
#include <Arduino.h>
#include <LedControl.h>
#include <TM1637Display.h>

// Thêm chân reset STM32
#define BTN_RESET_STM PA3

// TM1637 hiển thị giờ:phút
#define CLK PB6
#define DIO PB7
TM1637Display display(CLK, DIO);

// MAX7219 hiển thị ngày / nhiệt độ / độ ẩm
LedControl lc(PA7, PA5, PA4, 1);

// Nút chuyển chế độ hiển thị
#define BTN_DATE PA0
#define BTN_TEMP PA1
#define BTN_HUMI PA2

enum DisplayMode { DATE_MODE, TEMP_MODE, HUMI_MODE };
DisplayMode mode = DATE_MODE;

String inputString = "";
bool stringComplete = false;

// Thông tin từ ESP32
String dateStr = "00000000";  // ddmmyyyy
String tempStr = "TEMP28.50";  // TEMPxx.xx
String humiStr = "HUMI70.20";  // HUMIxx.xx
int hour = 0, minute = 0;

bool colonState = true;
unsigned long lastBlink = 0;
const unsigned long blinkInterval = 500;

void setup() {
  Serial.begin(9600);
  inputString.reserve(64);

  // Cấu hình chân reset
  pinMode(BTN_RESET_STM, INPUT_PULLUP);

  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  display.setBrightness(5);

  pinMode(BTN_DATE, INPUT_PULLUP);
  pinMode(BTN_TEMP, INPUT_PULLUP);
  pinMode(BTN_HUMI, INPUT_PULLUP);
}

void loop() {
  // Kiểm tra nút reset STM32
  if(digitalRead(BTN_RESET_STM) == LOW) {
    delay(50);
    if(digitalRead(BTN_RESET_STM) == LOW) {
      NVIC_SystemReset();
    }
  }
  
  // Xử lý nháy dấu 2 chấm giờ:phút
  if (millis() - lastBlink >= blinkInterval) {
    lastBlink = millis();
    colonState = !colonState;
    uint8_t colonFlag = colonState ? 0b01000000 : 0;
    display.showNumberDecEx(hour * 100 + minute, colonFlag, true);
  }

  // Đọc UART từ ESP32
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }

  if (stringComplete) {
    stringComplete = false;

    // Chuỗi dạng: 16/04/2025,14:35,28.50,70.20
    if (inputString.length() >= 20) {
      // Ngày
      String day = inputString.substring(0, 2);
      String month = inputString.substring(3, 5);
      String year = inputString.substring(6, 10);
      dateStr = day + month + year;

      // Giờ:phút
      hour = inputString.substring(11, 13).toInt();
      minute = inputString.substring(14, 16).toInt();

      // Nhiệt độ (định dạng TEMPxx.xx)
      int tempStart = inputString.indexOf(',', 16) + 1;
      int tempEnd = inputString.indexOf(',', tempStart);
      tempStr = "TEMP" + inputString.substring(tempStart, tempEnd);

      // Độ ẩm (định dạng HUMIxx.xx)
      int humiStart = tempEnd + 1;
      humiStr = "HUMI" + inputString.substring(humiStart);

      inputString = "";
    }
  }

  // Đọc trạng thái nút
  if (digitalRead(BTN_DATE) == LOW) {
    mode = DATE_MODE;
    delay(200); // Chống dội nút
  } else if (digitalRead(BTN_TEMP) == LOW) {
    mode = TEMP_MODE;
    delay(200);
  } else if (digitalRead(BTN_HUMI) == LOW) {
    mode = HUMI_MODE;
    delay(200);
  }

  // Hiển thị lên MAX7219 (CẬP NHẬT PHẦN NÀY)
  switch (mode) {
    case DATE_MODE:
      for (int i = 0; i < 8; i++) {
        char c = dateStr.charAt(i);
        bool dp = (i == 1 || i == 3); // Dấu chấm ở vị trí 2 và 4 (dd.mm.yyyy)
        lc.setChar(0, 7 - i, c, dp);
      }
      break;

    case TEMP_MODE:
      {
        int displayPos = 0;
        for (int i = 0; i < tempStr.length(); i++) {
          char c = tempStr.charAt(i);
          if (c == '.') {
            // Bật dấu chấm cho ký tự trước đó
            if (displayPos > 0) {
              lc.setChar(0, 7 - (displayPos - 1), tempStr.charAt(i - 1), true);
            }
          } else {
            if (displayPos >= 8) break;
            lc.setChar(0, 7 - displayPos, c, false);
            displayPos++;
          }
        }
        // Xóa các digit còn thừa
        for (int i = displayPos; i < 8; i++) {
          lc.setChar(0, 7 - i, ' ', false);
        }
      }
      break;

    case HUMI_MODE:
      {
        int displayPos = 0;
        for (int i = 0; i < humiStr.length(); i++) {
          char c = humiStr.charAt(i);
          if (c == '.') {
            // Bật dấu chấm cho ký tự trước đó
            if (displayPos > 0) {
              lc.setChar(0, 7 - (displayPos - 1), humiStr.charAt(i - 1), true);
            }
          } else {
            if (displayPos >= 8) break;
            lc.setChar(0, 7 - displayPos, c, false);
            displayPos++;
          }
        }
        // Xóa các digit còn thừa
        for (int i = displayPos; i < 8; i++) {
          lc.setChar(0, 7 - i, ' ', false);
        }
      }
      break;
  }
}