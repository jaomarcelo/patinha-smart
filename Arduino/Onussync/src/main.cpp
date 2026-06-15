#include <Arduino.h>
#include <LiquidCrystal.h>
#include <HX711.h>
#include <SoftwareSerial.h>

// --- O NOVO MAPA DE INFRAESTRUTURA FÍSICA ---
LiquidCrystal lcd(7, 6, 5, 4, 3, 2); 
const int LOADCELL_DOUT_PIN = 8;
const int LOADCELL_SCK_PIN = 9;
SoftwareSerial esp8266(10, 11); // RX no 10, TX no 11
const int BOTAO_PIN = 12;

const float FATOR_CALIBRACAO = 691.43;

// --- CONFIGURAÇÕES DO WI-FI E API ---
String SSID = "motog54"; 
String PASSWORD = "caio12345"; 
String SERVER = "api.seusite.com.br"; 
String ENDPOINT = "/rota-da-sua-api";
String PORTA = "80"; 

HX711 scale;

// --- DESIGN DOS ÍCONES CUSTOMIZADOS ---
byte iconePeso[8] = { B00100, B01110, B01110, B11111, B11111, B11111, B11111, B00000 };
byte iconeWifi[8] = { B00000, B11111, B00001, B01110, B00100, B00000, B00100, B00000 };

// --- CONTROLE DE TAREFAS (BALANÇA E HISTÓRICO) ---
unsigned long tempoUltimaLeitura = 0;
const unsigned long INTERVALO_LEITURA = 5000; 
float pesoAtual = 0.0; 

// --- CONFIGURAÇÕES DO DASHBOARD DE RAÇÃO ---
unsigned long tempoUltimoEnvio = 0;
const unsigned long INTERVALO_ENVIO_HISTORICO = 1800000; // 30 Minutos
const float LIMITE_ALERTA_GRAMAS = 500.0; // Abaixo de 500g, dispara alerta
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
  if (debug) Serial.print(resposta);
  return resposta;
}

// --- FUNÇÃO PARA MONTAR E ENVIAR O PACOTE HTTP ---
void dispararParaAPI(String tipoDeEvento, float pesoDaVez) {
  String json = "{\"tipo\": \"" + tipoDeEvento + "\", \"peso\": " + String(pesoDaVez) + "}";
  String requisicao = "POST " + ENDPOINT + " HTTP/1.1\r\n";
  requisicao += "Host: " + SERVER + "\r\n";
  requisicao += "Content-Type: application/json\r\n";
  requisicao += "Content-Length: " + String(json.length()) + "\r\n";
  requisicao += "Connection: close\r\n\r\n";
  requisicao += json;

  Serial.println("\n>>> DISPARANDO DADOS: " + tipoDeEvento + " <<<");
  enviarComandoAT("AT+CIPSTART=\"TCP\",\"" + SERVER + "\"," + PORTA, 4000, false);
  enviarComandoAT("AT+CIPSEND=" + String(requisicao.length()), 2000, false);
  enviarComandoAT(requisicao, 5000, false);
  enviarComandoAT("AT+CIPCLOSE", 1000, false);
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
  // A Proteção de Inicialização da Tela
  delay(500);
  
  Serial.begin(9600);
  esp8266.begin(9600); 
  lcd.begin(16, 2);
  pinMode(BOTAO_PIN, INPUT_PULLUP);

  lcd.createChar(0, iconePeso);
  lcd.createChar(1, iconeWifi);

  // Abertura OnusSync
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(0)); 
  lcd.print("   OnusSync   "); 
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
    delay(2000); // Dá tempo para tirar a mão da tábua
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
  // 1. O LEITOR DO BOTÃO (Alternância de Modo)
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
  // 3. MODO BALANÇA (MONITORAMENTO DA RAÇÃO E API)
  else {
    while (esp8266.available()) esp8266.read(); 

    // TAREFA A: Atualiza Visor LCD e lê peso
    if (tempoAtual - tempoUltimaLeitura >= INTERVALO_LEITURA) {
      tempoUltimaLeitura = tempoAtual; 
      
      if (scale.is_ready()) {
        pesoAtual = scale.get_units(10); // 10 leituras para maior estabilidade
        
        // O NOVO FILTRO DE ZONA MORTA (+/- 10g vira zero)
        if (pesoAtual >= -10.0 && pesoAtual <= 10.0) {
            pesoAtual = 0.0;
        }

        lcd.setCursor(0, 0);
        lcd.write(byte(0)); 
        lcd.print(" Monitorando:  ");
        lcd.setCursor(0, 1);
        lcd.print("Peso: " + String(pesoAtual, 1) + " g    ");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("Erro de Leitura ");
      }
    }

    // TAREFA B: Gatilho de Alerta Crítico (O Pote Esvaziou!)
    if (pesoAtual <= LIMITE_ALERTA_GRAMAS && alertaEnviado == false) {
      dispararParaAPI("alerta", pesoAtual);
      alertaEnviado = true; // Tranca para não sobrecarregar a API
    } 
    else if (pesoAtual > (LIMITE_ALERTA_GRAMAS + 50.0)) {
      alertaEnviado = false; // Destranca quando enche o pote
    }

    // TAREFA C: Envio de Histórico Padrão para Dashboard
    if (tempoAtual - tempoUltimoEnvio >= INTERVALO_ENVIO_HISTORICO) {
      tempoUltimoEnvio = tempoAtual; 
      dispararParaAPI("historico", pesoAtual);
    }
  }
}
