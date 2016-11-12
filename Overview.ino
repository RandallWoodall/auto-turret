volatile int posX[];
volatile int posY[];
volatile int posQuad[];
volatile int dist[];
volatile int i = 0;
volatile double angleX;
volatile double angleY;

void setup() {
    //Setup up for Bluetooth
    //Setup for IR
    //Setup for Stepper Motor
    //Setup for servo
    //Setup for Optical
    //Setup for laser
}

void loop() {
    //Wait Here until we recieve a a GO from the bluetooth
    if(bluetooth == GO){
        //function call findTargets()
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
    //IR Sensor: 33 degrees horizontal, 23 degrees verticle view range 1023x1023 grid
    //For grid ignore 93 grid points to the right (makes for perfect 30 degree view range)
    //0.032258 degrees per horizontal grid point
    //0.022483 degrees per vertical grid point
    //Turret to rotate to 6 positions (180 degree covered)
    /*************************/
    /*GEAR RATIOS FOR STEPPER*/
    /*************************/
    for(runs = 0; rot < 6; rot++){ //Track 6 rotations
        if(irInputx != (1023 - 93) && irInputy != 1023){ //If target is found
            posX[i] = irInputx; //Store x coordinate
            posY[i] = irInputy; //Store y coordinate
            posQuad[i] = rot;   //Store quadrant (0 - 5) that the target is in
        }
    }
    /***************************************/
    /*ADJUST TURRET ANGLES TO FIND DISTANCE*/
    /***************************************/
    if()
    //Return to main loop
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

