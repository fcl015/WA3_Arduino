#include <stdlib.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(12,13); // RX, TX

int s2=6;

unsigned long tiempo=10000;
unsigned long tiempoanterior=0;

float fixed_alpha=1.86;
float alpha_data=0.0;
float red_data=0.0;
float nir_data=0.0;
float nvdi_data=0.0;

char sensor_type=0;
char sensor_orientation=0;

/*-------------Metodo para leer NIR and RED reflectance ------------*/
void readNVDI(){

    char dato1[7], dato2[7];
    String message;
    int i;

    //Energizar sensor
    digitalWrite(s2, HIGH);
    delay(500);
    
    //Esperar y leer datos provenientes del sensor
    //if (Serial1.available()) {   
    if (mySerial.available()) {   

        //message=Serial1.readString();
        message=mySerial.readString();

        Serial.print("RAW MESSAGE  :"); 
        Serial.print(message);
        Serial.print("MESSAGE SIZE :");
        Serial.println(message.length(),DEC);
        
        Serial.print("MESSAGE HEX  :");
        for(int i=0; i<=message.length(); i++){
          Serial.print(message[i],HEX);      
          Serial.print(' '); 
        }
        Serial.println(' '); 

        dato1[0] = message[2];
        dato1[1] = message[3];
        dato1[2] = message[4];
        dato1[3] = message[5];
        dato1[4] = message[6];
        dato1[5] = message[7];
        dato1[6] = 0;
 
        dato2[0] = message[9];
        dato2[1] = message[10];
        dato2[2] = message[11];
        dato2[3] = message[12];
        dato2[4] = message[13];
        dato2[5] = message[14];
        dato2[6] = 0;

        sensor_orientation=message[16];
        sensor_type=message[18];

        //Convertir arreglo "a" a variable flotante
        red_data = atof(dato1); 
        //Convertir arreglo "b" a flotante 
        nir_data = atof(dato2);
        Serial.println("RESULTS:"); 
        Serial.print("RED:  ");
        Serial.println(red_data,4);
        Serial.print("NIR:  ");
        Serial.println(nir_data,4);
                
        //Calcular NVDI si el sensor tiene orientacion hacia abajo '1' , tipo de sensor de campo 'r'
        if( sensor_type=='r' and sensor_orientation=='1' ){
              //nvdi_data = (fixed_alpha*nir_data - red_data) / (fixed_alpha*nir_data + red_data);
              nvdi_data = (alpha_data*nir_data - red_data) / (alpha_data*nir_data + red_data);
              Serial.print("ALPHA: ");
              Serial.println(alpha_data,4);
              Serial.print("NDVI: ");
              Serial.println(nvdi_data,4);
        }
        //Calcular factor alfa si el sensor tiene orientacion hacia arriba '2', tipo de sensor de campo 'r'
        else if( sensor_type=='r' and sensor_orientation=='2' ){
              alpha_data = red_data / nir_data;
              nvdi_data = (alpha_data*nir_data - red_data) / (alpha_data*nir_data + red_data);
              Serial.print("ALPHA: ");
              Serial.println(alpha_data,4);
              Serial.print("NDVI: ");
              Serial.println(nvdi_data,4);
        }
        else{
              Serial.println("INVALID MEASUREMENT !!!");
        }
    }
    
    //Desenergizar sensor
    delay(50);
    digitalWrite(s2, LOW);

}



void setup() {
   pinMode(s2, OUTPUT);
   Serial.begin(9600);//Puerto para comunicacion por xbee
   //Serial1.begin(1200);//Puerto de comunicacion con sensor digital
   mySerial.begin(1200);//Puerto de comunicacion con sensor digital
   Serial.println("STARTING..."); 
}
 
void loop() {
  
    unsigned long tiempoactual=millis();

    if(tiempoactual - tiempoanterior > tiempo) {
          tiempoanterior = tiempoactual;
          Serial.println("----------------------------"); 
          Serial.println(millis()/1000.0); 
          Serial.println("----------------------------"); 
          readNVDI();
    }
}

