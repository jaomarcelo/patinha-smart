#include <Arduino.h>
#include <LiquidCrystal.h>
#include <HX711.h>
#include <SoftwareSerial.h>

// --- CONFIGURAÇÕES DA BALANÇA ---
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;
const float FATOR_CALIBRACAO = 691.43;

// --- CONFIGURAÇÕES DO WI-FI E API ---
String SSID = "motog54"; 
String PASSWORD = "caio12345"; 
String SERVER = "api.seusite.com.br"; 
String ENDPOINT = "/rota-da-sua-api";
String PORTA = "80"; 

// --- PINOS E COMPONENTES ---
const int BOTAO_PIN = 6;
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);
HX711 scale;
SoftwareSerial esp8266(2, 3); 

// --- DESIGN DOS ÍCONES CUSTOMIZADOS ---
byte iconePeso[8] = { B00100, B01110, B01110, B11111, B11111, B11111, B11111, B00000 };
byte iconeWifi[8] = { B00000, B11111, B00001, B01110, B00100, B00000, B00100, B00000 };

// --- CONTROLE DE TAREFAS (BALANÇA E HISTÓRICO) ---
unsigned long tempoUltimaLeitura = 0;
const unsigned long INTERVALO_LEITURA = 5000; 
float pesoAtual = 0.0; 

// NOVO: Configurações do Dashboard de Ração
unsigned long tempoUltimoEnvio = 0;
const unsigned long INTERVALO_ENVIO_HISTORICO = 1800000; // 30 Minutos (em milissegundos)
const float LIMITE_ALERTA_GRAMAS = 500.0; // Abaixo de 500g, dispara o aviso pro App
bool alertaEnviado = false; // Trava para não floodar a API de alertas

// --- VARIÁVEIS DO SCANNER DE WI-FI NO LCD ---
bool modoScanner = false;
bool ultimoEstadoBotao = HIGH; 
String redesEncontradas[10]; 
int totalRedes = 0;
unsigned long tempoUltimaTrocaLCD = 0;
int indiceRedeLCD = 0;

// --- FUNÇÃO AUXILIAR DE COMANDO ---
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

// --- RADAR ---
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
  delay(3500); 

  // Carregamento
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  for(int i = 0; i < 16; i++) {
    lcd.setCursor(i, 1);
    lcd.print(".");
    delay(150); 
  }

  // Inicia Sensores
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(FATOR_CALIBRACAO);
  scale.tare();

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
  // LER O BOTÃO DE MODOS
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
      tempoUltimaTrocaLCD = 0; 
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

 
  // MODO SCANNER
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
  // MODO BALANÇA (MONITORAMENTO)
  else {
    while (esp8266.available()) esp8266.read(); 

    // TAREFA 1: Atualiza Visor LCD (A cada 5 segundos)
    if (tempoAtual - tempoUltimaLeitura >= INTERVALO_LEITURA) {
      tempoUltimaLeitura = tempoAtual; 
      pesoAtual = scale.get_units(20);
      if (pesoAtual < 0.2 && pesoAtual > -0.2) pesoAtual = 0; 

      lcd.setCursor(0, 0);
      lcd.write(byte(0)); 
      lcd.print(" Monitorando:  ");
      lcd.setCursor(0, 1);
      lcd.print("Peso: " + String(pesoAtual, 2) + " g   ");
    }

    // TAREFA 2: Gatilho de Alerta (O Pote Esvaziou!)
    if (pesoAtual <= LIMITE_ALERTA_GRAMAS && alertaEnviado == false) {
      // Dispara IMEDIATAMENTE furando o cronômetro
      dispararParaAPI("alerta", pesoAtual);
      alertaEnviado = true; // Tranca a porta para não floodar a API
    } 
    // Se você encheu o pote de novo (com uma margem de folga de 50g), destranca o aviso
    else if (pesoAtual > (LIMITE_ALERTA_GRAMAS + 50.0)) {
      alertaEnviado = false; 
    }

    // TAREFA 3: Envio de Histórico Padrão (A cada 30 minutos)
    if (tempoAtual - tempoUltimoEnvio >= INTERVALO_ENVIO_HISTORICO) {
      tempoUltimoEnvio = tempoAtual; 
      dispararParaAPI("historico", pesoAtual);
    }
  }
}