int val = 0;
int new_val=0;

// include the library code:
#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12); //these pins can be changed but we will need to figure out what goes where
void setup() {
  // put your setup code here, to run once:
  lcd.begin(16,2);
  pinMode(A15, INPUT);
  Serial.begin(9600);
}
//Call this function to update the LCD screen with the current battery charge
void battery_Check(){
  val = analogRead(A15);
  lcd.clear();
  lcd.setCursor(0,0);
  new_val= val - 699;
  val = (new_val/290)*100;
  lcd.print(val);
  lcd.setCursor(0,1);
  lcd.print("batt: ");
  for(int i = 0; i < val/10; i++){
    lcd.write(255);
   // lcd.print(*); the write function should print a block, if not we can use the '*' as a filler
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
