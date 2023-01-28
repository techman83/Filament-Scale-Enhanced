/////////////////////////////////////////////////////////////////

#include "Button2.h"




#define MY_LONGCLICK_MS  500
#define MY_DOUBLECLICK_MS 700



Button2 buttonTare;
Button2 buttonTimer;


void fct_callTare();
void fct_callCalibrate();




void click_buttonTare(Button2& btn) {
    

    if (btn.wasPressedFor() > 1000) {
      ESP_LOGV("button","tareButton fct_callCalibrate");
      fct_callCalibrate();
    }
    else{
      ESP_LOGV("button","tareButton released");
      fct_callTare();
    }
    }


int counter = 0;
unsigned long now = 0;


/////////////////////////////////////////////////////////////////

void button_setup() {
  buttonTare.begin(TARE_BUTTON_PIN,INPUT_PULLDOWN,false);
  // buttonTare.setDebounceTime(200);
  // buttonTare.setLongClickTime(2000);
  buttonTare.setTapHandler(click_buttonTare);
  // buttonTare.setDoubleClickHandler(longClick_buttonTare);
}

/////////////////////////////////////////////////////////////////

void button_loop() {
  buttonTare.loop();
}