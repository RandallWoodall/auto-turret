volatile char state;
char prevState;
 
void setup() {
 Serial.begin(9600); // Default connection rate for my BT module
 state = '0';
 prevState = '0';
 pinMode(13, OUTPUT);
}
 
void loop() {
  if(Serial.available()>0)
    state = Serial.read();
  if(state == '1' && state != prevState) {
    Serial.println("High");
    digitalWrite(13, HIGH);
  }
  if(state == '0' && state != prevState) {
    Serial.println("Low");
    digitalWrite(13, LOW);
  }
  prevState = state;
}
