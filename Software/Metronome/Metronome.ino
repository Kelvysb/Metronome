#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Button.h>
#include "pitches.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC     6
#define OLED_CS     7
#define OLED_RESET  8
#define MENU_RESET 30000

#define BPM_MEM 0
#define COMPASS_MEM 1
#define LIGHT_MEM 2
#define BEEP_MEM 3

#define LED1 A0
#define LED2 A1
#define BUTTON_UP 2
#define BUTTON_DOWN 3
#define BUTTON_OK 4


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RESET, OLED_CS);

unsigned long lastMilli = 0;
int bpm = 60;
int compass = 4;
bool lightOn = true;
bool beepOn = true;
bool displayUpdate = true;

Button button_ok(BUTTON_OK);
Button button_up(BUTTON_UP);
Button button_down(BUTTON_DOWN);

int compassPos = 1;
long beatTimer = 0;
int menu = 0;
int currentTempo = 0;
int menuResetTimer = 0;

// 'AudioOn', 16x16px
const unsigned char bmp_AudioOn [] PROGMEM = {
	0x01, 0x10, 0x03, 0x08, 0x05, 0x24, 0x09, 0x12, 0x71, 0x4a, 0x81, 0x29, 0x81, 0x25, 0x81, 0x15, 
	0x81, 0x15, 0x81, 0x25, 0x81, 0x29, 0x71, 0x4a, 0x09, 0x12, 0x05, 0x24, 0x03, 0x08, 0x01, 0x10
};
// 'LightOff', 16x16px
const unsigned char bmp_LightOff [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08, 0x17, 0xe8, 
	0x10, 0x08, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'LightOn', 16x16px
const unsigned char bmp_LightOn [] PROGMEM = {
	0x81, 0x01, 0x41, 0x02, 0x20, 0x04, 0x13, 0xc8, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0xdf, 0xf8, 
	0x1f, 0xfb, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0, 0x13, 0xc8, 0x20, 0x04, 0x40, 0x82, 0x80, 0x81
};
// 'AudioOff', 16x16px
const unsigned char bmp_AudioOff [] PROGMEM = {
	0x01, 0x00, 0x03, 0x00, 0x05, 0x00, 0x09, 0x00, 0x71, 0x00, 0x81, 0x41, 0x81, 0x22, 0x81, 0x14, 
	0x81, 0x08, 0x81, 0x14, 0x81, 0x22, 0x71, 0x41, 0x09, 0x00, 0x05, 0x00, 0x03, 0x00, 0x01, 0x00
};

void setup() {
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  } 

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  button_ok.begin();
  button_up.begin();
  button_down.begin();

  display.setTextColor(SSD1306_WHITE); 
  display.cp437(true);

  Load();
}

void loop() {
  int delta = GetDelta();
  DoBeat(delta);
  Display(delta);
  CheckButons(delta);
  ResetMenu(delta);
}

void Display(unsigned long delta){
  if(displayUpdate == true){
    display.clearDisplay();
    display.setTextSize(3);   
    display.setCursor(35, 20);
    display.print(String(bpm));
    display.setTextSize(1);
    display.setCursor(90, 35);
    display.print("bpm");

    display.setTextSize(2);   
    display.setCursor(5, 47);
    display.print(String(compass));
    display.setTextSize(1);
    display.setCursor(17, 52);
    display.print("T");

    display.setTextSize(1);   
    display.setCursor(50, 54);
    display.print(GetName());  
    
    if(beepOn){
      display.drawBitmap(3, 3, bmp_AudioOn, 16, 16, 1);
    }else{
      display.drawBitmap(3, 3, bmp_AudioOff, 16, 16, 1);
    }

    if(lightOn){
      display.drawBitmap(109, 3, bmp_LightOn, 16, 16, 1);
    }else{
      display.drawBitmap(109, 3, bmp_LightOff, 16, 16, 1);
    }

    if(menu == 1){
      display.drawRect(45, 51, 83, 13, 1);
    }else if(menu == 2){      
      display.drawRect(0, 44, 27, 20, 1);
    }else if(menu == 3){
      display.drawRect(0, 0, 22, 22, 1);
    }else if(menu == 4){
      display.drawRect(106, 0, 22, 22, 1);      
    }

    display.display();
    Save();
    displayUpdate = false;
    menuResetTimer = MENU_RESET;
  }
}

void ResetMenu(unsigned long delta){
  if(menu != 0){
    menuResetTimer -= delta;
    if(menuResetTimer <= 0){      
      menu = 0;
      displayUpdate = true;
    }
  }
}

void CheckButons(unsigned long delta){
  if(button_ok.pressed()){
    if(menu >= 4){
      menu = 0;          
      displayUpdate = true;          
    }else{
      menu++;
      displayUpdate = true;
    }  
  }

  if(menu == 0){
    if(button_up.pressed()){
      if(bpm < 250){
        bpm++;            
        displayUpdate = true;
      }
    }else if (button_down.pressed()){
      if(bpm > 1){
        bpm--;
        displayUpdate = true;
      }
    }
  }else if(menu == 1){
    if(button_up.pressed()){
      SetByTempo(true);
      displayUpdate = true;          
    }else if (button_down.pressed()){
      SetByTempo(false);
      displayUpdate = true;
    }
  }else if(menu == 2){
    if(button_up.pressed()){
      if(compass < 9){
        compass++;
      }
      displayUpdate = true;  
    }else if (button_down.pressed()){
      if(compass > 1){
        compass--;
      }
      displayUpdate = true;  
    }
  }else if(menu == 3){
    if(button_up.pressed() || button_down.pressed()){
      beepOn = !beepOn;
      displayUpdate = true;
    }
  }else if(menu == 4){
    if(button_up.pressed() || button_down.pressed()){
      lightOn = !lightOn;
      displayUpdate = true;
    }
  }
}

void DoBeat(unsigned long delta){
  if(beatTimer >= (60000/bpm)){
    if(compassPos >= compass && compass > 1){
      compassPos = 0;
      Beat(true);
    }else{
      Beat(false);
    }
    beatTimer = 0;
    compassPos++;
  }
  beatTimer += delta;
}

void Beat(bool mark){
  if(!mark){    
    if(lightOn){
      digitalWrite(LED1, HIGH);
    }
    if(beepOn){
      tone(5, NOTE_G3);
    }
  }else{
    if(lightOn){
      digitalWrite(LED2, HIGH);
    }
    if(beepOn){
      tone(5, NOTE_G6);
    }
  }
  delay(20);
  noTone(5);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
}

unsigned long GetDelta(){
  unsigned long current = millis();
  unsigned long delta = current - lastMilli;
  lastMilli = current;
  return delta;
}

void Load(){
  bpm = EEPROM.read(BPM_MEM);
  compass = EEPROM.read(COMPASS_MEM);
  lightOn = EEPROM.read(LIGHT_MEM);
  beepOn = EEPROM.read(BEEP_MEM);
  if(compass > 9){
    compass = 4;
    Save();
  }
  if(bpm == 255){
    bpm = 120;
    Save();
  }
}

void Save(){
  EEPROM.write(BPM_MEM, bpm);
  EEPROM.write(COMPASS_MEM, compass);
  EEPROM.write(LIGHT_MEM, lightOn);
  EEPROM.write(BEEP_MEM, beepOn);
}

void SetByTempo(bool up){
  if(up && currentTempo < 11){
    currentTempo++;
  }else if(currentTempo > 0){
    currentTempo--;
  }

  if(currentTempo == 0){
    bpm = 10;
  }else if(currentTempo == 1){
    bpm = 20;
  }else if(currentTempo == 2){
    bpm = 40;
  }else if(currentTempo == 3){
    bpm = 60;
  }else if(currentTempo == 4){
    bpm = 66;
  }else if(currentTempo == 5){
    bpm = 76;
  }else if(currentTempo == 6){
    bpm = 108;
  }else if(currentTempo == 7){
    bpm = 112;
  }else if(currentTempo == 8){
    bpm = 120;
  }else if(currentTempo == 9){
    bpm = 168;
  }else if(currentTempo == 10){
    bpm = 176;
  }else if(currentTempo == 11){
    bpm = 200;
  }
}

String GetName(){
  if(bpm < 20){
    currentTempo = 0;
    return "Gravissimo";
  }else if(bpm >= 20 && bpm < 40){
    currentTempo = 1;
    return "Grave";
  }else if(bpm >= 40 && bpm < 60){
    currentTempo = 2;
    return "Largo";
  }else if(bpm >= 60 && bpm < 66){
    currentTempo = 3;
    return "Larghetto";
  }else if(bpm >= 66 && bpm < 76){
    currentTempo = 4;
    return "Adagio";
  }else if(bpm >= 76 && bpm < 108){
    currentTempo = 5;
    return "Andante";
  }else if(bpm >= 108 && bpm < 112){
    currentTempo = 6;
    return "Moderato";
  }else if(bpm >= 112 && bpm < 120){
    currentTempo = 7;
    return "Allegretto";
  }else if(bpm >= 120 && bpm < 168){
    currentTempo = 8;
    return "Allegro"; 
  }else if(bpm >= 168 && bpm < 176){
    currentTempo = 9;
    return "Vivace";
  }else if(bpm >= 176 && bpm < 200){
    currentTempo = 10;
    return "Presto";
  }else{
    currentTempo = 11;
    return "Prestissimo";
  }
}

