#include <Servo.h>
#include <Stepper.h>
#include <Wire.h>

volatile int posTargX[4];
volatile int posTargY[4];
volatile int posQuad[4];
volatile int dist[4];
byte data_buf[16];
volatile int endGame = 0;
int length = 0;

//Enumeration that will be set in a variable called state -- controlled via bluetooth
enum btState {
  hardKill,
  softKill,
  search,
  engage
};

//State variable
volatile btState state;

/*Stepper Motor Setup*/
//.9 degrees per step
const int stepDir = 8;
const int stepStp = 9;
const int stepsPerRevolution = 200;
Stepper myStepper(stepsPerRevolution, 2, 3, 4, 5);
/*Gear Conversion
 * RW: Switched from integer math to double math to avoid truncation, please advise if this is wrong.
 */
double gridRatHorz = 30.0/930.0;        //Horizontal grid ratio
double gridRatVert = 30.0/1023.0;       //Vertical grid ratio
int stepQuad = (int)((30.0) / .9); //How many steps to turn 30 degrees

/*Servo Setup*/
Servo myservo;
const int servPin = 9;
volatile int vertPos = 0;

/*Laser Setup*/
const int lasPin = 6;

/*Optical Sensor pins*/
const int opTrig = 8;
const int opEcho = 7;

/*IR Contants*/
const int irAddress = 0xB0;
volatile int irSlave;

//Prototypes
void irComm(byte d1, byte d2);
void findTargets();
void findDistance();
void targetConf();
void fire();
int checkState(bool prepFire);

/*Communication Setup for IR Sensor*/
void irComm(byte d1, byte d2)
{
    Serial.println(irSlave);
    Wire.beginTransmission(irSlave);
    Wire.write(d1); 
    Wire.write(d2);
    Wire.endTransmission();
}

void setup() {
    //Setup up for Bluetooth
    Serial.begin(9600);
    state = softKill;
    //Setup for IR
    irSlave = irAddress >> 1;   // This results in 0x21 as the address to pass to TWI
    Wire.begin();
    irComm(0x30,0x01); delay(10);
    irComm(0x30,0x08); delay(10);
    irComm(0x06,0x90); delay(10);
    irComm(0x08,0xC0); delay(10);
    irComm(0x1A,0x40); delay(10);
    irComm(0x33,0x33); delay(10);
    delay(100);
    //Setup for Stepper Motor
    // set the speed at 60 rpm:
    myStepper.setSpeed(60);
    //Setup for servo
    myservo.attach(servPin);
    //Setup for Optical
    pinMode(opTrig, OUTPUT);
    pinMode(opEcho, INPUT);
    //Setup for laser
    pinMode(lasPin, OUTPUT);
    Serial.println("Setup complete.");
}

void loop() {
    //Wait Here until we get into search or engage state
    if(state == search || state == engage){
        Serial.println("Made it inside the first if in loop()");
        findTarget();
        findDistance(); /*May want to put this function call in the findTargets() main loop*/
    }
    if(posTargX[0] != -1){
        //function call targetConf()
    }
    if(state == engage){
        //function, fire
    }
    if(endGame == 1){
      posTargX[length] = -1;
      posTargY[length] = -1;
      posQuad[length] = -1;
      dist[length] = -1;
    }
    state = softKill;
}

void findTarget(){
    int s;
    int j;
    int stepDef = 0;
    int posX[4];
    int posY[4];
    //Target number is 0
    length = 0;
  
    for(j = 0; j < 7; j++) {
        //Tell the sensor to prepare data for return;
        Wire.beginTransmission(irSlave);
        Wire.write(0x36);
        Wire.endTransmission();
  
        //Request the header.
        Wire.requestFrom(irSlave, 16);
        for(int i = 0; i < 16; i++)
            data_buf[i] = 0;
            //And the data
        for(int i = 0; Wire.available() && i < 16; i++)
            data_buf[i] = Wire.read();
  
        //Compile the data.
        posX[0] = data_buf[1];
        posY[0] = data_buf[2];
        s   = data_buf[3];
        posX[0] += (s & 0x30) <<4;
        posY[0] += (s & 0xC0) <<2;
  
        posX[1] = data_buf[4];
        posY[1] = data_buf[5];
        s   = data_buf[6];
        posX[1] += (s & 0x30) <<4;
        posY[1] += (s & 0xC0) <<2;
 
        posX[2] = data_buf[7];
        posY[2] = data_buf[8];
        s   = data_buf[9];
        posX[2] += (s & 0x30) <<4;
        posY[2] += (s & 0xC0) <<2;
 
        posX[3] = data_buf[10];
        posY[3] = data_buf[11];
        s   = data_buf[12];
        posX[3] += (s & 0x30) <<4;
        posY[3] += (s & 0xC0) <<2;
        
        for(int i = 0; i < 4 && length < 4; i++) {
            if(posX[i] == 1023) {
                posX[i] = 0;
                posY[i] = 0;
            }
            else {
                posTargX[length] = posX[i];
                posTargY[length] = posY[i];
                posQuad[length] = j;
                length++;
                break;
            }
        }
        //Display the data
        for(int i=0; i<4; i++) {
            if (posX[i] < 1000)
                Serial.print("");
            if (posX[i] < 100)  
                Serial.print("");
            if (posX[i] < 10)  
                Serial.print("");
            Serial.print( int(posX[i]) );
            Serial.print(",");
            if (posY[i] < 1000)
                Serial.print("");
            if (posY[i] < 100)  
                Serial.print("");
            if (posY[i] < 10)  
                Serial.print("");
            Serial.print( int(posY[i]) );
            if (i<3)
                Serial.print(",");
        }
        Serial.println("");
        delay(1000);
        //If we have the max number of targets, jump out of the loop.
        if(length == 4)
            break;
        //Go to the next quadrant
        myStepper.step(stepQuad);
        delay(500);
    }
    stepDef = -1 * stepQuad * j;
    myStepper.step(stepDef);
}

void findDistance(){
    Serial.println("GOT IN  FIND DISNATCE");
    int j;
    int curHorz;
    int curVert;
    int lastHPos;
    int lastVPos;
    
    for(j = 0; j < length; j++){
        curHorz = stepQuad * posQuad[j];
        curHorz += (posTargX[j] * gridRatHorz) / .9; //Steps to take to get to horizontal position
        myStepper.step(curHorz);
        curVert = posTargY[j] * gridRatHorz;  //Find elevation
        curVert = curVert - 180;
        if(curVert < 0)
            curVert = curVert * -1;
        myservo.write(curVert);           //Adujst elevation
        /*Read distance*/
        digitalWrite(opTrig, LOW);      
        delayMicroseconds(2);          
        digitalWrite(opTrig, HIGH);
        delayMicroseconds(10);
        digitalWrite(opTrig, LOW);
        dist[j] = pulseIn(opEcho,HIGH);
        /*Reset to default positions*/
        myservo.write(-curVert);
        myStepper.step(-curHorz);
    }
    
}

void targetConf(){
    Serial.println("GOT IN  TARGET CONFIRMATION");
    while(state != engage){}//!!Need a better way to do this.
}

void fire(){
    Serial.println("GOT IN  FIRE");
    int j;
    endGame = 1;
    int curHorz;
    int curVert;
    for(j = 0; j < length; j++){
        checkState(false); //Need to check return.
        curHorz = stepQuad * posQuad[j];
        curHorz += (posTargX[j] * gridRatHorz) / .9; //Steps to take to get to horizontal position
        myStepper.step(curHorz);
        curVert = posTargY[j] * gridRatHorz;  //Find elevation
        curVert = curVert - 180;
        if(curVert < 0)
            curVert = curVert * -1;
        myservo.write(curVert);           //Adujst elevation
        checkState(true); //Use the return here, as well.
        /*Toggle laser*/
        digitalWrite(lasPin, HIGH);
        delay(1000);
        digitalWrite(lasPin, LOW);
        /*Reset to default positions*/
        myservo.write(-curVert);
        myStepper.step(-curHorz);
    }
}

void serialEvent() {
  cli();
  switch (Serial.read()) {
    case '0':
      state = hardKill;
      break;
    case '1':
      state = softKill;
      break;
    case '2':
      state = search;
      break;
    case '3':
      state = engage;
      break;
    default:
      state = softKill;
      break;
  }//End switch
  sei();
}//End serialEvent

int checkState(bool prepFire) {
  //Wait here if we are in softKill.
  while(state==softKill){}
  
  //If we are in a firing function, prepFire should be fed true, if so, wait here unless state = engage
  while((prepFire==true) && (state!=engage)){}
  
  //If at any point the state becomes hardkill, return -1
  if(state==hardKill)
    return(-1);
  //else return 0.
  else
    return(0);
}
