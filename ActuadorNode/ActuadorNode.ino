 
 /*===================================================================================================================================*/
/*=====                                          Sensor node firmware v3.0                                                      =====*/
/*=====                                                   ARDUINO Mega                                                           =====*/
/*=====                                              Water Automation                                                           =====*/
/*=====                                                                                                                         =====*/
/*=====                                                                                                                         =====*/
/*=====                                                                                                                         =====*/
/*===================================================================================================================================*/

// Define output locations
#define VALVE1_ON   digitalWrite(7, HIGH)
#define VALVE1_OFF  digitalWrite(7, LOW)
#define VALVE2_ON   digitalWrite(6, HIGH)
#define VALVE2_OFF  digitalWrite(6, LOW)
#define VALVE3_ON   digitalWrite(5, HIGH)
#define VALVE3_OFF  digitalWrite(5, LOW)
#define LED1_ON     digitalWrite(9, HIGH)
#define LED1_OFF    digitalWrite(9, LOW)
#define LED2_ON     digitalWrite(10, HIGH)
#define LED2_OFF    digitalWrite(10, LOW)
#define LED3_ON     digitalWrite(11, HIGH)
#define LED3_OFF    digitalWrite(11, LOW)
  
// declare global variables
 char bufferin[80];
 char buffout[80];
 char serial_cmd[10];
 char valve1_status;
 char valve2_status;
 char valve3_status;

 unsigned char battery_level = 0xFF;
 unsigned char pressure_units = 0x42;
 int sum;
 unsigned char pressure_decimals = 0x42;
 float discharge2 =0;
 float discharge =0;


 unsigned long tiempoanterior;
 unsigned long tiempo=60000; //time for discharge calc
 unsigned long flow_counter = 0;
 int i = 0;
 
char input_message_size=22;
char output_message_size=32;

 
 void setup(){
 
   pinMode(9, OUTPUT); //LED 1
   pinMode(10, OUTPUT);//LED 2
   pinMode(11, OUTPUT);//LED 3
   pinMode(6, OUTPUT); //VALVE 2
   pinMode(7, OUTPUT); //VALVE 1
   pinMode(5, OUTPUT); //VALVE 3
   attachInterrupt(0, flow, RISING);
   Serial.begin(9600);

   for(int j=0; j<255 ; j++) {analogWrite(9,j); delay(2);}
   for(int j=255; j>=0 ; j--) {analogWrite(9,j); delay(2);}
   for(int j=0; j<255 ; j++) {analogWrite(10,j); delay(2);}
   for(int j=255; j>=0 ; j--) {analogWrite(10,j); delay(2);}
   for(int j=0; j<255 ; j++) {analogWrite(11,j); delay(2);}
   for(int j=255; j>=0 ; j--) {analogWrite(11,j); delay(2);}
   
  digitalWrite(7, HIGH);
  digitalWrite(9, HIGH);
  delay(300);
  digitalWrite(7, LOW);
  digitalWrite(9, LOW);
  delay(300);
  digitalWrite(6, HIGH);
  digitalWrite(10, HIGH);
  delay(300);
  digitalWrite(6, LOW);
  digitalWrite(10, LOW);
  delay(300);
  digitalWrite(5, HIGH);
  digitalWrite(11, HIGH);
  delay(300);
  digitalWrite(5, LOW);
  digitalWrite(11, LOW);
  delay(300);
 }
 
void loop(){
  
 if(valve1_status == 1) LED1_ON;
 else LED1_OFF;
 
 if(valve2_status == 1) LED2_ON;
 else LED2_OFF;
 
 if(valve3_status == 1) LED3_ON;
 else LED3_OFF;

 //if(valve1_status == 1 || valve2_status == 1) {pump_status = 1; PUMP_ON; LED3_ON;}
 //else if (valve1_status == 0 && valve2_status == 0) {pump_status = 0; PUMP_OFF; LED3_OFF;}
 
 unsigned long tiempoactual=millis();
  
 if(tiempoactual - tiempoanterior > tiempo) {         
          discharge_calc(flow_counter);  
          flow_counter = 0;
          tiempoanterior = tiempoactual; 
 }
}
 
void serialEvent(){
  char serial_cmd[11];
  i = 0;
  if(Serial.available()){  
      Serial.readBytes(bufferin,input_message_size);
      if(bufferin[0] == 0x7E && bufferin[15]== 'A'){
          if(bufferin[16] == '1') {valve1_status = 1; VALVE1_ON; }
          else if(bufferin[16] == '0'){valve1_status = 0; VALVE1_OFF;}
          if(bufferin[17] == '1') {valve2_status = 1; VALVE2_ON; }
          else if(bufferin[17] == '0'){valve2_status = 0; VALVE2_OFF;}
          if(bufferin[18] == '1') {valve3_status = 1; VALVE3_ON; }
          else if(bufferin[18] == '0'){valve3_status = 0; VALVE3_OFF;}
           
          buffout[0]=0x7E;//API mode start delimeter
          buffout[1]=0x00;//Data length MSB
          buffout[2]=output_message_size-4;//Data length LSB
          buffout[3]=0x10;//API ID TX Request 16 bit address
          buffout[4]=0x00;//Frame ID, if equals to 0x00 disable response frame
          buffout[5]=bufferin[4];
          buffout[6]=bufferin[5];
          buffout[7]=bufferin[6];
          buffout[8]=bufferin[7];
          buffout[9]=bufferin[8];
          buffout[10]=bufferin[9];
          buffout[11]=bufferin[10];
          buffout[12]=bufferin[11];//Destination Address LSB
          buffout[13]=0xFF;
          buffout[14]=0xFE;
          buffout[15]=0x00;
          buffout[16]=0x00;
          buffout[17]='A'; 
          buffout[18]=valve1_status;
          buffout[19]=valve2_status;
          buffout[20]=valve3_status;
          buffout[21]=(char)(unsigned int)discharge;//Water Flow liters/minute INTEGERS
          buffout[22]=(char)((unsigned int)(discharge*100)%100);//Water Flow liters/minute DECIMALS
          buffout[23]=0x00;
          buffout[24]=0x00;
          buffout[25]=0x00;
          buffout[26]=0x00;
          buffout[27]=0x00;
          buffout[28]=0x00;
          buffout[29]=0x00;
          buffout[30]=0x0A;//End of package
          for(int i=3;i<output_message_size-1;i++){
               sum+=buffout[i]; //Checksum
          }
          buffout[31]=0xFF-lowByte(sum);
          sum=0;
          Serial.write(buffout,output_message_size);      
      }
  }
}
 
void flow(){
   digitalWrite(11, HIGH);
   delay(20);
   flow_counter++;
   digitalWrite(11, LOW);
}
 

void discharge_calc(unsigned long pulses){
  
  #define offset 0.39
  #define K 0.44
  #define fact_litres 3.78541
 
  if (pulses > 10){
      // Liters = 0.0407*pulses +2.7478 
      discharge = (0.0407*pulses) + 2.7478;// Linear approximation formula
      //Datasheet formula
      discharge2 = (((float)pulses/60) + offset)*K;  // in gallons per minute
      discharge2 = discharge2*fact_litres;
      // 530 pulses = 20 lts = 5.28344 gallons
      //1003 pulses = 10 gallons
  }
  else{
      discharge = 0;  
      discharge2 = 0;
  }  
}


