/*
23/11/18 - Versão final funcional.

- Utilizou-se um Arduino MEGA.
- Os structs foram criados para organizar e facilitar a definição dos pinos de cada componente.
- A interrupção foi criada para ativar e desativar o sistema (ISR_userInterrupt()).
- O código é controlado por uma máquina de estados de 4 estados.
- A função stateMachine() é responsável pela troca de estados, enquanto que stateAction() controla o que acontece em cada estado.
- Estado 0: Desligado. Neste estado, o sistema aguarda o pressionar o botão para ir para o estado 2.
- Estado 2: Aguardando caneta. Neste estado, o sistema aguarda a detecção de uma caneta antes de transitar para o estado 3.
- Estado 3: Identificando e descartando caneta. Neste estado, o sistema identifica a cor da caneta, ativar o controlador e descarta-a, retornando para o estado 2.
- Estado 4: INTERROMPIDO. Neste estado, o botão de interrupção foi pressionado nos estados 2 ou 3, interrompendo a operação do sistema e levando-o para o estado 0.
- Para identificação da cor, foi realizado o mapeamento dos valores lidos pelo sensor para o formato RGB.
- Os valores de mapeamento mínimo e máximo (0 e 255 no modelo RGB) foram coletados realizando ensaios em canetas com as cores preto e branca. Vale ressaltar que o mais adequado seria realizar um ensaio em uma câmara escura com um material mais reflexivo do que o utilizado, portanto a calibração não ficou tão boa quanto o desejado. Uma ideia parecida seria realizar a calibração sugerida no link a seguir: https://arduinoplusplus.wordpress.com/2015/07/15/tcs230tcs3200-sensor-calibration/
- Foi desenvolvido um controlador proporcional, visto que o motor pode ser modelado como um sistema integrador. Isso significa que o sistema é capaz de seguir referências com erro nulo, mas não rejeita totalmente perturbações de tipo degrau e outros. Essas perturbações foram desconsideradas.
- O motor possui zona morta de aproximadamente 1V e uma dinâmica de início que pode ser modelada como um atraso de 0.6s.
*/

#include <Servo.h>

// Sensor de cor (TCS3200):
struct TCS3200 {
    const byte OUT = 9;
    const byte S0 = 2;
    const byte S1 = 3;
    const byte S2 = 4;
    const byte S3 = 5;
    unsigned int RGB[3];  // Valores lidos pelo sensor.
};

// Sensor Infravermelho (detecção de presença):
struct IR {
    const byte OUT = 19;
};

// Motor Driver (Ponte H):
struct PONTEH {
    const byte ENA = 8;
    const byte IN1 = 24;
    const byte IN2 = 22;
};

// Botão simples:
struct BUTTON {
    const byte OUT = 18;  // Interrupt pin. Deve ser um desses (MEGA): 2, 3, 18, 19, 20, 21.
};

// Potenciômetro de 10k:
struct POTENTIOMETER {
    const byte OUT = A1;
};

// Inicialização dos structs para uso dos pinos configurados:
struct TCS3200 T;
struct IR I;
struct PONTEH H;
struct BUTTON B;
struct POTENTIOMETER P;

// Servo:
Servo S;  // Criar objeto para controlar o Servo.
byte pin_S = 20;  // Pino ao qual o OUT do servo está conectado.

// Ajustes de referência para a rampa (em Volts) e para o Servo (em graus):
float R_ref = 0.5; //
float G_ref = 1.5;
float B_ref = 3.5;
int pegar_caneta = 180;  // Graus de ajuste do servo para pegar caneta.
int descartar_caneta = 70;  // Graus de ajuste do servo para descartar caneta.

// Variáveis para o controlador:
float e;  // Erro.
float u;  // Ação de controle.
float r;  // Referência.
float y;  // Saída.
float kp = 1.0;  // Ganho proporcional.
float tol = 0.1;  // Faixa de erro tolerada = [-tol, +tol].
unsigned long t_max = 100;  // Tempo máximo para o controlador convergir.

// Outras variáveis:
unsigned short int current_state;  // define o estado atual.
bool done = false;  // Indica "fim" do processo de descarte das peças.
int t_descarte = 2000;  // Tempo até a caneta percorrer a rampa. Escolhido aleatoriamente.
volatile bool user_interrupt;  // o botao foi pressionado? -> nota: cuidar com interrupção + debounce. (== b).
bool presence_detected;  // alguma presença (caneta) foi detectada pelo sensor IR? (== p).
char color_reading;  // Leitura de cor.


void setup() {
    pinMode(B.OUT, INPUT);
    pinMode(I.OUT, INPUT);
    pinMode(T.OUT, INPUT);
    pinMode(P.OUT, INPUT);
    pinMode(T.S0, OUTPUT);
    pinMode(T.S1, OUTPUT);
    pinMode(T.S2, OUTPUT);
    pinMode(T.S3, OUTPUT);
    pinMode(H.IN1, OUTPUT);
    pinMode(H.IN2, OUTPUT);
    pinMode(H.ENA, OUTPUT);

    S.attach(pin_S);
    S.write(descartar_caneta);  // Iniciar Servo na posição para trancar a caneta.


    // Iniciar o TCS3200 em modo POWER-DOWN (economia de energia).
    digitalWrite(T.S0,LOW);
    digitalWrite(T.S1,LOW);

    // Iniciar no estado 0 (Arduino recém-ligado).
    current_state = 0;

    Serial.begin(9600);

    // Interrupção do botão para ativar ou desativar o projeto:
    attachInterrupt(digitalPinToInterrupt(B.OUT), ISR_userInterrupt, RISING);
}

void loop() {
    stateMachine();
    stateAction();
    delay(1000);  // Delay arbitrário.
}


/* ----- Funções relacionadas à MÁQUINA DE ESTADOS ----- */
// Controla a mudança de estados:
void stateMachine() {
    switch (current_state) {
    case 0:
        if (user_interrupt) {
            current_state = 2;
        }
        break;
    case 2:
        if (user_interrupt) {
            current_state = 4;
        }
        else if (presence_detected) {
            current_state = 3;
        }
        break;
    case 3:
        if (user_interrupt) {
            current_state = 4;
        }
        else if (done) {
            current_state = 2;
        }
        break;
    case 4:
        current_state = 0;
    }
}

// Controla o que acontece em cada estado:
// Nota: os user_interrupt foram colocados devido à interrupção do botão. Dessa forma, a interrupção consegue forçar o estado 4 e evitar que a máquina troque de estados sozinha como se o botão ainda estivesse sendo pressionado. Ou seja, eles servem para resetar o valor de user_interrupt.
void stateAction() {
    switch (current_state) {
    case 0:
        Serial.println("Estado atual: Desligado (0).");
        user_interrupt = false;
        break;

    case 2:
        presence_detected = !digitalRead(I.OUT);
        Serial.println("Estado atual: Esperando caneta (2).");

        user_interrupt = false;
        break;

    case 3:
        Serial.println("Estado atual: Identificando e descartando caneta (3).");
        done = false;
        color_reading = readColor();  // Ativar TCS3200 e identificar a cor.
        r = setRef(color_reading);  // Definir referência [tensão].
        activateController(r);  // Ativar o controlador e posicionar a rampa.

        if (user_interrupt == false) {
            spinServo();  // Girar o servo para pegar e descartar a caneta.
            delay(t_descarte);  // Esperar que a caneta seja descartada antes de processar a próxima.
            done = true;  // Fim do estado 3.
        }
        break;
    case 4:
        Serial.println("Estado atual: INTERROMPIDO (4).");
        user_interrupt = false;
        break;
    }
}


/* ----- Funções relacionadas ao CONTROLADOR ----- */
// Controlador de posição do sistema. Posiciona a rampa:
void activateController(float r) {
    unsigned long t0 = millis();
    unsigned long t = 0;

    // Cálculo preliminar do erro para verificar se é necessário calcular uma ação de controle.
    y = readPot();  // Saída de tensão do potenciômetro em [Volts].
    e = (r - y);  // Erro [Volts].

    // Parar o motor quando a rampa estiver posicionada com erro aceitável ou quando o tempo ultrapassar um valor limite.
    while( ((e < -tol) || (e > tol)) || (t < t_max) ) {
        y = readPot();  // Saída de tensão do potenciômetro em [Volts].
        e = r - y;  // Erro [Volts].
        u = U(e);  // Ação de controle [bits].

        // Se não houve interrupção do usuário, enviar 'u'. Caso contrário, finaliza o controlador.
        if(user_interrupt == false) {
            analogWrite(H.ENA, (int) u);
        }
        else {
            break;
        }
        t = t0 - millis();  // Calcular quanto tempo se passou desde que o controlador foi ativado.
    }
    stop_motor();  // Freiar o motor.
    return;
}

// Calcula e ajusta a ação de controle:
float U(float e) {
    u = kp*e;  // Ação de controle proporcional [Volts].
    u = spin_direction(u);  // Configura a Ponte H para que o motor gire no sentido adequado.
    u = bias(u);  // BIAS para resolver o problema da zona-morta.
    u = saturate(u);  // Garantir que 'u' opere na faixa [0, 5]V.
    u = rescale(u, 0.0, 5.0, 0.0, 255.0);  // Conversão para [bits].
    return u;
}

// Configura a Ponte H para que o motor gire no sentido adequado de acordo com a ação de controle calculada:
float spin_direction(float u) {
    if (u >= 0.0) { // Sentido Horário
        digitalWrite(H.IN1, HIGH);
        digitalWrite(H.IN2, LOW);
        return u;
    }
    else if (u < 0.0) { // Sentido Anti-Horário
        digitalWrite(H.IN1, LOW);
        digitalWrite(H.IN2, HIGH);
        return -u;
    }
}

// BIAS para resolver o problema da zona-morta:
float bias(float u) {
    if (u < 1.5) { // A zona-morta do motor utilizado é de aproximadamente 1V.
        u = u + 1.5;
    }
    return u;
}

// Limita 'u' à faixa [0, 5]V:
float saturate(float u) {
    if (u > 5.0) {
        u = 5.0;
    }
    else if (u < 0) {
        u = 0.0;
    }
    return u;
}

// Enviar 0V ao motor e configurar a Ponte H de forma a freiar o motor:
void stop_motor() {
    analogWrite(H.ENA, 0);
    digitalWrite(H.IN1, HIGH);
    digitalWrite(H.IN2, HIGH);
}

// Ler tensão de saída do potenciômetro e converter para [Volts]:
float readPot() {
    float pot_read = analogRead(P.OUT);  // Leitura de tensão na saída do potenciômetro [bits].
    pot_read = rescale(pot_read, 0.0, 1023.0, 0.0, 5.0);  // Conversão para [Volts].
    return pot_read;
}

// Definir referência do controlador com base na cor detectada:
float setRef(char color) {
    if (color == 'R') {
        return R_ref;
    }
    else if (color == 'G') {
        return G_ref;
    }
    else if (color == 'B') {
        return B_ref;
    }
}


/* ----- Funções relacionadas ao TCS3200 e ao SERVO ----- */
// Ativar o TCS3200 e identificar a cor:
char readColor() {
    // Modo POWER-ON:
    digitalWrite(T.S0,HIGH);
    digitalWrite(T.S1,LOW);

    // RED:
    digitalWrite(T.S2,LOW);
    digitalWrite(T.S3,LOW);
    T.RGB[0] = pulseIn(T.OUT, LOW);

    // GREEN:
    digitalWrite(T.S2,HIGH);
    digitalWrite(T.S3,HIGH);
    T.RGB[1] = pulseIn(T.OUT, LOW);

    // BLUE:
    digitalWrite(T.S2,LOW);
    digitalWrite(T.S3,HIGH);
    T.RGB[2] = pulseIn(T.OUT, LOW);

    // Conversão para formato RGB [0, 255]:
    T.RGB[0] = map(T.RGB[0], 108.0, 171.0, 255, 0);
    T.RGB[1] = map(T.RGB[1], 116.0, 193.0, 255, 0);
    T.RGB[2] = map(T.RGB[2], 78.0, 132.0, 255, 0);

    plotRGB(T.RGB[0], T.RGB[1], T.RGB[2]);

    // Identificar a cor detectada:
    if ((T.RGB[0] > T.RGB[2]) && (T.RGB[0] > T.RGB[1])) {
        Serial.println("Cor identificada: VERMELHO!");
        return 'R';
    } else if (T.RGB[1] > T.RGB[2] && T.RGB[1] > T.RGB[0]) {
        Serial.println("Cor identificada: VERDE!");
        return 'G';
    } else {
        Serial.println("Cor identificada: AZUL!");
        return 'B';
    };
}

// Imprime no monitor serial os valores RGB identificados:
void plotRGB(float R, float G, float B) {
    Serial.print("RGB");
    Serial.print(" (");
    Serial.print(R);
    Serial.print(':');
    Serial.print(G);
    Serial.print(':');
    Serial.println(B);
    Serial.print(')');
}

// Gira o Servo Motor de forma a pegar a caneta e descarta-la:
void spinServo() {
    S.write(pegar_caneta);
    delay(1250);
    S.write(descartar_caneta);
    delay(1250);
}


/* ----- Funções relacionadas às INTERRUPÇÕES ----- */
// Interrupção do botão (liga/desliga/parar):
void ISR_userInterrupt() {
    user_interrupt = true;
}


/* ----- Funções extras ----- */
// "Map" implementado para floats:
float rescale(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min)*(out_max - out_min)/(in_max - in_min) + out_min;
}
