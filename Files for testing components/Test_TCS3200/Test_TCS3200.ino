/* TESTAR E CALIBRAR O SENSOR DE COR TCS3200 */

/*
 * Conecte os pinos do TCS3200 de acordo com a pinagem do seu struct (MEGA).
 * Todos os pinos são digitais, incluindo sua alimentação VCC 5V.
 * O pino de VCC do TCS3200 será usado para desligar seus leds.
 * O pino OUT será usado para determinar a cor detectada.
 * Os restantes são pinos de configuração de frequência (S0 e S1) e cor a ler (S2 e S3).
 *
 * Agora execute o código e abra o MONITOR SERIAL. Lá serão encontradas as leituras realizadas.
 * A cada 5 leituras, aparecerá a média das últimas 5 leituras.
 * Anote essas leituras de média nos respectivos struct da cor sendo testada.
 *
 * Por exemplo, se os testes estão sendo realizado sobre uma peça VERMELHA
 * anote as médias de leituras no struct RED (as variáveis desse struct
 * não serão usadas neste programa. Ela servem apenas para anotação).
 * Esses valores serão usados pelo TCS3200 na aplicação real para detectar
 * a cor da peça. Não esqueça de determinar uma faixa de tolerância
 * quando for aplicar esses valores.
 */

struct TCS3200 {
    const byte OUT = 9;
    const byte S0 = 2;
    const byte S1 = 3;
    const byte S2 = 4;
    const byte S3 = 5;
};

// PARA ANOTAÇÃO DA CALIBRAÇÃO DO SENSOR:
// Valores de leitura do TCS3200 pra cor vermelha:
struct RED {
    float w = 100.0;
    float r = 80.0;
    float g = 100.0;
    float b = 90.0;
};

// Valores de leitura do TCS3200 pra cor verde:
struct GREEN {
    float w = 80.0;
    float r = 70.20;
    float g = 76.40;
    float b = 62.60;
};

// Valores de leitura do TCS3200 pra cor azul:
struct BLUE {
    float w = 33.0; 
    float r = 100.0;
    float g = 120.0;
    float b = 80.0;
};

struct TCS3200 T;
struct RED R;
struct GREEN G;
struct BLUE B;

int k = 1;
float readw = 0;
float readr = 0;
float readg = 0;
float readb = 0;


void setup() {
    pinMode(T.S0, OUTPUT);
    pinMode(T.S1, OUTPUT);
    pinMode(T.S2, OUTPUT);
    pinMode(T.S3, OUTPUT);
    pinMode(T.OUT, INPUT);

    // Setting frequency-scaling to 20%
    digitalWrite(T.S0,HIGH);
    digitalWrite(T.S1,LOW);

    Serial.begin(9600);
}

void loop() {
    if(k <= 5) {
        detectColor(T.OUT);
        k++;
    }
    else {
        k = 1;
        printMeanValues();
        resetReadValues();
    }
}

// Detectar cor. Faz chamada para leituras das cores.
void detectColor(int tcsOutPin) {
    float white = colorRead(tcsOutPin,0);
    float red = colorRead(tcsOutPin,1);
    float blue = colorRead(tcsOutPin,2);
    float green = colorRead(tcsOutPin,3);

    Serial.print("white ");
    Serial.println(white);
    readw += white;

    Serial.print("red ");
    Serial.println(red);
    readr += red;

    Serial.print("green ");
    Serial.println(green);
    readg += green;

    Serial.print("blue ");
    Serial.println(blue);
    readb += blue;

    

    Serial.println(' ');
}

// Ler cor na frequência maxima (100%).
float colorRead(int tcsOutPin, int color) {
//turn on sensor and use highest frequency
    tcsMode(2);

    int sensorDelay = 200;

//set the S2 and S3 pins to select the color to be sensed
    if(color == 0) { //white
        digitalWrite(T.S3, LOW); //S3
        digitalWrite(T.S2, HIGH); //S2
// Serial.print(" w");
    }

    else if(color == 1) { //red
        digitalWrite(T.S3, LOW); //S3
        digitalWrite(T.S2, LOW); //S2
// Serial.print(" r");
    }

    else if(color == 2) { //blue
        digitalWrite(T.S3, HIGH); //S3
        digitalWrite(T.S2, LOW); //S2
// Serial.print(" b");
    }

    else if(color == 3) { //green
        digitalWrite(T.S3, HIGH); //S3
        digitalWrite(T.S2, HIGH); //S2
// Serial.print(" g");
    }

    float readPulse;

// wait a bit for LEDs to actually turn on
    delay(sensorDelay);

// now take a measurement from the sensor, timing a low pulse on the sensor's "out" pin
    readPulse = pulseIn(tcsOutPin, LOW);

//if the pulseIn times out, it returns 0 and that throws off numbers. just cap it at 80k if it happens
//    if(readPulse < .1) {
//        readPulse = 80000;
//    }

//turn off color sensor and LEDs to save power
    //tcsMode(0);

    return readPulse;
}

// Imprimir media dos valores lidos.
void printMeanValues() {
    Serial.print("Mean Values:");
    Serial.print(" W");
    Serial.print(readw/5);
    Serial.print(" R");
    Serial.print(readr/5);
    Serial.print(" G");
    Serial.print(readg/5);
    Serial.print(" B");
    Serial.print(readb/5);
    Serial.println(' ');
}

// Resetar valores lidos.
void resetReadValues() {
    readw = 0.0;
    readr = 0.0;
    readg = 0.0;
    readb = 0.0;
}

// Definir modo do tcs (ligado, desligado, frequência).
void tcsMode(int mode) {
    if(mode == 0) {
        //power OFF mode-  LED off and both channels "low"
        digitalWrite(T.S0, LOW); //S0
        digitalWrite(T.S1, LOW); //S1
        //  Serial.println("mOFFm");

    } else if(mode == 1) {
        //this will put in 1:1, highest sensitivity
        digitalWrite(T.S0, HIGH); //S0
        digitalWrite(T.S1, HIGH); //S1
        // Serial.println("m1:1m");

    } else if(mode == 2) {
        //this will put in 1:5
        digitalWrite(T.S0, HIGH); //S0
        digitalWrite(T.S1, LOW); //S1
        //Serial.println("m1:5m");

    } else if(mode == 3) {
        //this will put in 1:50
        digitalWrite(T.S0, LOW); //S0
        digitalWrite(T.S1, HIGH); //S1
        //Serial.println("m1:50m");
    }
    return;
}
