
/*===================================================================================================================================*/
/*=====                                          Sensor node firmware v3.0                                                      =====*/
/*=====                                                   ARDUINO UNO                                                           =====*/
/*=====                                              Water Automation                                                           =====*/
/*=====                                                                                                                         =====*/
/*=====     To power sensors use digital pins 5, 6 and 7                                                                        =====*/
/*=====     To read sensors use analog pins A0, A1, A2                                                                          =====*/
/*=====                                                                                                                         =====*/
/*=====                                                                                                                         =====*/
/*===================================================================================================================================*/


/*=====Sensor variables=====*/
int S1_ON = 7, S2_ON = 6, S3_ON = 5;
int S1_PORT = A0, S2_PORT = A1, S3_PORT = A2;
int S1_RAW, S2_RAW, S3_RAW;
float S1_MOIST, S2_MOIST, S3_MOIST;
int S1_MOIST_INT, S1_MOIST_DEC, S2_MOIST_INT, S2_MOIST_DEC, S3_MOIST_INT, S3_MOIST_DEC;
char soil_moisture_equation='0';
/*==========================*/

/*=====Communication variables=====*/
char input_message_size=22;
char output_message_size=32;
char Buffout[80], Buffin[80];
char XBeeAdd[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
/*=================================*/

/*=====Timer variables=====*/
unsigned long previousMillis;
unsigned long timeInterval = 1000;
/*=========================*/

/*=====Misc variables=====*/
char ERRORCODE = 0x00;
char BATLVL = 100;
char NODE_TYPE = 'S';
/*========================*/

int sum1;

void setup(){
  Serial.begin(9600);
  pinMode(S1_ON, OUTPUT);
  pinMode(S2_ON, OUTPUT);
  pinMode(S3_ON, OUTPUT);
  analogReference(INTERNAL);
  //analogReference(INTERNAL1V1);
}


void serialEvent(){
  while(!Serial.available());   
      Serial.readBytes(Buffin,input_message_size);
      if(Buffin[0] == 0x7E && Buffin[15] == 'S'){ 
            soil_moisture_equation=Buffin[18];     
          /*=======Emisor MAC address=======*/
            XBeeAdd[0] = Buffin[4];
            XBeeAdd[1] = Buffin[5];
            XBeeAdd[2] = Buffin[6];
            XBeeAdd[3] = Buffin[7];
            XBeeAdd[4] = Buffin[8];
            XBeeAdd[5] = Buffin[9];
            XBeeAdd[6] = Buffin[10];
            XBeeAdd[7] = Buffin[11];
            /*=================================*/
            /*=======API Frame=======*/
            Buffout[0]=0x7E;                 //API mode start delimeter
            Buffout[1]=0x00;                 //Data length MSB
            Buffout[2]=output_message_size-4;//Data length LSB
            Buffout[3]=0x10;            //API ID TX Request 64 bit address
            Buffout[4]=0x00;            //Frame ID, if equals to 0x00 disable response frame
            Buffout[5]=XBeeAdd[0];      //Destination Address MSB
            Buffout[6]=XBeeAdd[1];            
            Buffout[7]=XBeeAdd[2];
            Buffout[8]=XBeeAdd[3];
            Buffout[9]=XBeeAdd[4];
            Buffout[10]=XBeeAdd[5];
            Buffout[11]=XBeeAdd[6];
            Buffout[12]=XBeeAdd[7];     //Destination Address LSB                
        		Buffout[13]=0xFF;
        		Buffout[14]=0xFE;
        		Buffout[15]=0x00;
        		Buffout[16]=0x00;
            Buffout[17]=NODE_TYPE;           // Node 'S'
            Buffout[18]=S1_MOIST_INT;   //Moisture 1 integer
		        Buffout[19]=S1_MOIST_DEC;   //Moisture 1 decimals
            Buffout[20]=S2_MOIST_INT;   //Moisture 2 integer
            Buffout[21]=S2_MOIST_DEC;   //Moisture 2 decimals
            Buffout[22]=S3_MOIST_INT;   //Moisture 3 integer
            Buffout[23]=S3_MOIST_DEC;   //Moisture 3 decimals
            Buffout[24]=0x00;           //Not used
            Buffout[25]=0x00;           //Not used
            Buffout[26]=0x00;           //Not used
            Buffout[27]=0x00;           //Not used
            Buffout[28]=0x00;           //Not used
            Buffout[29]=0x00;           //Not used
            Buffout[30]=0x0A;           //End of packet
            for(int i=3;i<output_message_size-1;i++){      
                  sum1+=Buffout[i]; //Checksum
            }
            Buffout[31]=0xFF-lowByte(sum1);
            sum1=0;
            Serial.write(Buffout,output_message_size);
      }
}


void loop(){ 
   unsigned long currentMillis = millis();
 
    if(currentMillis - previousMillis > timeInterval) {
        previousMillis = currentMillis;   
        S1_RAW = SensorRead(S1_ON, S1_PORT, 100);
        S2_RAW = SensorRead(S2_ON, S2_PORT, 100);
        S3_RAW = SensorRead(S3_ON, S3_PORT, 100);
    
        S1_MOIST = MoistureCalculation(S1_RAW, soil_moisture_equation, 10, 1.1);
        S2_MOIST = MoistureCalculation(S2_RAW, soil_moisture_equation, 10, 1.1);
        S3_MOIST = MoistureCalculation(S3_RAW, soil_moisture_equation, 10, 1.1);
    
        S1_MOIST_INT = int(S1_MOIST);
        S1_MOIST_DEC = (float(S1_MOIST - S1_MOIST_INT))*100;
        S2_MOIST_INT = int(S2_MOIST);
        S2_MOIST_DEC = (float(S2_MOIST - S2_MOIST_INT))*100;
        S3_MOIST_INT = int(S3_MOIST);
        S3_MOIST_DEC = (float(S3_MOIST - S3_MOIST_INT))*100;
    }
}

int SensorRead(int port_ON, int port_adc, int stab_time){
  int rawData;

  digitalWrite(port_ON, HIGH);
  delay(stab_time);
  rawData = analogRead(port_adc);
  delay(20);
  digitalWrite(port_ON, LOW);
  delay(20);
  
  return rawData;
}


float MoistureCalculation(int SensorRaw, char eq_num, int adc_resolution, float voltage_reference){
    float moisture;
    float milivolts;
    float milivolts_resolution;
    
    milivolts_resolution = float((voltage_reference/(pow(2,adc_resolution) - 1))*1000);
    milivolts = float(SensorRaw * milivolts_resolution);
       
    switch(eq_num){
        case '0':     /* Factory Calibration */
        moisture = ((0.00119*milivolts) - 0.4000)*100;  
        break;
        
        case '1':     /* Area TEC EC5 */
        moisture = ((0.0009*milivolts) - 0.3487)*100;  
        break;
        
        case '2':     /* Area TEC 10HS */
        moisture = ((0.0008*milivolts) - 0.4138)*100;
        break;
        
        case '3':     /* Area Delicias EC5 */
        moisture = ((0.0009*milivolts) - 0.3926)*100;  
        break;
        
        case '4':     /* Area Delicias 10HS */
        moisture = ((0.0008*milivolts) - 0.4897)*100;
        break;
        
        case '5':     /* Area GranjaHogar EC5 */
        moisture = ((0.0009*milivolts) - 0.3487)*100;  
        break;
        
        case '6':     /* Area GranjaHogar 10HS */
        moisture = ((0.0008*milivolts) - 0.3681)*100;
        break;

        default:     /* Factory Calibration */
        moisture = ((0.00119*milivolts) - 0.4000)*100;  
        break;
    }
    if (moisture < 0.0) {
        moisture=0.0;      
        }
        
    return moisture;
}




