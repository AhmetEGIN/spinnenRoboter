

#include <IRremote.h> 
#include <Servo.h>    
#include <SoftwareSerial.h>
#include <MPU6050.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
//===== Globals ============================================================================

// Define USRF pins and variables
#define trigPin A3
#define echoPin A2
#define INCH 0
#define CM 1

// Define IR Remote Button Codes
#define irUp 0xE718FF00
#define irDown 0xAD52FF00
#define irRight 0xA55AFF00
#define irLeft 0xF708FF00
#define irOK 0xE31CFF00
#define ir1 0xBA45FF00
#define ir2 0xB946FF00
#define ir3 0xBC43FF00
#define ir4 0xBB44FF00
#define ir5 0xBF40FF00
#define ir6 0xBC43FF00
#define ir7 0xF807FF00
#define ir8 0xEA15FF00
#define ir9 0xF609FF00
#define ir0 0xE619FF00
#define irStar 0xE916FF00
#define irPound 0xF20DFF00

char key;

// calibration
int da =  -12,  // Left Front Pivot
    db =   10,  // Left Back Pivot
    dc =  -18,  // Right Back Pivot
    dd =   12;  // Right Front Pivot

// servo initial positions + calibration
int a90  = (90  + da),
    a120 = (120 + da),
    a150 = (150 + da),
    a180 = (180 + da);

int b0   = (0   + db),
    b30  = (30  + db),
    b60  = (60  + db),
    b90  = (90  + db);

int c90  = (90  + dc),
    c120 = (120 + dc),
    c150 = (150 + dc),
    c180 = (180 + dc);

int d0   = (0   + dd),
    d30  = (30  + dd),
    d60  = (60  + dd),
    d90  = (90  + dd);

// start points for servo
int s11 = 90; // Front Left Pivot Servo
int s12 = 90; // Front Left Lift Servo
int s21 = 90; // Back Left Pivot Servo
int s22 = 90; // Back Left Lift Servo
int s31 = 90; // Back Right Pivot Servo
int s32 = 90; // Back Right Lift Servo
int s41 = 90; // Front Right Pivot Servo
int s42 = 90; // Front Right Lift Servo

int f    = 0;
int b    = 0;
int l    = 0;
int r    = 0;
int spd  = 3;  // Speed of walking motion, larger the number, the slower the speed
int high = 0;   // How high the robot is standing

// Define 8 Servos
Servo myServo1; // Front Left Pivot Servo
Servo myServo2; // Front Left Lift Servo
Servo myServo3; // Back Left Pivot Servo
Servo myServo4; // Back Left Lift Servo
Servo myServo5; // Back Right Pivot Servo
Servo myServo6; // Back Right Lift Servo
Servo myServo7; // Front Right Pivot Servo
Servo myServo8; // Front Right Lift Servo
SoftwareSerial bluetooth(10, 11);
char d;

Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

// Set up IR Sensor
int irReceiver = 12;       // Use pin D12 for IR Sensor
IRrecv irrecv(irReceiver); // create a new instance of the IR Receiver
decode_results results;

MPU6050 git_sensor;
int ivmeX , ivmeY , ivmeZ , GyroX , GyroY , GyroZ;
int sum = 0;
#define GYRO_UPDATE_INTERVAL 10
MPU6050 mpu;

double setpoint = 0;  // Hedef açı
double Kp = 10.0;        // Proportional katsayısı
double Ki = 0.02;       // Integral katsayısı
double Kd = 2.0;        // Derivative katsayısı

double errorSum = 0.0;  // Toplam hata (Integral için)
double lastError = 0.0;  // Son hata (Derivative için)

//==========================================================================================

//===== Setup ==============================================================================

void setup()
{
  // Attach servos to Arduino Pins
  myServo1.attach(2);
  myServo2.attach(3);
  myServo3.attach(4);
  myServo4.attach(5);
  myServo5.attach(6);
  myServo6.attach(7);
  myServo7.attach(8);
  myServo8.attach(9);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  irrecv.enableIRIn(); //start the receiver

  Serial.begin (9600);
  bluetooth.begin(9600);
  Wire.begin();
  mpu.initialize();
  sum = 0;

   if(!mag.begin())
  {

    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while(1);
  }
  
  setpoint = calculateHeadingDegrees();

}

void loop()
{
  
  center_servos();  

  float headingDegrees = calculateHeadingDegrees();

  double error =(int) (setpoint - headingDegrees);


  double Pvalue = error * Kp;
  double Ivalue = errorSum * Ki;
  double Dvalue = (error - lastError) * Kd;

  double PIDvalue = Pvalue + Ivalue + Dvalue;

  lastError = error;
  errorSum += error;
  //Serial.println(PIDvalue);
  int Fvalue = (int)PIDvalue;
  Fvalue = map((int)PIDvalue, -1000, 1000, -15, 15);

 if (Fvalue > 15) {
  Fvalue = 15;
 } else if(Fvalue < -15) {
  Fvalue = -15;
 }
 

  Serial.println(Fvalue);
  if (Fvalue < -1) {

    turn_right3(-(Fvalue * 2));
    delay(300);
  } else if(Fvalue > 1) {
    turn_left2(20);
    delay(300);
  }

  delay(50);

  if(bluetooth.available()) {
    d = bluetooth.read();

    Serial.println(d);
    if (d == 'd') {
  
      setpoint -= 20;
      d == ' ';
      delay(500);
      
 
    } else if (d == 'c') {
      center_servos();

    } else if (d == 'a') {
      setpoint +=20;
      delay(500);
      d = ' ';
    } else if(d == 'w'){
      forward();
      d = ' ';
    } else if (d == 's') {
      back();
      d = ' ';
    }
  }
  

 //while (Serial.available()) {
 //    key = Serial.read();
 //    String
 //}
  if(irrecv.decode()){

    if (irrecv.decodedIRData.decodedRawData == 0xE718FF00){
      forward();
    }
  }


  if(key == 'w') {
    forward();
    key = ' ';
  }
  if(key == 'c') {
    center_servos();
    key = ' ';
  }

 

}

int calculateHeadingDegrees(){
  sensors_event_t event; 
  mag.getEvent(&event);


  float heading = atan2(event.magnetic.y, event.magnetic.x);  
  float declinationAngle = 0.6;
  heading += declinationAngle;

  //if(heading < 0)
  //heading += 2*PI;
    
  if(heading > 2*PI)
    heading -= 2*PI;

  float headingDegrees = heading * 180/M_PI; 
  
  return headingDegrees;
}

void forward()
{
  // calculation of points

  // Left Front Pivot
  a90 = (90 + da),
  a120 = (120 + da),
  a150 = (150 + da),
  a180 = (180 + da);

  // Left Back Pivot
  b0 = (0 + db),
  b30 = (30 + db),
  b60 = (60 + db),
  b90 = (90 + db);

  // Right Back Pivot
  c90 = (90 + dc),
  c120 = (120 + dc),
  c150 = (150 + dc),
  c180 = (180 + dc);

  // Right Front Pivot
  d0 = (0 + dd),
  d30 = (30 + dd),
  d60 = (60 + dd),
  d90 = (90 + dd);

  // set servo positions and speeds needed to walk forward one step
  // (LFP,  LBP, RBP,  RFP, LFL, LBL, RBL, RFL, S1, S2, S3, S4)
  srv(a180, b0 , c120, d60, 42,  33,  33,  42,  1,  3,  1,  1);
  srv( a90, b30, c90,  d30, 6,   33,  33,  42,  3,  1,  1,  1);
  srv( a90, b30, c90,  d30, 42,  33,  33,  42,  3,  1,  1,  1);
  srv(a120, b60, c180, d0,  42,  33,  6,   42,  1,  1,  3,  1);
  srv(a120, b60, c180, d0,  42,  33,  33,  42,  1,  1,  3,  1);
  srv(a150, b90, c150, d90, 42,  33,  33,  6,   1,  1,  1,  3);
  srv(a150, b90, c150, d90, 42,  33,  33,  42,  1,  1,  1,  3);
  srv(a180, b0,  c120, d60, 42,  6,   33,  42,  1,  3,  1,  1);
  //srv(a180, b0,  c120, d60, 42,  15,  33,  42,  1,  3,  1,  1);
  
}

//== Back ==================================================================================

void back ()
{
  // set servo positions and speeds needed to walk backward one step
  // (LFP,  LBP, RBP,  RFP, LFL, LBL, RBL, RFL, S1, S2, S3, S4)
  srv(180, 0,  120, 60, 42, 33, 33, 42, 3,  1, 1, 1);
  srv(150, 90, 150, 90, 42, 18, 33, 42, 1,  3, 1, 1);
  srv(150, 90, 150, 90, 42, 33, 33, 42, 1,  3, 1, 1);
  srv(120, 60, 180, 0,  42, 33, 33, 6,  1,  1, 1, 3);
  srv(120, 60, 180, 0,  42, 33, 33, 42, 1,  1, 1, 3);
  srv(90,  30, 90,  30, 42, 33, 18, 42, 1,  1, 3, 1);
  srv(90,  30, 90,  30, 42, 33, 33, 42, 1,  1, 3, 1);
  srv(180, 0,  120, 60, 6,  33, 33, 42, 3,  1, 1, 1);

}

//== Left =================================================================================

void turn_left ()
{
  // set servo positions and speeds needed to turn left one step
  // (LFP,  LBP, RBP,  RFP, LFL, LBL, RBL, RFL, S1, S2, S3, S4)
  srv(150,  90, 90,  30, 42, 6,  33, 42, 1, 3, 1, 1);
  srv(150,  90, 90,  30, 42, 33, 33, 42, 1, 3, 1, 1);
  srv(120,  60, 180, 0,  42, 33, 6,  42, 1, 1, 3, 1);
  srv(120,  60, 180, 0,  42, 33, 33, 24, 1, 1, 3, 1);
  srv(90,   30, 150, 90, 42, 33, 33, 6,  1, 1, 1, 3);
  srv(90,   30, 150, 90, 42, 33, 33, 42, 1, 1, 1, 3);
  srv(180,  0,  120, 60, 6,  33, 33, 42, 3, 1, 1, 1);
  srv(180,  0,  120, 60, 42, 33, 33, 33, 3, 1, 1, 1);
}
void turn_left2 (int s)
{
  // set servo positions and speeds needed to turn left one step
  // (LFP,  LBP, RBP,  RFP, LFL, LBL, RBL, RFL, S1, S2, S3, S4)
  srv((90 + 2 * s),  90, 90,  (90 - 2 * s), 42, 6,  33, 42, 1, 3, 1, 1);
  srv((90 + 2 * s),  90, 90,  (90 - 2 * s), 42, 33, 33, 42, 1, 3, 1, 1);
  srv((90 + s),  (90 - s), (90 + 3 * s), (90 - 3 * s),  42, 33, 6,  42, 1, 1, 3, 1);
  srv((90 + s),  (90 - s), (90 + 3 * s), (90 - 3 * s),  42, 33, 33, 24, 1, 1, 3, 1);
  srv(90, (90 - 2 * s), (90 + 2 * s), 90, 42, 33, 33, 6,  1, 1, 1, 3);
  srv(90, (90 - 2 * s), (90 + 2 * s), 90, 42, 33, 33, 42, 1, 1, 1, 3);
  srv((90 + 3 * s),  (90 - 3 * s),  (90 + s), (90 - s), 6,  33, 33, 42, 3, 1, 1, 1);
  srv((90 + 3 * s),  (90 - 3 * s),  (90 + s), (90 - s), 42, 33, 33, 33, 3, 1, 1, 1);
}
//== Right ================================================================================

void turn_right ()
{
  // set servo positions and speeds needed to turn right one step
  // (LFP, LBP,RBP,RFP, LFL,LBL,RBL,RFL, S1, S2, S3, S4)
  srv( 90, 30, 150, 90, 6,  33, 33, 42, 3, 1, 1, 1);
  //delay(1000);
  srv( 90, 30, 150, 90, 42, 33, 33, 42, 3, 1, 1, 1);
  //delay(5000);
  srv(120, 60, 180, 0,  42, 33, 33, 6,  1, 1, 1, 3);
  srv(120, 60, 180, 0,  42, 33, 33, 42, 1, 1, 1, 3);
  srv(150, 90, 90,  30, 42, 33, 6,  42, 1, 1, 3, 1);
  srv(150, 90, 90,  30, 42, 33, 33, 42, 1, 1, 3, 1);
  srv(180, 0,  120, 60, 42, 6,  33, 42, 1, 3, 1, 1);
  srv(180, 0,  120, 60, 42, 33, 33, 42, 1, 3, 1, 1);
}

void turn_right3 (int s)
{
  // set servo positions and speeds needed to turn right one step
  // (LFP, LBP,RBP,RFP, LFL,LBL,RBL,RFL, S1, S2, S3, S4)
  srv( 90, (90 - 2 * s), (90 + 2 * s), 90, 6,  33, 33, 42, 3, 1, 1, 1);
  //delay(1000);
  srv( 90, (90 - 2 * s), (90 + 2 * s), 90, 42, 33, 33, 42, 3, 1, 1, 1);
  //delay(5000);
  srv((90 + s), (90 - s), (90 + 3 * s), (90 - 3 * s),  42, 33, 33, 6,  1, 1, 1, 3);
  srv((90 + s), (90 - s), (90 + 3 * s), (90 - 3 * s),  42, 33, 33, 42, 1, 1, 1, 3);
  srv((90 + 2 * s), 90, 90,  (90 - 2 * s), 42, 33, 6,  42, 1, 1, 3, 1);
  srv((90 + 2 * s), 90, 90,  (90 - 2 * s), 42, 33, 33, 42, 1, 1, 3, 1);
  srv((90 + 3 * s), (90 - 3 * s),  (90 + s), (90 - s), 42, 6,  33, 42, 1, 3, 1, 1);
  srv((90 + 3 * s), (90 - 3 * s),  (90 + s), (90 - s), 42, 33, 33, 42, 1, 3, 1, 1);
}



void my_turn_right3 (int s)
{
  // set servo positions and speeds needed to turn right one step
  // (LFP, LBP,RBP,RFP, LFL,LBL,RBL,RFL, S1, S2, S3, S4)
  srv( 90, 90 - 2 * s, 90 + 2 * s, 90, 6,  33, 33, 42, 3, 1, 1, 1);
  delay(100);
  srv( 90, 90 - 2 * s, 90 + 2 * s, 90, 42, 33, 33, 42, 3, 1, 1, 1);
  delay(100);
  srv(90 + s, 90 - s, 90 + 3 * s, 90 - 3 * s,  42, 33, 33, 6,  1, 1, 1, 3);
  delay(100);
  srv(90 + s, 90 - s, 90 + 3 * s, 90 - 3 * s,  42, 33, 33, 42, 1, 1, 1, 3);
  delay(100);
  srv(90 + 2 * s, 90, 90,  90 - 2 * s, 42, 33, 6,  42, 1, 1, 3, 1);
  delay(100);
  srv(90 + 2 * s, 90, 90,  90 - 2 * s, 42, 33, 33, 42, 1, 1, 3, 1);
  delay(100);
  srv(90 + 3 * s, 90 - 3 * s,  90 + s, 90 - s, 42, 6,  33, 42, 1, 3, 1, 1);
  delay(100);
  srv(90 + 3 * s, 90 - 3 * s,  90 + s, 90 - s, 42, 33, 33, 42, 1, 3, 1, 1);
}
 // rotate_servo(s2, 6, 90, 33, 90, 33, s8, 42);
 // delay(300);
 // rotate_servo(s2, 42, 90, 33, 90, 33, s8, 42);
 // delay(300);
 // rotate_servo(s2, 42, 90, 33, 90, 33, s8, 6);
 // delay(300);  
 // rotate_servo(s2, 42, 90, 33, 90, 33, s8, 42);
 // delay(300);
 // rotate_servo(s2, 42, 90, 33, 90, 6, s8, 42);
 // delay(300);
 // rotate_servo(s2, 42, 90, 33, 90, 33, s8, 42);
 // delay(300);
 // rotate_servo(s2, 42, 90, 6, 90, 33, s8, 42);
 // delay(300);
 // rotate_servo(s2, 42, 90, 33, 90, 33, s8, 42);
 // delay(300);


//== Center Servos ========================================================================
void my_center_servos()
{
  rotate_servo(90, 90, 90, 90, 90, 90, 90, 90);
}
void center_servos()
{
  myServo1.write(90);
  myServo2.write(90);
  myServo3.write(90);
  myServo4.write(90);
  myServo5.write(90);
  myServo6.write(90);
  myServo7.write(90);
  myServo8.write(90);

  int s11 = 90; // Front Left Pivot Servo
  int s12 = 90; // Front Left Lift Servo
  int s21 = 90; // Back Left Pivot Servo
  int s22 = 90; // Back Left Lift Servo
  int s31 = 90; // Back Right Pivot Servo
  int s32 = 90; // Back Right Lift Servo
  int s41 = 90; // Front Right Pivot Servo
  int s42 = 90; // Front Right Lift Servo
}



void srv( int  p11, int p21, int p31, int p41, int p12, int p22, int p32, int p42, int sp1, int sp2, int sp3, int sp4)
{

  // p11: Front Left Pivot Servo
  // p21: Back Left Pivot Servo
  // p31: Back Right Pivot Servo
  // p41: Front Right Pivot Servo
  // p12: Front Left Lift Servo
  // p22: Back Left Lift Servo
  // p32: Back Right Lift Servo
  // p42: Front Right Lift Servo
  // sp1: Speed 1
  // sp2: Speed 2
  // sp3: Speed 3
  // sp4: Speed 4

  // Multiply lift servo positions by manual height adjustment
  p12 = p12 + high * 3;
  p22 = p22 + high * 3;
  p32 = p32 + high * 3;
  p42 = p42 + high * 3;

  while ((s11 != p11) || (s21 != p21) || (s31 != p31) || (s41 != p41) || (s12 != p12) || (s22 != p22) || (s32 != p32) || (s42 != p42))
  {

    // Front Left Pivot Servo
    if (s11 < p11)            // if servo position is less than programmed position
    {
      if ((s11 + sp1) <= p11)
        s11 = s11 + sp1;      // set servo position equal to servo position plus speed constant
      else
        s11 = p11;
    }

    if (s11 > p11)            // if servo position is greater than programmed position
    {
      if ((s11 - sp1) >= p11)
        s11 = s11 - sp1;      // set servo position equal to servo position minus speed constant
      else
        s11 = p11;
    }

    // Back Left Pivot Servo
    if (s21 < p21)
    {
      if ((s21 + sp2) <= p21)
        s21 = s21 + sp2;
      else
        s21 = p21;
    }

    if (s21 > p21)
    {
      if ((s21 - sp2) >= p21)
        s21 = s21 - sp2;
      else
        s21 = p21;
    }

    // Back Right Pivot Servo
    if (s31 < p31)
    {
      if ((s31 + sp3) <= p31)
        s31 = s31 + sp3;
      else
        s31 = p31;
    }

    if (s31 > p31)
    {
      if ((s31 - sp3) >= p31)
        s31 = s31 - sp3;
      else
        s31 = p31;
    }

    // Front Right Pivot Servo
    if (s41 < p41)
    {
      if ((s41 + sp4) <= p41)
        s41 = s41 + sp4;
      else
        s41 = p41;
    }

    if (s41 > p41)
    {
      if ((s41 - sp4) >= p41)
        s41 = s41 - sp4;
      else
        s41 = p41;
    }

    // Front Left Lift Servo
    if (s12 < p12)
    {
      if ((s12 + sp1) <= p12)
        s12 = s12 + sp1;
      else
        s12 = p12;
    }

    if (s12 > p12)
    {
      if ((s12 - sp1) >= p12)
        s12 = s12 - sp1;
      else
        s12 = p12;
    }

    // Back Left Lift Servo
    if (s22 < p22)
    {
      if ((s22 + sp2) <= p22)
        s22 = s22 + sp2;
      else
        s22 = p22;
    }

    if (s22 > p22)
    {
      if ((s22 - sp2) >= p22)
        s22 = s22 - sp2;
      else
        s22 = p22;
    }

    // Back Right Lift Servo
    if (s32 < p32)
    {
      if ((s32 + sp3) <= p32)
        s32 = s32 + sp3;
      else
        s32 = p32;
    }

    if (s32 > p32)
    {
      if ((s32 - sp3) >= p32)
        s32 = s32 - sp3;
      else
        s32 = p32;
    }

    // Front Right Lift Servo
    if (s42 < p42)
    {
      if ((s42 + sp4) <= p42)
        s42 = s42 + sp4;
      else
        s42 = p42;
    }

    if (s42 > p42)
    {
      if ((s42 - sp4) >= p42)
        s42 = s42 - sp4;
      else
        s42 = p42;
    }

    // Write Pivot Servo Values
    myServo1.write(s11 + da);
    myServo3.write(s21 + db);
    myServo5.write(s31 + dc);
    myServo7.write(s41 + dd);

    // Write Lift Servos Values
    myServo2.write(s12);
    myServo4.write(s22);
    myServo6.write(s32);
    myServo8.write(s42);

    delay(spd); // Delay before next movement

  }//while
} //srv

void rotate_servo(int s2, int s3, int s4, int s5, int s6, int s7, int s8, int s9){
  myServo1.write(s2);
  myServo2.write(s3);
  myServo3.write(s4);
  myServo4.write(s5);
  myServo5.write(s6);
  myServo6.write(s7);
  myServo7.write(s8);
  myServo8.write(s9);
  delay(100);

}

//== USRF Function ========================================================================

long get_distance(bool unit)
{
  // if unit == 0 return inches, else return cm

  long duration = 0, 
       cm = 0, 
       inches = 0;

  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  // convert the time into a distance
  cm = (duration / 2) / 29.1;
  inches = (duration / 2) / 74;

  if (unit == INCH)
    return inches;
  else
    return cm;
}
