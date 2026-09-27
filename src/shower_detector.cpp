// Detecting shower
/*
//analogReadResolution(12);
bool isShower() {
  unsigned int samples = 0;
unsigned long readValue = 0; 
  int showering = 0;
  long value = 0;
  readValue += analogRead(GPIO_NUM_3);
  
  samples++;
  if (samples > 1000) {
    int avr = readValue/samples;
    samples = 0;
    readValue = 0;
    if (avr > 10) {
      showering = 1;
    } else {
      showering = 0;
    }
    return showering == 1 ? true : false;
  }
  return false;
}
*/
