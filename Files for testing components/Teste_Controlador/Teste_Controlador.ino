 /*

13/11/18 - Teste do controlador PD

*/


struct PONTEH {
    const byte ENA = 8;
    const byte IN1 = 24;
    const byte IN2 = 22;
};

struct BUTTON {
    const byte OUT = 18;  // Interrupt pin. One of these: 2, 3, 18, 19, 20, 21.
};

struct POTENTIOMETER {
    const byte OUT = A1;
};

struct PONTEH H;
struct BUTTON B;
struct POTENTIOMETER P;

bool done = false;  // Indica "fim" do processo de descarte das peças.
int t_descarte = 2000;  // Tempo até a caneta percorrer a rampa. Escolhido aleatoriamente.
volatile bool user_interrupt;  // Botão de interrupção pressionado.
volatile bool ref_id = 0;
float e_anterior[5] = {};  // Valores de erro anteriores (para uso com "tol").
int k = 0;  // Indice para "e_anterior".
int ref_id = 0;  // Indice para as referências.

// Variáveis para o controlador:
float e;  // Erro.
float u;  // Ação de controle.
float r;  // Referência.
float y;  // Saída.
float kp = 0.75;  // Ganho proporcional.
float tol = 0.1;  // Faixa de erro tolerada = [-tol, +tol].
unsigned long t_max = 10000;  // Tempo máximo para o controlador convergir.


void setup() {
    pinMode(B.OUT, INPUT);
    pinMode(P.OUT, INPUT);
  	pinMode(H.IN1, OUTPUT);
  	pinMode(H.IN2, OUTPUT);
  	pinMode(H.ENA, OUTPUT);
 	
    Serial.begin(9600);

    attachInterrupt(digitalPinToInterrupt(B.OUT), ISR_userInterrupt, RISING);
}

void loop() {
	user_interrupt = false;
	r = setRef(ref_id);  // Volts.
	activateController(r);  // Ativa o controlador de posição.
}

// Definir referência do controlador:
float setRef(int ref_id) {
    if (ref_id == 0) {
        return 0.5;
    }
    else if (ref_id == 1) {
        return 1;
    }
    else if (ref_id == 2) {
        return 1.5;
    }
	else if (ref_id == 3) {
        return 2;
    }
    else if (ref_id == 4) {
        return 2.5;
    }
	else if (ref_id == 5) {
        return 3;
    }
    else if (ref_id == 6) {
        return 3.5;
    }
	else if (ref_id == 7) {
        return 4;
    }
	else{
		ref_id = 0;
		return 4;
	}
}

// Ler tensão de saída do potenciômetro e converter para [Volts]:
float readPot() {
    float pot_read = analogRead(P.OUT);  // Leitura de tensão na saída do potenciômetro [bits].
    pot_read = rescale(pot_read, 0.0, 1023.0, 0.0, 5.0);  // [Volts].
    return pot_read;
}

// Controlador de posição do sistema. Posiciona a rampa:
void activateController(float r){
    unsigned long t0 = millis();
    unsigned long t = 0;
  
  // Cálculo preliminar do erro para verificar se é necessário calcular uma ação de controle.
  y = readPot();  // Saída de tensão do potenciômetro em [Volts].
  e = (r - y);  // Erro [Volts].
  
  // Parar o motor quando a rampa estiver posicionada com erro aceitável ou quando o tempo ultrapassar um valor limite.
    while( ((e < -tol) || (e > tol)) || (t < t_max) ){
    y = readPot();  // Saída de tensão do potenciômetro em [Volts].
    e = r - y;  // Erro [Volts].
    
	Serial.print(r); Serial.print(' '); Serial.print(y); Serial.print(' '); Serial.print(e); Serial.print(' ');
	
	u = U(e);  // Ação de controle [bits
	
    // Se não houve interrupção do usuário, enviar 'u'. Caso contrário, finaliza o controlador.
    if(user_interrupt == false){
      analogWrite(H.ENA, u);
    }
    else{
      break;
    }
    t = t0 - millis();
  }
    stop_motor();
  return;
}


float U(float e){
  u = kp*e;  // Ação de controle [Volts].
  u = spin_direction(u);  // Configura a Ponte H para que o motor gire no sentido adequado.
  u = bias(u);  // BIAS para resolver o problema da zona-morta.
  u = saturate(u);  // Garantir que 'u' opere na faixa [0, 5]V.
  Serial.println(u);
  u = rescale(u, 0.0, 5.0, 0.0, 255.0);  // Conversão para [bits].
  return u;
}


// Limita 'u' à faixa [0, 5]V.
float saturate(float u){
  if (u > 5.0){
    u = 5.0;
  }
  else if (u < 0){
    u = 0.0;
  }
  return u;
}

// BIAS para resolver o problema da zona-morta.
float bias(float u){
  if (u < 1.5){  // Dead-zone
      u = u + 1.5;
  }
  return u;
}

// Configura a Ponte H para que o motor gire no sentido adequado de acordo com a ação de controle calculada.
float spin_direction(float u){
  if (u >= 0.0){  // Horário
      digitalWrite(H.IN1, HIGH);
      digitalWrite(H.IN2, LOW);
      return u;
    }
    else if (u < 0.0){  // Anti-Horário
      digitalWrite(H.IN1, LOW);
      digitalWrite(H.IN2, HIGH);
      return -u;
    }
}

// Desliga e freia o motor.
void stop_motor() {
  analogWrite(H.ENA, 0);
  digitalWrite(H.IN1, HIGH);
  digitalWrite(H.IN2, HIGH);
}


// "Map" implementado para floats:
float rescale(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min)*(out_max - out_min)/(in_max - in_min) + out_min;
}

// Plotar variáveis do controlador (y, r, e, u).
void plot(){
  Serial.print(r); Serial.print(' '); Serial.print(y); Serial.print(' '); Serial.print(e); Serial.print(' '); Serial.println(u); 
}

// Interrupção do botão (liga/desliga/parar):
void ISR_userInterrupt(){
    user_interrupt = true;
	ref_id++;
	for(int i = 0; i<1000; i++){
	}  // Delay para tentar evitar debounce do botão.
}


// Lixão que ainda pode ser útil:
/* void pi(){
  y = analogRead(P.OUT);
    y = rescale(y, 0.0, 1023.0, 0.0, 5.0);  // Volts.  
    e[k] = (r - y);  // Volts
    u[k] = u[k-1] + 0.74*e[k] - 0.703*e[k-1];  // Volts.
    u[k] = saturate(u[k]);
    u_aux = spin_direction(u[k]);
    u_aux = bias(u_aux);  
    u_aux = rescale(u_aux, 0.0, 5.0, 0.0, 255.0);  // (0-5)V e (0-255)bits pwm.
    analogWrite(H.ENA, u_aux);
    e[k-1] = e[k];
    u[k-1] = u[k]; 
} */
