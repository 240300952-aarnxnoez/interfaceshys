// código attiny
#include "DigiKeyboard.h"

void setup() {
}

void loop() {
  DigiKeyboard.update();
  DigiKeyboard.delay(3000);
  
  DigiKeyboard.sendKeyStroke(KEY_R, MOD_GUI_LEFT);
  DigiKeyboard.delay(600);
  
  DigiKeyboard.print("notepad");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.delay(1200);
  
  DigiKeyboard.print("aaron.martinez.nova@gmail.com");
  DigiKeyboard.delay(400);
  
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.delay(400);
  
  DigiKeyboard.print("ContrasenaSegura123!");
  
  for(;;){ }
}
