#include <Servo.h>

volatile int posX[];
volatile int posY[];
volatile int posQuad[];
volatile int dist[];
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
const int stepDir 8;
const int stepStp 9;
/*Gear Conversion
 * RW: Switched from integer math to double math to avoid truncation, please advise if this is wrong.
 */
double gearRat = 2.0 / 1.0;             //Gear rotations per stepper rotations
double gridRatHorz = 30.0/930.0;        //Horizontal grid ratio
double gridRatVert = 30.0/1023.0;       //Vertical grid ratio
int stepQuad = (int)((30.0 * gearRat) / .9); //How many steps to turn 30 degrees

/*Servo Setup*/
const int servPin 10;
volatile int vertPos = 0;

/*Laser Setup*/
const int lasPin 11;
/*Optical Sensor pins*/
const int opTrig 12;
const int opEcho 13;
/*IR Contants*/
const int irAddress = 0xB0;
volatile int irSlave;

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
    Write_2bytes(0x30,0x01); delay(10);
    Write_2bytes(0x30,0x08); delay(10);
    Write_2bytes(0x06,0x90); delay(10);
    Write_2bytes(0x08,0xC0); delay(10);
    Write_2bytes(0x1A,0x40); delay(10);
    Write_2bytes(0x33,0x33); delay(10);
    delay(100);
    //Setup for Stepper Motor
    pinMode(stepDir, OUTPUT);
    pinMode(stepStp, OUTPUT);
    digitalWrite(stepDir, 0);
    digitalWrite(stepStp, 0);
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
        findTargets();
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
        Wire.beginTransmission(slaveAddress);
        Wire.write(0x36);
        Wire.endTransmission();
 
        Wire.requestFrom(slaveAddress, 16);        // Request the 2 byte heading (MSB comes first)
        /*Clear data buffer*/
        for (i = 0; i < 16; i++){
           irBuff[i]=0; 
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
        if(irInputx != (1023 - 93) && irInputy != 1023){ //If target is found in range
            posX[i] = irInputx;                          //Store x coordinate
            posY[i] = irInputy;                          //Store y coordinate
            posQuad[i] = rot;                            //Store quadrant (0 - 5) that the target is in
            i++;                                         //increment number of targets
        }
        /************************************************************/
        /*May want to call findDistance here rather than in the loop*/
        /************************************************************/
        /*Turn 30 degrees*/
        digitalWrite(stepDir, 1);                        //Set motor direction
        for(j = 0; j < stepQuad; j++){                   //Step 33 times (30 degrees)
            digitalWrite(stepStp, 1);                    //Take one step
            delay(1);
        }
    }
    /*Loop to return turret to 0 degrees*/
    /*digitalWrite(stepDir, 0);                        //Set motor direction
    for(j = 0; j < (stepQuad * 6); j++){                         //Step 33 times (30 degrees)
        digitalWrite(stepStp, 1);                    //Take one step
        delay(1);
    }
    */
    returnToDefault();
    //Return to main loop
}

void findDistance(){
    int j;
    int curHorz;
    int curVert;
    for(j = 0; j < i; j++){
        curHorz = stepQuad * posQuad[j];
        curHorz += (posX[j] * gridRatHorz * gearRat) / .9; //Steps to take to get to horizontal position
        digitalWrite(stepDir, 1);                          //Set motor direction
        for(j = 0; j < curHorz; j++){                      //Step X times (30 degrees)
            digitalWrite(stepStp, 1);                      //Take one step
            delay(1);
        }
        curVert = posY[j] * gridRatHorz;  //Find elevation
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
        digitalWrite(stepDir, 0);                          //Set motor direction
        for(j = 0; j < curHorz; j++){                      //Step X times (30 degrees)
            digitalWrite(stepStp, 1);                      //Take one step
            delay(1);
        }
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
        //For safety in this section, we will be checking the state several times through this whole section.
        while(state == softKill || state == search) { //We want to be waiting here until we get into the engage state or hardKill.
          digitalWrite(lasPin, LOW);//Again, just to make sure.
        }
        if(state == hardKill) {
            digitalWrite(lasPin, LOW);//Just to make sure it's off.
            returnToDefault();//Function not yet implemented.  Will be moving the position back to default.
            return;
        }
        curHorz = stepQuad * posQuad[j];
        curHorz += (posX[j] * gridRatHorz * gearRat) / .9; //Steps to take to get to horizontal position
        digitalWrite(stepDir, 1);                          //Set motor direction
        for(j = 0; j < curHorz; j++){                      //Step X times (30 degrees)
            //We need to run our checks here as well.
            while(state == softKill || state == search) { //We want to be waiting here until we get into the engage state or hardKill.
              digitalWrite(lasPin, LOW);//Again, just to make sure.
            }
            if(state == hardKill) {
                digitalWrite(lasPin, LOW);//Just to make sure it's off.
                returnToDefault();//Function not yet implemented.  Will be moving the position back to default.
                return;
            }
            digitalWrite(stepStp, 1);                      //Take one step
            delay(1);
        }
        curVert = posY[j] * gridRatHorz;  //Find elevation
        myservo.write(curVert);           //Adujst elevation
        //One last check before using the laser.
        while(state == softKill || state == search) { //We want to be waiting here until we get into the engage state or hardKill.
          digitalWrite(lasPin, LOW);//Again, just to make sure.
        }
        if(state == hardKill) {
            digitalWrite(lasPin, LOW);//Just to make sure it's off.
            returnToDefault();//Function not yet implemented.  Will be moving the position back to default.
            return;
        }
        /*Toggle laser*/
        digitalWrite(lasPin, HIGH);
        delay(1000);
        digitalWrite(lasPin, LOW);
        /*Reset to default positions*/
        /*myservo.write(-curVert);
        digitalWrite(stepDir, 0);                          //Set motor direction
        for(j = 0; j < curHorz; j++){                      //Step X times (30 degrees)
            digitalWrite(stepStp, 1);                      //Take one step
            delay(1);
        }
        */
        returnToDefault();
    }
}

void serialEvent() {
  cli();
  switch Serial.read() {
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

void returnToDefault() {
  //This function should return the our direction to the default position.
}

int checkState() {
  //Condense the blocks to a single function that returns -1 on hardKill.  Should simplify the loop code.
}

