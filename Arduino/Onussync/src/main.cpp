#include <Arduino.h>
#include <LiquidCrystal.h>
#include <HX711.h>
#include <SoftwareSerial.h>

// --- MAPA DE INFRAESTRUTURA FÍSICA ---
LiquidCrystal lcd(7, 6, 5, 4, 3, 2); 
const int LOADCELL_DOUT_PIN = 8;
const int LOADCELL_SCK_PIN = 9;
SoftwareSerial esp8266(10, 11); // RX no 10, TX no 11
const int BOTAO_PIN = 12;

const float FATOR_CALIBRACAO = 691.43;

// --- CONFIGURAÇÕES DO WI-FI E API ---
String SSID = "motog54"; 
String PASSWORD = "caio12345"; 
String SERVER = "patinha-smart.onrender.com"; 
String ENDPOINT = "/status";
String PORTA = "443"; // Criptografia ativada (HTTPS)

HX711 scale;

// --- DESIGN DOS ÍCONES CUSTOMIZADOS ---
byte iconePeso[8] = { B00100, B01110, B01110, B11111, B11111, B11111, B11111, B00000 };
byte iconeWifi[8] = { B00000, B11111, B00001, B01110, B00100, B00000, B00100, B00000 };

// --- CRONÔMETROS E CONTROLES DE TAREFA ---
float pesoAtual = 0.0; 

unsigned long tempoUltimaLeituraLCD = 0;
const unsigned long INTERVALO_LEITURA_LCD = 1000; // Atualiza a tela a cada 1 seg

unsigned long tempoUltimoEnvioTempoReal = 0;
const unsigned long INTERVALO_TEMPO_REAL = 5000; // Dispara pra API a cada 5 seg

unsigned long tempoUltimoEnvioHistorico = 0;
const unsigned long INTERVALO_ENVIO_HISTORICO = 1800000; // Dispara histórico a cada 30 min

const float LIMITE_ALERTA_GRAMAS = 200.0; // Abaixo de 200g, dispara alerta
bool alertaEnviado = false; 

// --- VARIÁVEIS DO SCANNER DE WI-FI ---
bool modoScanner = false;
bool ultimoEstadoBotao = HIGH; 
String redesEncontradas[10]; 
int totalRedes = 0;
unsigned long tempoUltimaTrocaLCD = 0;
int indiceRedeLCD = 0;

// --- FUNÇÃO AUXILIAR DE COMANDO ESP-01 ---
String enviarComandoAT(String comando, const int tempoEspera, boolean debug) {
  String resposta = "";
  esp8266.println(comando);
  long int tempo = millis();
  while ((tempo + tempoEspera) > millis()) {
    while (esp8266.available()) {
      char c = esp8266.read();
      resposta += c;
    }
  }
  if (debug) Serial.print(resposta); // Imprime a resposta do servidor
  return resposta;
}

// --- FUNÇÃO PARA MONTAR E ENVIAR O PACOTE HTTP ---
void dispararParaAPI(String tipoDeEvento, float pesoDaVez) {
  String json = "{\"tipo\": \"" + tipoDeEvento + "\", \"racao\": " + String(pesoDaVez, 1) + "}";
  String requisicao = "POST " + ENDPOINT + " HTTP/1.1\r\n";
  requisicao += "Host: " + SERVER + "\r\n";
  requisicao += "Content-Type: application/json\r\n";
  requisicao += "Content-Length: " + String(json.length()) + "\r\n";
  requisicao += "Connection: close\r\n\r\n";
  requisicao += json;

  Serial.println("\n>>> DISPARANDO DADOS: " + tipoDeEvento + " (" + String(pesoDaVez, 1) + "g) <<<");
  
  // Conexão SSL ativada com tempo de handshake ampliado para 6000ms
  enviarComandoAT("AT+CIPSTART=\"SSL\",\"" + SERVER + "\"," + PORTA, 6000, true);
  enviarComandoAT("AT+CIPSEND=" + String(requisicao.length()), 2000, true);
  enviarComandoAT(requisicao, 5000, true); 
  enviarComandoAT("AT+CIPCLOSE", 1000, true);
}

// --- O RADAR ---
void buscarESalvarRedes() {
  totalRedes = 0; 
  esp8266.println("AT+CWLAP"); 
  long int tempo = millis();
  String linhaAtual = "";
  
  while ((tempo + 10000) > millis()) { 
    while (esp8266.available()) {
      char c = esp8266.read();
      if (c == '\n' || c == '\r') {
        if (linhaAtual.startsWith("+CWLAP:") && totalRedes < 10) {
          int primeiraAspas = linhaAtual.indexOf('"');
          int segundaAspas = linhaAtual.indexOf('"', primeiraAspas + 1);
          if (primeiraAspas != -1 && segundaAspas != -1) {
            redesEncontradas[totalRedes] = linhaAtual.substring(primeiraAspas + 1, segundaAspas);
            totalRedes++;
          }
        }
        linhaAtual = ""; 
      } else {
        if (linhaAtual.length() < 100) linhaAtual += c; 
      }
    }
  }
}

void setup() {
  delay(500); 
  
  Serial.begin(9600);
  esp8266.begin(9600); 
  lcd.begin(16, 2);
  pinMode(BOTAO_PIN, INPUT_PULLUP);

  lcd.createChar(0, iconePeso);
  lcd.createChar(1, iconeWifi);

  // Abertura Patinhas Smart
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(0)); 
  lcd.print(" Patinhas Smart "); 
  lcd.write(byte(0)); 
  lcd.setCursor(0, 1);
  lcd.print("    v1.0 IoT    ");
  delay(2500); 

  // Inicia Sensores com Aviso Visual
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  if (scale.is_ready()) {
    scale.set_scale(FATOR_CALIBRACAO);
    lcd.clear();
    lcd.print("Calibrando Base");
    lcd.setCursor(0, 1);
    lcd.print("Nao toque...");
    delay(2000); 
    scale.tare(); 
  }

  // Conexão Wi-Fi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(1)); 
  lcd.print(" Conectando... ");
  lcd.setCursor(0, 1);
  lcd.print(SSID); 

  enviarComandoAT("AT+CWMODE=1", 2000, true);
  delay(1000); 
  enviarComandoAT("AT+CWQAP", 1000, true); 
  delay(1000); 

  String cmdConexao = "AT+CWJAP=\"" + SSID + "\",\"" + PASSWORD + "\"";
  enviarComandoAT(cmdConexao, 15000, true);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(1));
  lcd.print(" WiFi Online!  ");
  delay(2000);
  lcd.clear();
}

void loop() {
  // 1. LEITOR DO BOTÃO
  bool leituraBotao = digitalRead(BOTAO_PIN);
  if (leituraBotao == LOW && ultimoEstadoBotao == HIGH) {
    modoScanner = !modoScanner; 
    delay(200); 

    if (modoScanner == true) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write(byte(1));
      lcd.print(" Buscando Redes");
      lcd.setCursor(0, 1);
      lcd.print("Aguarde 10s...");
      
      buscarESalvarRedes(); 
      indiceRedeLCD = 0; 
      tempoUltimaTrocaLCD = millis(); 
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Retornando a");
      lcd.setCursor(0, 1);
      lcd.print("Balanca...");
      delay(1500);
      lcd.clear();
    }
  }
  ultimoEstadoBotao = leituraBotao;

  unsigned long tempoAtual = millis(); 

  // 2. MODO SCANNER
  if (modoScanner == true) {
    if (totalRedes == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Nenhuma rede    ");
      lcd.setCursor(0, 1);
      lcd.print("encontrada.     ");
    } else {
      if (tempoAtual - tempoUltimaTrocaLCD >= 2000) {
        tempoUltimaTrocaLCD = tempoAtual;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.write(byte(1)); 
        lcd.print(" Rede " + String(indiceRedeLCD + 1) + "/" + String(totalRedes));
        lcd.setCursor(0, 1);
        
        String nomeParaMostrar = redesEncontradas[indiceRedeLCD];
        if (nomeParaMostrar.length() > 16) nomeParaMostrar = nomeParaMostrar.substring(0, 16);
        lcd.print(nomeParaMostrar);
        
        indiceRedeLCD++;
        if (indiceRedeLCD >= totalRedes) indiceRedeLCD = 0; 
      }
    }
  } 
  
  // 3. MODO BALANÇA (MONITORAMENTO)
  else {
    while (esp8266.available()) esp8266.read(); 

    if (scale.is_ready()) {
      pesoAtual = scale.get_units(5);
      if (pesoAtual >= -10.0 && pesoAtual <= 10.0) {
        pesoAtual = 0.0;
      }
    }

    // TAREFA A: Atualiza Visor
    if (tempoAtual - tempoUltimaLeituraLCD >= INTERVALO_LEITURA_LCD) {
      tempoUltimaLeituraLCD = tempoAtual; 
      lcd.setCursor(0, 0);
      lcd.write(byte(0)); 
      lcd.print(" Monitorando:  ");
      lcd.setCursor(0, 1);
      lcd.print("Peso: " + String(pesoAtual, 1) + " g    ");
    }

    // TAREFA B: Gatilho de Alerta Crítico
    if (pesoAtual <= LIMITE_ALERTA_GRAMAS && alertaEnviado == false) {
      dispararParaAPI("alerta", pesoAtual);
      alertaEnviado = true; 
    } 
    else if (pesoAtual > (LIMITE_ALERTA_GRAMAS + 50.0)) {
      alertaEnviado = false; 
    }

    // TAREFA C: Envio de Histórico 
    if (tempoAtual - tempoUltimoEnvioHistorico >= INTERVALO_ENVIO_HISTORICO) {
      tempoUltimoEnvioHistorico = tempoAtual; 
      dispararParaAPI("historico", pesoAtual);
    }

    // TAREFA D: Sincronização em "Tempo Real" 
    if (tempoAtual - tempoUltimoEnvioTempoReal >= INTERVALO_TEMPO_REAL) {
      tempoUltimoEnvioTempoReal = tempoAtual; 
      dispararParaAPI("status_atual", pesoAtual);
    }
  }
}
