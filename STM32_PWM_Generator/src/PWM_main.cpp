// PWM Generator Example with STM32 
// Author: Tales-sv

#include <Arduino.h>

#define pwm_out PB10   //pwm_out pin

u_int32_t duty= 0; //Duty cycle

u_int32_t frequency= 80000; //PWM frequency

void setup() {

  pinMode(pwm_out,OUTPUT);

}

void loop() {
  //default arduino function. Have a limited frequency and resolution (~1kHz and 8bits)
  // analogWrite(pwm_out, 125);

  //that function (pwm_start) already exists in arduino library and it's caled inside AnalogWrite(). the arduino func inputs are maped to fit in pwm_star
  pwm_start(digitalPinToPinName(pwm_out), frequency, duty, RESOLUTION_10B_COMPARE_FORMAT);
  /*/ Inputs:

  PinName pin : The output pin, in PinName type. 
    If you don have the pin in the correct type, A simple conversion made by digitalPinToPinName(int) (as shown) can give you the correct value.

  uint32_t clock_freq : The PWM clock frequency.
    Can be 1Hz to MClk Hz (82MHz in black pill), it changes the max resolution too.

  uint32_t value : The duty cycle value.
    Can be 0 to 2^(resolution bits), changes the amount of time the waveform stays in high

  TimerCompareFormat_t resolution: The resolution of the Duty Cycle
    This is just your input resolution, if it is highter than the real, it's just maped internaly to the real value
    The real max resolution is affected by the frequency. A higher frequency, means a lower resolution.
    Can be: 
    RESOLUTION_1B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 1] (0 or 50%)
    RESOLUTION_2B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 3]
    RESOLUTION_3B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 7]
    RESOLUTION_4B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 15]
    RESOLUTION_5B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 31]
    RESOLUTION_6B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 63]
    RESOLUTION_7B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 127]
    RESOLUTION_8B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 255]
    RESOLUTION_9B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 511]
    RESOLUTION_10B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 1023]
    RESOLUTION_11B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 2047]
    RESOLUTION_12B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 4095]
    RESOLUTION_13B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 8191]
    RESOLUTION_14B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 16383]
    RESOLUTION_15B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 32767]
    RESOLUTION_16B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 65535]
    
   PS:
    If you're using a high frequency/resolution. I strongly recommend you to test your real resolution, by slowly changing the duty and watching the change in output signal. Beacause the configured resolution may not be the real one.
  /*/
  
  duty +=10; //duty change test
  if (duty>(1023)) duty=0; //reset the duty


  //you can change the frequency inside the code too:
  // frequency += 1000;
  // if(frequency>1000000) frequency=100;

  delay(10);
}