#include <Wire.h>                                           
#include <iarduino_RTC.h>                                  
                        
iarduino_RTC watch(RTC_DS3231);                        
                                                            
void setup(){                                               
     delay(300);                                            
     Serial.begin(9600);                                    
     watch.begin(&Wire); 
     watch.settime(0,17,15,21,4,26,2);                     
}                                                           
void loop(){                                                
     if( millis()%1000==0 ){                               
         Serial.println(watch.gettime("d-m-Y, H:i:s, D"));  
         delay(1);                                          
     }                                                      
}                                                           