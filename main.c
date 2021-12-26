// Santiago Ortiz Ceballos
// Santiago Cardona Florez

#include "project.h"

// Variables Etapa de configuracion
int i=0;                                                // i & j: variables empleadas para el registro serial de cada comando
int j=0;
int etapa=9;                                            // Indica la etapa del proceso
char emergencia=0;                                      // Variable empleada en la etapa de radiacion

uint8 dato[3]={0};                                      // Datos recibidos en cada interrupcion del UART
//uint16 comando[13]={0};
uint16 comando[13]={360,90,270,0,2,3,1,0,50,0,1,0,0};    // Comandos a ejecutar

uint8 sentido_giro=0;                                   // Variable Etapa de Posicionamiento

// Variables empleadas para el cronometro
int relojito=0;                                         // Habilitador del cronometro
uint16 milisegundos;                                    // Variables que intervienen en el conteo regresivo
signed char segundos;
signed char minutos;

// Variables empleadas para el ADC
unsigned int medida_ADC;
unsigned int temperatura;
unsigned int radiacion;
char control_ADC=0;
char control_reloj=0;
char control_UART=0;

// Interrupciones definidas
CY_ISR(RX);
CY_ISR(UP);
CY_ISR(DOWN);
CY_ISR(ENTER);
CY_ISR(int_cronometro);
CY_ISR(int_ADC);
CY_ISR(int_ADC_termico);
CY_ISR(int_500ms);

// Funciones empleadas
void reloj(void);
void mmss(void);
void posicionamiento(uint16 posicion, uint16 velocidad);
void sensor_termico(void);
void sensor_radiacion(void);
void pantalla(void);
void dato_invalido(void);
void docklight(void);

int main(void)
{
    CyGlobalIntEnable; 
    UART_Start();                                       // COMPONENTES
    LCD_Start();
    
    PWM_Init();
    PWM_Aire_Init();
    PWM_Radiacion_Init();
    
    ADC_Start();
    ADC_IRQ_Enable();
    
    ADC_Termico_Start();
    ADC_Termico_IRQ_Enable();
    
    Contador_ADC_Start();
    
    isr_RX_StartEx(RX);                                 // INTERRUPCIONES INICIALIZADAS
    isr_UP_StartEx(UP);
    isr_DOWN_StartEx(DOWN);
    isr_ENTER_StartEx(ENTER);
    isr_reloj_StartEx(int_cronometro);
    isr_ADC_StartEx(int_ADC);
    isr_ADC_termico_StartEx(int_ADC_termico);
    isr_500ms_StartEx(int_500ms);
    
    LCD_ClearDisplay();
    LCD_Position(0,5);LCD_PrintString("Bienvenido");    // Interfaz de Bienvenida
    LCD_Position(2,2);LCD_PrintString("Equipo de Radio");
    LCD_Position(3,1);LCD_PrintString("Cirugia Robotica");
    CyDelay(000);LCD_ClearDisplay();
    
    for(;;)
    
    {
        // Configuracion
        
        if (control_UART==0){
            docklight();
            control_UART=1;
        }
        while (etapa==0){                               // Posicionamiento: comandos 0 --> 2, confirmacion: comando 3
            pantalla();
            
            // Dato Invalido: posicion > 360
            if (i==4 && (comando[0]>360 || comando[1]>360 || comando[2]>360 || comando[3]!=23) ){                
                i=0;
                j=0;
                memset(dato,0,3);
                comando[0]=0;
                comando[1]=0;
                comando[2]=0;
                comando[3]=0;
                dato_invalido();
                control_UART=0;
            }   
            if (i==4 && comando[3]==23){
                control_UART=2;
                etapa=1;
                LCD_ClearDisplay();CyDelay(300);
            }
        }
        
        if (control_UART==2){
            docklight();
            control_UART=3;
        }
        while(etapa==1){                                // Velocidad: comandos 4 --> 6, confirmacion: comando 7
            pantalla();
            
            // Dato Invalido: velocidad > 3
            if (i==8 && (comando[4]>3 || comando[5]>3 || comando[6]>3 || comando[7]!=23)){                
                i=4;
                j=0;
                memset(dato,0,3);
                comando[4]=0;
                comando[5]=0;
                comando[6]=0;
                comando[7]=0;
                dato_invalido();
                control_UART=2;
            }
            if (i==8 && comando[7]==23){
                control_UART=4;
                etapa=2;
                LCD_ClearDisplay();CyDelay(300); 
            }
        }
        
        if (control_UART==4){
            docklight();
            control_UART=5;
        }
        while(etapa==2){                                // Potencia de rad: comando 8, confirmacion: comando 9
            pantalla();
            
            // Dato Invalido: potencia > 100
            if (i==10 && (comando[9]!=23 || comando[8]>100) ){                
                i=8;
                j=0;
                memset(dato,0,3);
                comando[8]=0;
                comando[9]=0;
                dato_invalido();
                control_UART=4;
            }
            if (i==10 && comando[9]==23){
                control_UART=0;
                etapa=3;
                control_UART=6;
                LCD_ClearDisplay();CyDelay(300); 
            }
        }
        
        if (control_UART==6){
            docklight();
            control_UART=7;
        }
        while(etapa==3){                                // Tiempo de rad: comando 10, confirmacion: comando 11
            pantalla();
            
            // Dato Invalido: tiempo > 5
            if (i==12 && (comando[11]!=23 || comando[10]>5) ){                
                i=10;
                j=0;
                memset(dato,0,3);
                comando[10]=0;
                comando[11]=0;
                dato_invalido();
                control_UART=6;
            }
            if (i==12 && comando[11]==23){
                control_UART=0;
                etapa=4;
                control_UART=8;
                LCD_ClearDisplay();CyDelay(300); 
            }
        }
        
        if (control_UART==8){
            docklight();
            control_UART=9;
        }
        while(etapa==4){                                // Confirmacion: comando 12
            LCD_Position(0,13);LCD_PrintString("C:SI/NO");
            pantalla();
            
            // Dato Invalido != ( SI || NO )
            if (i==13 && (comando[12]!=156 && comando[12]!=157)){   
                i=12;
                j=0;
                comando[12]=0;
                dato_invalido();
                control_UART=8;
            }
            if (i==13 && comando[12]==156){             // Confirmar: SI
                LCD_ClearDisplay();CyDelay(1000);
                LCD_Position(0,0);LCD_PrintString("Configuracion Lista");CyDelay(2000);
                LCD_Position(2,0);LCD_PrintString("Iniciando");
                LCD_Position(3,0);LCD_PrintString("Procedimiento");CyDelay(2000);
                LCD_ClearDisplay();CyDelay(1000);
                
                pantalla();
                etapa=5; 
                control_UART=10;
            }
            if (i==13 && comando[12]==157){             // Confirmar: NO
                LCD_ClearDisplay(); LCD_PrintString("Try Again");CyDelay(500);LCD_ClearDisplay();
                etapa=0;
                i=0;
                memset(comando,0,15);LCD_ClearDisplay();
                control_UART=0;
            }                      
        }
        
        // Funcionamiento 
        
        // 1) Posicionamiento
        if (etapa==5 && sentido_giro==0){               // Mov. Codo 
            pantalla();
            sentido_giro=1;
            CyDelay(1000);posicionamiento(comando[0],comando[4]);CyDelay(1000);
        }
        if (etapa==6 && sentido_giro==0){               // Mov. Brazo
            sentido_giro=1;
            CyDelay(1000);posicionamiento(comando[1],comando[5]);CyDelay(1000);
        }
        if (etapa==7 && sentido_giro==0){               // Mov. Base
            sentido_giro=1;
            posicionamiento(comando[2],comando[6]);CyDelay(1000);
        }
        
        // 2) Calentamiento de 1 minuto
        if (etapa==8){
            segundos=0;
            minutos=1;
            pantalla();          
            PWM_Stop();PWM_WritePeriod(499);PWM_WriteCompare(499);PWM_Start();
            PWM_Radiacion_Stop();
            PWM_Radiacion_WritePeriod(2000);
            PWM_Radiacion_WriteCompare(2000);
            PWM_Radiacion_Start();
            Cronometro_Start();    
            
            
            while (etapa==8){
                reloj();
                LCD_Position(2,15);LCD_PrintString("Rad=");LCD_PrintNumber(radiacion);
                LCD_Position(3,13);LCD_PrintString("T=");LCD_PrintNumber(temperatura);
                LCD_Position(3,18);LCD_PrintString("*C");
                
                PWM_Radiacion_Stop();
                PWM_Radiacion_WritePeriod(2000);
                PWM_Radiacion_WriteCompare(2000);
                PWM_Radiacion_Start();
                
            }
        }
        
        // 3) Radiacion (tiempo, potencia)
        if (etapa==9){
            LCD_ClearDisplay();buzzer_Write(0);
            CyDelay(500);
            segundos=0;
            minutos=comando[10];                        // Minutos de radiacion establecidos
            Cronometro_Start();                         // Cuenta regresiva de 1 minuto
            control_reloj=0;
            pantalla();
            
            while (etapa==9){
                if (relojito==1){                       
                    relojito=0;
                    segundos--;
                    if(segundos==-1){
                        if (minutos==0){     
                            segundos=0;
                            buzzer_Write(1);       
                            Cronometro_Stop();
                            PWM_Radiacion_Stop();
                            etapa=10;                   // Avanza la etapa al finalizar el conteo regresivo
                            CyDelay(1000);buzzer_Write(0);
                        }
                        else{
                            minutos--;
                            segundos=59;
                        }
                    }
                }
                
                PWM_Radiacion_Stop();
                PWM_Radiacion_WritePeriod(2000);
                PWM_Radiacion_WriteCompare((2000/100)*comando[8]);
                PWM_Radiacion_Start();
                
                if (segundos==30 && control_reloj==0){
                    control_reloj=1;
                    
                    if (radiacion < 1){                     // En caso de emergencia se borra el LCD y solo muestra
                        emergencia=1;                       // el mensaje definido por 1 minuto     
                        
                        LCD_ClearDisplay();
                        LCD_Position(0,0);LCD_PrintString("Radiacion");
                        LCD_Position(1,0);LCD_PrintString("Insuficiente");
                        UART_PutString("Radiacion Insuficiente");
                        Mal_Estado_Write(1);                // Led indicador de mal estado
                        minutos=1;
                        segundos=0;
                        }
                    
                    if (radiacion > 3){                     // En caso de emergencia se borra el LCD y solo muestra
                        emergencia=1;                       // el mensaje definido por 1 minuto   
                        
                        LCD_ClearDisplay();
                        LCD_Position(0,0);LCD_PrintString("Radiacion");
                        LCD_Position(1,0);LCD_PrintString("Excesiva");
                        UART_PutString("Radiacion Excesiva");
                        Mal_Estado_Write(1);                // Led indicador de mal estado
                        minutos=1;
                        segundos=0;
                    }
                }
                
                if (emergencia==0){                                 // Imprime todo mientras no sea una emergencia
                    LCD_Position(2,15);LCD_PrintString("Rad=");LCD_PrintNumber(radiacion);
                    LCD_Position(3,13);LCD_PrintString("T=");LCD_PrintNumber(temperatura);
                    LCD_Position(3,18);LCD_PrintString("*C");
                    mmss();                                         // Muestra minutos y segundos
                }
            }
        }
        
        // 4) Reset
        
        if (etapa==10){                                 // Mov. Base
            PWM_Radiacion_Stop();
            if(emergencia==1){
                LCD_ClearDisplay();
                LCD_Position(2,15);LCD_PrintString("Rad=");LCD_PrintNumber(radiacion);
                LCD_Position(3,13);LCD_PrintString("T=");LCD_PrintNumber(temperatura);
                LCD_Position(3,18);LCD_PrintString("*C");
                sentido_giro=2;
                CyDelay(1000);posicionamiento(comando[2],comando[6]);CyDelay(1000);
            }
            else{
                pantalla();
                sentido_giro=2;
                CyDelay(1000);posicionamiento(comando[2],comando[6]);CyDelay(1000);
            }
        }
        if (etapa==11){                                 // Mov. Brazo
            if(emergencia==1){
                LCD_ClearDisplay();
                LCD_Position(2,15);LCD_PrintString("Rad=");LCD_PrintNumber(radiacion);
                LCD_Position(3,13);LCD_PrintString("T=");LCD_PrintNumber(temperatura);
                LCD_Position(3,18);LCD_PrintString("*C");
                sentido_giro=2;
                CyDelay(1000);posicionamiento(comando[1],comando[5]);CyDelay(1000);
            }
            else{
                pantalla();
                sentido_giro=2;
                CyDelay(1000);posicionamiento(comando[1],comando[5]);CyDelay(1000);
            }
        }
        if (etapa==12){                                 // Mov. Codo
            if(emergencia==1){
                LCD_ClearDisplay();
                LCD_Position(2,15);LCD_PrintString("Rad=");LCD_PrintNumber(radiacion);
                LCD_Position(3,13);LCD_PrintString("T=");LCD_PrintNumber(temperatura);
                LCD_Position(3,18);LCD_PrintString("*C");
                sentido_giro=2;
                CyDelay(1000);posicionamiento(comando[0],comando[4]);CyDelay(1000);
            }
            else{
                pantalla();
                sentido_giro=2;
                CyDelay(1000);posicionamiento(comando[0],comando[4]);CyDelay(1000);
            }
        }
        
        if (etapa==13){
            LCD_ClearDisplay();leds_Write(0b0000);Mal_Estado_Write(0);    // Proceso Finalizado
            UART_PutString("Proceso Finalizado");
            LCD_Position(0,5);LCD_PrintString("Proceso");
            LCD_Position(1,3);LCD_PrintString("Finalizado");CyDelay(1000);
            
            i=0;                                                         // Reseteo de variables
            PWM_Stop();
            control_reloj=0;
            control_ADC=0;
            emergencia=0;
            memset(comando,0,13);etapa=0;LCD_ClearDisplay();
            
            LCD_Position(0,5);LCD_PrintString("Bienvenido");            // Interfaz de Bienvenida
            LCD_Position(2,2);LCD_PrintString("Equipo de Radio");
            LCD_Position(3,1);LCD_PrintString("Cirugia Robotica");
            CyDelay(1000);LCD_ClearDisplay();
        }
    }
}


// INTERRUPCIONES

CY_ISR(RX){                                             // Almacena los datos recibidos por comunicacion serial
    j++;
    if (j==1){
        dato[0]=UART_ReadRxData();
        if (i==4 || i==5 || i==6 || i==10){             // Almacena Velocidad & tiempo
            j=0;
            i++;
            comando[i-1]=dato[0];
            memset(dato,0,3);                           // Resetea el Vector Dato luego de ser utilizado
            
        }
    }
    
    if (j==2){
        dato[1]=UART_ReadRxData();
        if (i==3 || i==7 || i==9 || i==11 || i== 12){   // Almacena: Enter, SI & NO
            j=0;
            i++;
            comando[i-1]=( (dato[0]) + (dato[1]) );
            memset(dato,0,3);  
            
        }
    }
    
    if (j==3){                                          // Almacena Posicion & potencia
        dato[2]=UART_ReadRxData();
        j=0;
        i++;
        comando[i-1]=( 100*(dato[0]) + 10*(dato[1]) + (dato[2]) );
        memset(dato,0,3);                               // Resetea el Vector Dato luego de ser utilizado
    }
    
    if (etapa > 4){                                             // Impresion en la interfaz a solicitud
        if (j==1){
            dato[0]=UART_ReadRxData();
        }
        if (j==2){
            dato[1]=UART_ReadRxData();
        }
        if (dato[0]+dato[1]==23){
            j=0;
            memset(dato,0,3);
            
            UART_PutString("A1=");UART_PutChar((comando[0]/10)+48);UART_PutChar((comando[0]%10)+48);UART_PutChar(' ');
            UART_PutString("A2=");UART_PutChar((comando[1]/10)+48);UART_PutChar((comando[1]%10)+48);UART_PutChar(' ');
            UART_PutString("A3=");UART_PutChar((comando[2]/10)+48);UART_PutChar((comando[2]%10)+48);UART_PutChar(' ');
            
            UART_PutString("V1=");UART_PutChar((comando[4]/10)+48);UART_PutChar((comando[4]%10)+48);UART_PutChar(' ');
            UART_PutString("V2=");UART_PutChar((comando[5]/10)+48);UART_PutChar((comando[5]%10)+48);UART_PutChar(' ');
            UART_PutString("V3=");UART_PutChar((comando[6]/10)+48);UART_PutChar((comando[6]%10)+48);UART_PutChar(' ');
            
            UART_PutString("Potencia=");UART_PutChar((comando[8]/10)+48);UART_PutChar((comando[8]%10)+48);UART_PutChar(' ');
            
            UART_PutString("Tiempo de radiacion=");UART_PutChar((comando[10]/10)+48);UART_PutChar((comando[10]%10)+48);UART_PutChar(' ');
        }
    }
}

CY_ISR(UP){
    LCD_ClearDisplay();
    if (etapa==2){                                      // Aumenta en 1 el valor de potencia
        i=8;comando[8]++;
        LCD_ClearDisplay();
    }
    if (etapa==3 && comando[10]<5){                     // Aumenta en 1 el valor de tiempo
        i=10;comando[10]++;
        LCD_ClearDisplay();
    }
}

CY_ISR(DOWN){
    if (etapa==2){                                      // Disminuye en 1 el valor de potencia
        i=8;comando[8]--;
        LCD_ClearDisplay();
    }
    if (etapa==3 && comando[10]>0){                     // Disminuye en 1 el valor de tiempo
        i=10;comando[10]--;
        LCD_ClearDisplay();
    }
}

CY_ISR(ENTER){                                          // Almacena un 23 en representacion del ENTER en ASCII
    if (etapa==2){
        i=10;comando[9]=23;
    }
    if (etapa==3){
        i=12;comando[11]=23;
    }
    if (etapa==4){
        i=13;comando[12]=156;
    }
    if (etapa==9){                                     // Emergencia durante la etapa de radiacion
        Mal_Estado_Write(1);
        emergencia=0;
        control_reloj=1;
        minutos=0;
        segundos=0;
    }
}

CY_ISR(int_cronometro){                                 // Interrupcion que genera el conteo regresivo
    relojito=1;
}
CY_ISR(int_ADC){                                        // Registra la medida del ADC, y se activa siempre que el  
    radiacion=1.22*ADC_GetResult16();                       // contador de 0.5 segundos de la señal
    sensor_radiacion();
}
CY_ISR(int_ADC_termico){                                     // Registra la medida del ADC, y se activa siempre que el  
    temperatura=ADC_Termico_GetResult16()/10;               // contador de 0.5 segundos de la señal
    sensor_termico();
}
CY_ISR(int_500ms){                                      // Cada 0.5 segundos, se activa el sensado del ADC
    ADC_StartConvert();
    ADC_Termico_StartConvert();
    control_ADC=1;
}

// FUNCIONES

void reloj(void){
    if (relojito==1){                       // Cuenta regresiva de 1 minuto
        relojito=0;
        segundos--;
        if(segundos==-1){
            if (minutos==0){     
                segundos=0;
                buzzer_Write(1);       
                Cronometro_Stop();
                PWM_Stop();
                PWM_Radiacion_Stop();
                etapa=9;                    // Avanzar etapa al finalizar el conteo regresivo
                CyDelay(1000);buzzer_Write(0);
            }
            else{
                minutos--;
                segundos=59;
            }
        }
        
        LCD_Position(0,15);
        LCD_PutChar((minutos/10)+'0');
        LCD_PutChar((minutos%10)+'0');
        LCD_PutChar(':');
        LCD_PutChar((segundos/10)+'0');
        LCD_PutChar((segundos%10)+'0');
    }
}

void mmss(void){
    LCD_Position(0,15);
    LCD_PutChar((minutos/10)+'0');
    LCD_PutChar((minutos%10)+'0');
    LCD_PutChar(':');
    LCD_PutChar((segundos/10)+'0');
    LCD_PutChar((segundos%10)+'0');
}

void posicionamiento(uint16 posicion, uint16 velocidad){
    uint16 paso=0;
    if (velocidad==1) velocidad=39; // 39
    if (velocidad==2) velocidad=78;
    if (velocidad==3) velocidad=117;
    
    
    if(sentido_giro==1){
        sentido_giro=0;
        for (paso=0; paso < posicion*4.28; paso++){
            if (leds_Read()==0b0000){                   // Si el estado es 0, ponga el motor en estado 1
                leds_Write(0b1100);
                CyDelay(velocidad);
            }
            if (leds_Read()==0b0011){                   // Si el estado es 3, ponga el motor en estado 4
                leds_Write(0b1001);
                CyDelay(velocidad);
            }
            if (leds_Read()==0b1001){                   // Si el estado es 4, ponga el motor en estado 1
                leds_Write(0b1100);
                CyDelay(velocidad);
            }
            else{
                leds_Write(leds_Read() >> 1 );
                CyDelay(velocidad);
            }
        }
        etapa++;
    }
    
    if(sentido_giro==2){
        sentido_giro=0;
        for (paso=0; paso < posicion*4.28; paso++){
            if (leds_Read()==0b0000){                   // Si el estado es 0, ponga el motor en estado 1
                leds_Write(0b0011);
                CyDelay(velocidad);
            }
            if (leds_Read()==0b1100){                   // Si el estado es 3, ponga el motor en estado 4
                leds_Write(0b1001);
                CyDelay(velocidad);
            }
            if (leds_Read()==0b1001){                   // Si el estado es 4, ponga el motor en estado 1
                leds_Write(0b0011);
                CyDelay(velocidad);
            }
            else{
                leds_Write(leds_Read() << 1 );
                CyDelay(velocidad);
            }
        }
        etapa++;
    }
}

void sensor_termico(void){                              // ADC: Sensor Termico a voltaje
 
    if (temperatura <= 20){
        PWM_Aire_Stop();
        PWM_Aire_WritePeriod(1999);
        PWM_Aire_WriteCompare(1999*0.4);    
        PWM_Aire_Start();    
    }
    if (temperatura > 20 && temperatura <= 35){
        PWM_Aire_Stop();
        PWM_Aire_WritePeriod(1999);
        PWM_Aire_WriteCompare(1999*0.5);    
        PWM_Aire_Start(); 
        
    }
    if (temperatura > 35 && temperatura <= 60){
        PWM_Aire_Stop();
        PWM_Aire_WritePeriod(1999);
        PWM_Aire_WriteCompare(1999*0.6);    
        PWM_Aire_Start();
        
    }
    if (temperatura > 60 && temperatura <= 100){
        PWM_Aire_Stop();
        PWM_Aire_WritePeriod(1999);
        PWM_Aire_WriteCompare(1999*0.7);    
        PWM_Aire_Start();
        
    }
}

void sensor_radiacion(void){                                // ADC: Sensor de voltaje (trimer)
    if (radiacion <= 1000){
       radiacion=radiacion/1000;
    }
    
    if (radiacion > 1000 && radiacion < 2500){
        radiacion=( (radiacion - 250) / 750 );
    }
    
    if (radiacion >= 2500 && radiacion <= 4990){
        radiacion=(radiacion+5000)/2500;
    }
    if (radiacion > 4990){
        radiacion=4;
    }
}

void pantalla(void){                                                    // Imprime formato de pantalla en el LCD
    LCD_Position(0,2);LCD_PutChar('B');
    LCD_Position(0,6);LCD_PutChar('C');
    LCD_Position(0,10);LCD_PutChar('M');
    LCD_Position(1,0);LCD_PutChar('A');
    LCD_Position(1,2);LCD_PrintNumber(comando[0]);
    LCD_Position(1,6);LCD_PrintNumber(comando[1]);
    LCD_Position(1,10);LCD_PrintNumber(comando[2]); 
    LCD_Position(2,0);LCD_PrintString("V");
    LCD_Position(2,2);LCD_PrintNumber(comando[4]);
    LCD_Position(2,6);LCD_PrintNumber(comando[5]);
    LCD_Position(2,10);LCD_PrintNumber(comando[6]);
    LCD_Position(3,0);LCD_PrintString("P");
    LCD_Position(3,2);LCD_PrintNumber(comando[8]);     
    LCD_Position(3,6);LCD_PrintString("t");
    LCD_Position(3,8);LCD_PrintNumber(comando[10]);
    LCD_Position(2,15);LCD_PrintString("Rad=");LCD_PrintNumber(radiacion);
    LCD_Position(3,13);LCD_PrintString("T=");LCD_PrintNumber(temperatura);
    LCD_Position(3,18);LCD_PrintString("*C");
    
    if (etapa==0){                                                  // Se indica la etapa de configuracion actual
        LCD_Position(1,1);LCD_PrintString("*");
    }
    else{
        LCD_Position(1,1);LCD_PrintString("-");
    }
    
    if (etapa==1){
        LCD_Position(2,1);LCD_PrintString("*");
    }
    else{
        LCD_Position(2,1);LCD_PrintString("-");
    }
    
    if (etapa==2){
        LCD_Position(3,1);LCD_PrintString("*");
    }
    else{
        LCD_Position(3,1);LCD_PrintString("-");
    }
    
    if (etapa==3){
        LCD_Position(3,7);LCD_PrintString("*");
    }
    else{
        LCD_Position(3,7);LCD_PrintString("-");
    }
}

void dato_invalido(void){
    memset(dato,0,3);
    LCD_ClearDisplay();LCD_PrintString("Dato Invalido");CyDelay(500);LCD_ClearDisplay();
}

void docklight(void){
    if(etapa==0){
        UART_PutString("Ingrese los angulos de posicionamiento: ");
    }
    if(etapa==1){
        UART_PutString("Ingrese las velocidades de posicionamiento: ");
    }
    if(etapa==2){
        UART_PutString("Ingrese la potencia de radiacion (%): ");
    }
    if(etapa==3){
        UART_PutString("Ingrese el tiempo de radiacion (min): ");
    }
    if(etapa==4){
        UART_PutString("Confirme la informacion e inicie el procedimiento: ");
    }
    
}
