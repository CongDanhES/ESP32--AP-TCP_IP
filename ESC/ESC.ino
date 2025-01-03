#define ESC_Pin 18 

int dutyCycle;
const int pwmChannel = 0;        
const int pwmFrequency = 50;     
const int pwmResolution = 16;    

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Attach the pin to the PWM channel
  ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
  ledcAttachPin(ESC_Pin, pwmChannel);

  // 2000us / 20000us
  dutyCycle = (2000 * (1 << pwmResolution)) / 20000;
  Serial.println(dutyCycle);
  ledcWrite(pwmChannel, dutyCycle);
  delay(5000);  

  // 1000us / 20000us
  dutyCycle = (1000 * (1 << pwmResolution)) / 20000;
  ledcWrite(pwmChannel, dutyCycle);
  delay(5000);  
}

void loop() {
  // put your main code here, to run repeatedly:
  // 1100us / 20000us
  dutyCycle = (1060 * (1 << pwmResolution)) / 20000;
  Serial.println(dutyCycle);
  ledcWrite(pwmChannel, dutyCycle);
  delay(5000);

  dutyCycle = (1070 * (1 << pwmResolution)) / 20000;
  Serial.println(dutyCycle);
  ledcWrite(pwmChannel, dutyCycle);
  delay(5000);
}