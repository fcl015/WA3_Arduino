#include<stdlib.h>
#include <Wire.h>

/*----------Variables de configuracion-------------*/
int adc_1_1 = 1;   // 0-ADC 5 volts, 1-ADC 1.1 volts
long tiempo=10000; //Tiempo de muestreo


/*----------Variables generales-------------*/
float humedad, temp; 
char raw_temp_hum[16];
int s1=7, s2=6, s3=5;
unsigned long tiempoanterior=0;

/*--------Variables VP3 (Sensor de humedad)-------------*/
unsigned int humedad_ent;
unsigned int humedad_dec;
unsigned int temp_ent;
unsigned int temp_dec;
float humedad_ant = 0;
float temp_ant = 0;


 
 
/*-------------Metodo para leer datos de sensor de humedad y temperatura------------*/
void readVP3(){

    float valor;
    int x = 0;
    char a[5], b[5];
    char datos[16];
    String mensaje;

    //Energizar sensor
    digitalWrite(s2, HIGH);
    delay(30);
    
    //Esperar y leer datos provenientes del sensor
    if (Serial1.available()) {   

        mensaje=Serial1.readString();
        Serial.println("---------------");
        Serial.print("Message Size: ");
        Serial.println(mensaje.length(),DEC);
        //Serial1.readBytes(datos,17);
        Serial.println("----------------");
        
        //Enviar datos leidos a variable gobal para monitoreo
        for(int i=0; i<17; i++){
          raw_temp_hum[i]=mensaje[i]; 
        }

       b[0] = raw_temp_hum[2];
       b[1] = raw_temp_hum[3];
       b[2] = raw_temp_hum[4];
       b[3] = raw_temp_hum[5];
       b[4] = 0;
          Serial.print("H (%): ");
          Serial.println(b);
 
       a[0] = raw_temp_hum[7];
       a[1] = raw_temp_hum[8];
       a[2] = raw_temp_hum[9];
       a[3] = raw_temp_hum[10];
       a[4] = 0;
          Serial.print("T (oC): ");
          Serial.println(a);

        //Convertir arreglo "a" a variable flotante
        temp = atof(a); 
        //Convertir arreglo "b" a flotante y obtener humedad relativa
        humedad = (atof(b)/(0.611*pow(2.7182,((17.502*temp)/(240.97+temp))))*100);



    }
    //Desenergizar sensor
    digitalWrite(s2, LOW);

}



void setup() {

   pinMode(s2, OUTPUT);
   Serial.begin(9600);//Puerto para comunicacion por xbee
   Serial1.begin(1200);//Puerto de comunicacion con sensor digital
   if(adc_1_1==1){
      analogReference(INTERNAL1V1);  //Utilizar para ADC de 1.1 volts
   }
}
 
void loop() {
  
    unsigned long tiempoactual=millis();

    if(tiempoactual - tiempoanterior > tiempo) {
          tiempoanterior = tiempoactual;  
          readVP3();

          // Monitoreo de datos
          Serial.println("-------------");
          Serial.println("Sample");
          Serial.println("-------------");
          Serial.print("Temp Humedad (Raw): ");
          for(int i=0; i<17; i++){
            if( raw_temp_hum[i]>=' ' ){
              Serial.print(raw_temp_hum[i]);
            }
            else{
             Serial.print("-"); 
            }
          }
        Serial.println(" "); 
        Serial.print("Humedad (%): ");
        Serial.println(humedad,2);
        Serial.print("Temperatura (oC): ");
        Serial.println(temp,2);
    }
}

