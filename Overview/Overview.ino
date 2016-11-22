#include <Servo.h>
#include <Stepper.h>
#include <Wire.h>

volatile int posX[1];
volatile int posY[1];
volatile int posQuad[1];
volatile int dist[1];
volatile int i = 0;

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
const int servPin = 10;
volatile int vertPos = 0;

/*Laser Setup*/
const int lasPin = 11;
/*Optical Sensor pins*/
const int opTrig = 12;
const int opEcho = 13;
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
    Wire.beginTransmission(irSlave);
    Wire.write(d1); Wire.write(d2);
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
    
}

void loop() {
    //Wait Here until we get into search or engage state
    if(state == search || state == engage){
        findTarget();
        findDistance(); /*May want to put this function call in the findTargets() main loop*/
    }
    if(posX[0] != -1){
        //function call targetConf()
    }
    if(state == engage){
        //function, fire
    }
    /***********************************************/
    /*Clear all Variables, go back to resting state*/
    /***********************************************/
}

void findTarget(){
    int rot;
    int j;
    byte irInputx[4];
    byte irInputy[4];
    //IR Sensor: 33 degrees horizontal, 23 degrees verticle view range 1023x1023 grid
    //For grid ignore 93 grid points to the right (makes for perfect 30 degree view range)
    //0.032258 degrees per horizontal grid point
    //0.022483 degrees per vertical grid point
    //Turret to rotate to 6 positions (180 degree covered)
    for(rot = 0; rot < 6; rot++){       //Track 6 rotations
        /*Ready to read IR sensor*/
        Wire.beginTransmission(irAddress);
        Wire.write(0x36);
        Wire.endTransmission();
 
        Wire.requestFrom(irAddress, 16);        // Request the 2 byte heading (MSB comes first)
        /*Clear data buffer*/
        for (i = 0; i < 16; i++){
           irInputx[4]=0; //!!This is out of bounds
        }
        j = 0;
        /*Read in IR values*/
        while(Wire.available() && j < 16) { 
            irInputx[j] = Wire.read();
            j++;
            /*MAY BE ERROR HERE!!!*/
            irInputy[j] = Wire.read();
            j++;
            /*END POSSIBLE ERROR*/
        }
        if(irInputx[i] != (1023 - 93) && irInputy[i] != 1023){ //If target is found in range
            posX[i] = irInputx[i];                          //Store x coordinate
            posY[i] = irInputy[i];                          //Store y coordinate
            posQuad[i] = rot;                            //Store quadrant (0 - 5) that the target is in
            i++;                                         //increment number of targets
        }
        digitalWrite(stepDir, 1);                        //Set motor direction
        myStepper.step(stepQuad);
    }
    //Return to main loop
}

void findDistance(){
    int j;
    int curHorz;
    int curVert;
    int lastHPos;
    int lastVPos;
    
    for(j = 0; j < i; j++){
        curHorz = stepQuad * posQuad[j];
        curHorz += (posX[j] * gridRatHorz) / .9; //Steps to take to get to horizontal position
        myStepper.step(curHorz);
        curVert = posY[j] * gridRatHorz;  //Find elevation
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
    while(state != engage){}//!!Need a better way to do this.
}

void fire(){
    int j;
    int curHorz;
    int curVert;
    for(j = 0; j < i; j++){
        checkState(false); //Need to check return.
        curHorz = stepQuad * posQuad[j];
        curHorz += (posX[j] * gridRatHorz) / .9; //Steps to take to get to horizontal position
        myStepper.step(curHorz);
        curVert = posY[j] * gridRatHorz;  //Find elevation
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
  while(state=softKill){}
  
  //If we are in a firing function, prepFire should be fed true, if so, wait here unless state = engage
  while((prepFire==true) && (state!=engage)){}
  
  //If at any point the state becomes hardkill, return -1
  if(state=hardKill)
    return(-1);
  //else return 0.
  else
    return(0);
}

