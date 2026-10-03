# STM32_PWM-Generator
Simple example code to generate a PWM signal, with frequency 1hz to 82Mhz and 1-16bit resolution. Only with arduino library.

Test board: STM32F401CC (Blackpill)

# The function 
```
void pwm_start(PinName pin, uint32_t PWM_freq, uint32_t value, TimerCompareFormat_t resolution);
```
That's is a function used inside AnalogWrite(), where the inputs are maped to fit in pwm_start().

By using directly that function you'll be able to generate a pwm signal with custom frequency and resolution, instead of the default 1khz and 8bit resolution.

# Function Inputs
- _PinName pin_: The output pin, in PinName type.
  - PinName type variable
  - digitalPinToPinName(Pin_Number) //this func returns a PinName type. Where the Pin_Number is the same input in pinMode(), for example

-  _uint32_t clock_freq_ : The PWM clock frequency.
   - 1hz to 82Mhz //Notice that it **changes the max effective resolution** too

-  _uint32_t value_ : The duty cycle value.  
   - 0 to 2^(resolution bits) //changes the amount of time the waveform stays in high

- _TimerCompareFormat_t resolution_ : The input resolution of the Duty Cycle, if it is highter than the real, it's just maped internaly to fit the real value. (**The max is affected by frequency**)
  - RESOLUTION_1B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 1] (0 or 50%)
  - RESOLUTION_2B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 3]
  - RESOLUTION_3B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 7]
  - RESOLUTION_4B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 15]
  - RESOLUTION_5B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 31]
  - RESOLUTION_6B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 63]
  - RESOLUTION_7B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 127]
  - RESOLUTION_8B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 255]
  - RESOLUTION_9B_COMPARE_FORMAT      // used for Dutycycle: [0 .. 511]
  - RESOLUTION_10B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 1023]
  - RESOLUTION_11B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 2047]
  - RESOLUTION_12B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 4095]
  - RESOLUTION_13B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 8191]
  - RESOLUTION_14B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 16383]
  - RESOLUTION_15B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 32767]
  - RESOLUTION_16B_COMPARE_FORMAT     // used for Dutycycle: [0 .. 65535]

# Frequency x Resolution
  If your input resolution is highter than the real resolution, it's just maped internaly to the fit in the real value
  
  The real max resolution is affected by the frequency. **A higher frequency, means a lower resolution.**
    
  If you're using a high frequency/resolution. I strongly recommend you to test your real resolution, by slowly changing the duty and watching the change in output signal. Beacause the **seted resolution may not be the real one**.

# Upload method
  That isn't the main point of the repo, but it's important to know that the platformio.ini file contains all the settings I needed to send the code to my blackpill via usb. If you're using another board, you'll need to change the settings.
