enum btState {
  softKill,
  hardKill,
  search,
  searchEngage
};

volatile btState permission;
btState lastPermission;
 
void setup() {
 Serial.begin(9600); // Default connection rate for our BT module
 permission = softKill;
 lastPermission = softKill;
 pinMode(13, OUTPUT);
}
 
void loop() {
  if(permission == searchEngage && permission != lastPermission) {
    digitalWrite(13, HIGH);
  }
  else if(permission == softKill && permission != lastPermission) {
    digitalWrite(13, LOW);
  }
  else if(permission == hardKill && permission != lastPermission) {
    //Stop all immediately -- turn all states to off, polling style system since stop is important
  }
  else {
    //We are in search state
  }
  lastPermission = permission;
}

void serialEvent() {
  noInterrupts();
  char state = Serial.read();
  if(state == '2')
    permission = search;
  else if(state == '1')
    permission = searchEngage;
  else if(state == '3')
    permission = hardKill;
  else
    permission = softKill;
  interrupts();
}
