// Evapotranspiration method: Penman-Monteith FAO 1998
// EQUATION:
//                                             900  
//          0.408 delta ( Rn - G ) + gamma ---------- u2 ( es - ea)
//                                            T + 273
//   ETo = ------------------------------------------------------------
//                     delta + gamma ( 1 + 0.34 u2 ) 
//   where
//       delta - pendiente de la curva de la tension de vapor saturado (kPa/oC)
//       Rn - radiacion neta sobre la superficie de cultivo (MJ/(m^2 day))
//       G - flujo calorifico utilizado en el calentamiento del suelo (MJ/(m^2 day))
//       gamma - constante psicometrica (kPa/oC)
//       u2 - velocidad del viento a 2 metros de altura (m/s)
//       es-ea - deficit de tension de vapor (kPa)
//  
//
// En periodo horario: Se entra con los datos medios horarios y el resultado de
// ETo se cambia 0,34, por 0,24 en el día y por 0,96,
// en la noche.
//

#include<stdlib.h>

/*----------Variables de configuracion-------------*/
int adc_1_1 = 1;   // 0-ADC 5 volts, 1-ADC 1.1 volts
long tiempo=60000; //Tiempo de muestreo
int standalone=0; // 1-standalone, 0-Zigbee comm
char reset_status=1;
int led = 13;

/*----------Variables generales-------------*/
volatile int radiation;
float velocidad, humedad, temp; 
int s1=7, s2=6, s3=5;
unsigned long tiempoanterior=0;

/*---------Variables anemometro------------*/
unsigned long totalpulsos=0; //Variable donde se guarda el total de pulsos
float totalpulsosrps;
unsigned int velocidad_ent;
unsigned int velocidad_dec;

/*--------Variables sensor de radiacion solar----------*/
int PYR = A0;
unsigned int radiacionh;
unsigned int radiacionl;
int raw_radiation;

/*--------Variables VP3 (Sensor de humedad)-------------*/
unsigned int humedad_ent;
unsigned int humedad_dec;
unsigned int temp_ent;
unsigned int temp_dec;
char raw_temp_hum[16];

/*-----------Variables ETo--------------*/
float ETo;
int ETo_ent, ETo_dec;

/*-----------Constantes ETo------------*/
float Altitud=1440;   // Default value Chihuahua´s altitude
char current_month=5; // Default value 5-June

// Ra - Radiacion solar extraterrestre para latitud 28o
//    Ene 22.2  Feb 27.1  Mar 32.4  Abr 37.2  May 39.9  Jun 40.9 
//    Jul 40.4  Ago 38.3  Sep 34.4  Oct 29.2  Nov 23.8  Dic 21.1
float Ra_data_array[12]={22.2, 27.1, 32.4, 37.2, 39.9, 40.9, 40.4, 38.3, 34.4, 29.2, 23.8, 21.1}; 

// G - Flujo calorifico utilizado en el calentamiento del suelo
//    Ene 0.077   Feb 0.441   Mar 0.651   Abr 0.630   May 0.448   Jun -0.091  
//    Jul -0.231  Ago -0.175  Sep -0.336  Oct -0.455  Nov -0.609  Dic -0.350
float G_data_array[12]={0.077,0.441,0.651,0.630,0.448,-0.091,-0.231,-0.175,-0.336,-0.455,-0.609,-0.350 };

/*-----------Recibir datos y transmitir respuesta----------*/
void serialEvent(){
 char buffout[40];
 char buffin[40];
 char input_message_size=22;
 char output_message_size=32;
 int sum=0;
 
 if( standalone==0){
    if (Serial.available() > 0) {    
            Serial.readBytes(buffin,input_message_size);
            if(buffin[0] == 0x7E && buffin[15] == 'W'){

                    current_month=buffin[16];   // Get current month from message
                    Altitud=(float)(buffin[17]*256+buffin[18]);  // Get city altitude from message
    
                    buffout[0]=0x7E;//API mode start delimeter
                    buffout[1]=0x00;//Data length MSB
                    buffout[2]=output_message_size-4;//Data length LSB
                    buffout[3]=0x10;//API ID TX Request 16 bit address
                    buffout[4]=0x00;//Frame ID, if equals to 0x00 disable response frame
                    buffout[5]=buffin[4];//Destination Address MSB
                    buffout[6]=buffin[5];
                    buffout[7]=buffin[6];
                    buffout[8]=buffin[7];
                    buffout[9]=buffin[8];
                    buffout[10]=buffin[9];
                    buffout[11]=buffin[10];
                    buffout[12]=buffin[11];//Destination Address LSB
                    buffout[13]=0xFF;//Reserved
                    buffout[14]=0xFE;//Reserved
                    buffout[15]=0x00;
                    buffout[16]=0x00;
                    buffout[17]='W';//Identificador de nodo (31)
                    buffout[18]=radiacionh;//Radiacion solar MSB
                    buffout[19]=radiacionl;//Radiacion solar LSB
                    buffout[20]=humedad_ent;//Humedad entero
                    buffout[21]=humedad_dec;//Humedad decimal
                    buffout[22]=temp_ent;//Temperatura entero
                    buffout[23]=temp_dec;//Temperatura decimal
                    buffout[24]=velocidad_ent;//Velocidad viento entero
                    buffout[25]=velocidad_dec;//Velocidad viento decimal
                    buffout[26]=ETo_ent;//Evapotranspiracion entero
                    buffout[27]=ETo_dec;//Evapotranspiracion decimal
                    buffout[28]=(char)Ra_data_array[current_month];
                    buffout[29]=(char)(abs(G_data_array[current_month]*100));
                    buffout[30]=0x0A;//End of package
                    for(int i=3;i<output_message_size-1;i++){
                          sum+=buffout[i];
                    }
                    buffout[31]=0xFF-lowByte(sum); //Checksum
                    sum=0;
                    Serial.write(buffout,output_message_size);
              }
    }
 }
}


/*-----------Metodo para leer sensor de radiacion solar----------*/
void readPYR(){
    float tmp_rad;
    
    //Energizar sensor
    digitalWrite(s1, HIGH);
    delay(20);
    //Lectura analógica del sensor
    raw_radiation=analogRead(PYR);
    //Conversion a radiación solar watt/m2
    if(adc_1_1==1){
        tmp_rad = 1.07422*raw_radiation*5.0;//ADC de 1.1 volts
    } 
    else{
        tmp_rad = 4.88281*raw_radiation*5.0;//ADC de 5 volts  
    }   
    radiation = (int)tmp_rad;
    //Desenergizar sensor
    digitalWrite(s1, LOW);  
    //Dividir radiacion en 2 bytes
    radiacionh = highByte(radiation);
    radiacionl = lowByte(radiation);
}
 
 
/*------------Metodo para contar pulsos de anemometro-----------*/
void readWind(){ 
  totalpulsos = totalpulsos + 1;
}


/*-------------Metodo para leer datos de sensor de humedad y temperatura------------*/
void readVP3(){
    float valor;
    int x = 0;
    char a[6], b[6];
    char datos[16];
    int spcount=0;
    int i=0;
    //Energizar sensor
    digitalWrite(s2, HIGH);
    delay(50);
    
    //Esperar y leer datos provenientes del sensor
    if (Serial1.available()) {   
        Serial1.readBytes(datos,16);
        //Enviar datos leidos a variable gobal para monitoreo
        for(int i=0; i<=16; i++){
          raw_temp_hum[i]=datos[i];        
        }
        //Guardar vapor en arreglo "a"
        spcount=0;
        x=0;
        while(spcount!=1) {
            if(datos[x]==0x09) spcount++;
            x++;
        }
        spcount=0;
        i=0;
        while(spcount!=1) {
            if(datos[x]==' ') spcount++;
            a[i]=datos[x];
            i++;
            x++;
        }
        a[i]=0;
              
        //Guardar temperatura en arreglo "b" 
        spcount = 0;
        i=0;
        while(spcount!=1) {
            if(datos[x]==0x0D) spcount++;
            b[i]=datos[x];
            i++;
            x++;
        }
        b[i]=0;
        
        //Convertir arreglo "b" a variable flotante
        temp = atof(b); 
        //Convertir arreglo "b" a flotante y obtener humedad relativa
        humedad = (atof(a)/(0.611*pow(2.7182,((17.502*temp)/(240.97+temp))))*100);
    }
    //Desenergizar sensor
    digitalWrite(s2, LOW);

    //Dividir humedad en 2 bytes, 1 para enteros y 1 para decimales
    humedad_ent = humedad;
    humedad_dec = (humedad - humedad_ent)*100;
  
    //Dividir temperatura en 2 bytes, 1 para enteros y 1 para decimales y establecer bit mas significativo para signo
    if(temp<0){
          temp_ent = abs(temp) + 128;                                   
          temp_dec = ((abs(temp) - 128) - temp_ent)*100;
    }
    else{
          temp_ent = temp;
          temp_dec = (temp - temp_ent)*100;
    } 
}


/*---------Metodo de calculo de ETo---------------*/
float EtoCalc(float Rad_Eto, float Hum_Eto, float Temp_Eto, float Viento_Eto){
    float eto_mm_day;
    float Ra = Ra_data_array[current_month];
    float G = G_data_array[current_month];
    
    //Pendiente de la curva de la tensión de vapor saturado (kPa/ ºC)
    float delta = (2503.6*pow(2.7182,(17.27*Temp_Eto/(Temp_Eto+237.3))))/pow((Temp_Eto+237.3),2); 
    //Tension de vapor saturado a una temperatura dada en Kpa
    float es = 0.6108*pow(2.7182,(17.27*Temp_Eto/(Temp_Eto+237.3)));                              
    //Tension de vapor actual en Kpa
    float ea = es*Hum_Eto/100.0; 
    
    //Estimacion de Rn a partir de Ra según Hargreaves en MJ/(m^2 dia)
    float Rs=Rad_Eto*0.0864;      // Radiación solar en MJ/(m^2 dia)
    float alfa=0.23;              // albedo de la grama =0.23
    float sigma=4.903e-9;         // constante de Stefan-Boltzmann
    float Tkelvin=Temp_Eto+273;   // Temperatura en Kelvin
    float Rso=Ra*(2e-5*Altitud+0.75);
    float Rn=Rs*(1-alfa)-sigma*pow(Tkelvin,4)*(0.34-0.14*sqrt(ea))*((1.35*Rs/Rso)-0.35);
    float coef_u2;
    if( Rs > 0){
        coef_u2=0.24;   //0.24 dia
    }
    else{
        coef_u2=0.96;   //0.96 noche
    }

    // Velocidad del viento en m/s
    float u2 = Viento_Eto;
    // Presion atmosferica en Kpa
    float P=101.3*pow(((293-0.0065*Altitud)/293),5.26); //Presion atmosferica en Kpa
    // Constante psicrométrica (kPa/ ºC)
    float gamma = 0.000665*P;
    
    // Calculo de ETo por metodo de Penman-Monteith
    eto_mm_day= (0.408*delta*(Rn-G)+gamma*(900/(Temp_Eto+273))*u2*(es-ea))/(delta+gamma*(1+coef_u2*u2));
    return eto_mm_day;
}


void setup() {
   pinMode(s1, OUTPUT);
   pinMode(s2, OUTPUT);
   pinMode(s3, OUTPUT);
   pinMode(led, OUTPUT);
   pinMode(21, INPUT_PULLUP);
   attachInterrupt(2, readWind, FALLING); // Interrupcion en pin 21 para conteo por hardware
   Serial.begin(9600);//Puerto para comunicacion por xbee
   Serial1.begin(1200);//Puerto de comunicacion con sensor digital
   if(adc_1_1==1){
      analogReference(INTERNAL1V1);  //Utilizar para ADC de 1.1 volts
   }
   delay(1000);
   reset_status=1;
}
 
void loop() {
    float tmp;
    unsigned long tiempoactual=millis();

    if( (tiempoactual - tiempoanterior > tiempo) || reset_status) {
          tiempoanterior = tiempoactual;
          reset_status=0;
  
          // Lectura de velocidad de viento 
          totalpulsosrps = float(totalpulsos)/(tiempo/1000);
          velocidad = 2.25*0.44704*totalpulsosrps;          //Velocidad para RPS. No importa el valor del muestreo
          velocidad_ent = velocidad;
          velocidad_dec = (velocidad - velocidad_ent)*100;

          // Lectura de humedad y temperatura
          readVP3();

          // Lectura de radiación solar
          readPYR();   
 
          // Cálculo de ETo 
          ETo = EtoCalc(radiation,humedad,temp,velocidad);
          ETo_ent = ETo;
          ETo_dec =( ETo - ETo_ent )*100;

          // Monitoreo de datos
          if( standalone==1){
                Serial.println("-------------");
                Serial.println("Sample ");
                Serial.println("-------------");
                Serial.print("Viento (Raw): ");
                Serial.println(totalpulsos);
                Serial.print("Viento (m/s): ");
                Serial.println(velocidad);
                Serial.print("Radiacion (Raw): ");
                Serial.println(raw_radiation);
                Serial.print("Radiacion (watt/m2): ");
                Serial.println(radiation);
                Serial.print("Temp Humedad (Raw): ");
                for(int i=0; i<=16; i++){
                  if( raw_temp_hum[i]>=' ' ){
                    Serial.print(raw_temp_hum[i]);
                  }
                }
                Serial.println(" "); 
                Serial.print("Temperatura (oC): ");
                Serial.println(temp);
                Serial.print("Humedad (%): ");
                Serial.println(humedad);
                Serial.print("ETo (mm/day): ");
                Serial.println(ETo);
                Serial.print("G: ");
                Serial.println(G_data_array[current_month]);
                Serial.print("Ra: ");
                Serial.println(Ra_data_array[current_month]);
                Serial.print("Altitude: ");
                Serial.println(Altitud);
                Serial.print("Month: ");
                Serial.println(current_month+1);
          }
          
          totalpulsos=0;  
    }
}

