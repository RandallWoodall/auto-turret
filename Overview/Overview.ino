volatile int posX[];
volatile int posY[];
volatile int posQuad[];
volatile int dist[];
volatile int i = 0;
volatile double angleX;
volatile double angleY;

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
    //Setup for servo
    //Setup for Optical
    //Setup for laser
}

void loop() {
    //Wait Here until we recieve a a GO from the bluetooth
    if(bluetooth == GO){
        findTargets();
        findDistance(); /*May want to put this function call in the findTargets() main loop*/
    }
    if(posX[0] != -1){
        //function call targetConf()
    }
    if(confirmation == 1){
        //function, fire
    }
    /***********************************************/
    /*Clear all Variables, go back to resting state*/
    /***********************************************/
}

void findTarget(){
    int rot;
    byte irInputx[4];
    byte irInputy[4];
    //IR Sensor: 33 degrees horizontal, 23 degrees verticle view range 1023x1023 grid
    //For grid ignore 93 grid points to the right (makes for perfect 30 degree view range)
    //0.032258 degrees per horizontal grid point
    //0.022483 degrees per vertical grid point
    //Turret to rotate to 6 positions (180 degree covered)
    /*************************/
    /*GEAR RATIOS FOR STEPPER*/
    /*************************/
    for(rot = 0; rot < 6; rot++){ //Track 6 rotations
        /*Ready to read IR sensor*/
        Wire.beginTransmission(slaveAddress);
        Wire.write(0x36);
        Wire.endTransmission();
 
        Wire.requestFrom(slaveAddress, 16);        // Request the 2 byte heading (MSB comes first)
        /*Clear data buffer*/
        for (i=0;i<16;i++){
           irBuff[i]=0; 
        }
        i=0;
        /*Read in IR values*/
        while(Wire.available() && i < 16) { 
            irInputx[i] = Wire.read();
            i++;
            /*MAY BE ERROR HERE!!!*/
            irInputy[i] = Wire.read();
            i++;
            /*END POSSIBLE ERROR*/
        }
        if(irInputx != (1023 - 93) && irInputy != 1023){ //If target is found in range
            posX[i] = irInputx;                          //Store x coordinate
            posY[i] = irInputy;                          //Store y coordinate
            posQuad[i] = rot;                            //Store quadrant (0 - 5) that the target is in
        }
        /*May want to call findDistance here rather than in the loop*/
        /**************************/
        /*Rotate Turret 30 degrees*/
        /**************************/
    }
    //Return to main loop
}

void findDistance(){
    /***************************************/
    /*ADJUST TURRET ANGLES TO FIND DISTANCE*/
    /***************************************/
}

void targetConf(){
    //Return to a resting stat
    //Wait to recieve bluetooth confirmation
    //Return to main loop
}

void fire(){
    int tar;
    for(tar = 0; tar <= i; tar++){
        //rotate turret to point at target
        //Turn laser on
        //Rinse and repeat
    }
    //Return to main loop
}

