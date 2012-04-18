/*
  Glados Arduino code:
  CMPS03: Compass
  IMU (Gyro & accelerometer)
  Bumper

  Credits:
  Asa Hammond
  Aamoy Gupta - aamoyg@linux.ucla.edu
  Ed Solis - edsolis1@engineering.ucla.edu

  CMPS03 (Compass):
  This will display a value of 0 - 359 for a full rotation of the
  compass.

  The SDA line is on analog pin 4 of the arduino and is connected to
  pin 3 of the CMPS03.
  The SCL line is on analog pin 5 of the arduino and is conected to
  pin 2 of the CMPS03.
  Both SDA and SCL are also connected to the +5v via a couple of 1k8
  resistors.
  A switch to callibrate the CMPS03 can be connected between pin 6
  of the CMPS03 and the ground.
  
  Bumper: Array of buttons is connected to pin 7 and gnd; this
          program indicates if bumper button(s) is/are pushed
          and publishes bumper state to appropriate rostopic
*/

#include <Wire.h>
#include <ros.h>
//#include <std_msgs/Float32.h>
//#include <rosserial_arduino/Adc.h>
#include <glados_sensors/imu.h>
#include <std_msgs/Bool.h>

ros::NodeHandle nh;

// the following is required to work with the smaller atmel 168
// not needed with other chips with more memory
//ros::NodeHandle_<ArduinoHardware,2, 2, 80, 105> nh;

//std_msgs::Float32 bearing_msg;
//ros::Publisher pub_bearing("bearing", &bearing_msg);

//rosserial_arduino::Adc adc_msg;
glados_sensors::imu imu_msg;

//ros::Publisher p("adc", &adc_msg);
ros::Publisher p("imu", &imu_msg);

#define ADDRESS 0x60 //defines address of compass

std_msgs::Bool bumper_msg;
ros::Publisher pub_bumper("bumped", &bumper_msg);

const int bumper_pin = 7;
const int led_pin = 13;

bool last_reading;
long last_debounce_time=0;
long debounce_delay=50;
bool bumper_published = true;

void setup(){

  Wire.begin(); //conects I2C
     // Serial.begin(9600);
  nh.initNode();
//  nh.advertise(pub_bearing);
  nh.advertise(p);
  nh.advertise(pub_bumper);

  pinMode(13, OUTPUT);
  pinMode(led_pin, OUTPUT);
  pinMode(bumper_pin, INPUT);

  //Enable the pullup resistor on the button
  PORTD |= (1<<PD7);

  //The button is a normally button
  last_reading = ! digitalRead(bumper_pin);
}

//We average the analog reading to elminate some of the noise
int averageAnalog(int pin){
  int v=0;
  for(int i=0; i<5; i++) v+= analogRead(pin);
  return v/5;
}


void loop(){
  byte highByte;
  byte lowByte;
  Wire.beginTransmission(ADDRESS);//starts communication with cmps03
  Wire.write(2);                  //Sends the register we wish to read
  Wire.endTransmission();
  
  Wire.requestFrom(ADDRESS, 2);   //requests high byte
  while(Wire.available() < 2);    //while there is a byte to receive
  highByte = Wire.read();         //reads the byte as an integer
  lowByte = Wire.read();


  imu_msg.bearing = ((highByte<<8)+lowByte)/10.0;

  // now work on the imu
  // check out code.google.com drh-balancing-robot IDG300.h
  const int c_ZeroX = 502;
  const int c_ZeroY = 569;

  //scaling from analog in values to degree/sec
  // sensitivity: 2 mv / degree/sec
  // 10bits (1024 values) for 3.3 volts: 3.22265625 mv per step
  //  or: 1.611328125 degree/sec for each step
  const float c_Scaling = 1.611328125;
  double xRaw, yRaw;
  xRaw = averageAnalog(1);
  yRaw = averageAnalog(2);

//  const double contribution = 0.1;
//  RawXValue = Smooth(RawXValue, xRaw, contribution);
//  RawYValue = Smooth(RawYValue, yRaw, contribution);



//  imu_msg.gx = xRaw;
//  imu_msg.gy = yRaw;


  imu_msg.gx = (xRaw - c_ZeroX ) * c_Scaling ; // degrees / sec
  imu_msg.gy = (yRaw - c_ZeroY ) * c_Scaling ; // degrees / sec

  p.publish(&imu_msg);

  // Bumper code:
  bool reading = ! digitalRead(bumper_pin);
  
  if (last_reading!= reading){
      last_debounce_time = millis();
      bumper_published = false;
  }
  
  // if the button value has not changed for the debounce delay,
  // we know it's stable
  if ( !bumper_published && (millis() - last_debounce_time)
  	> debounce_delay)
  {
    digitalWrite(led_pin, reading);
    bumper_msg.data = reading;
    pub_bumper.publish(&bumper_msg);
    bumper_published = true;
  }

  last_reading = reading;

  nh.spinOnce();
  delay(100);

}
