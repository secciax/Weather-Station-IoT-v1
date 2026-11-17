/*
Progetto di IoT: Stazione Meteo
Modulo Interno: Arduino Nano RP2040 Connect
Ruolo BLE: central

Il modulo interno si occupa di stabilire le connessioni principali (WiFi, MQTT).
Si connette al peripheral, ne scopre gli attributi e si iscrive ai servizi di NOTIFY da esso serviti.
Ogni volta che avviene il NOTIFY, e quindi i dati sono aggiornati (ogni 15 minuti), li invia al broker MQTT di ThingSpeak.
Inoltre è presente uno schermo LCD per mostrare eventuali notifiche e mostrare i dati non appena vengono aggiornati.

Federico Secciani
*/

#include <ArduinoBLE.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "secrets.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);              //inizializzazione dello schermo lcd (16x2) che comunica sulla linea 0x27 del bus I2C

//Stringhe di ssid e password per la connessione al wifi
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

const char* mqttServer = "mqtt3.thingspeak.com"; //server destinatario dei vari payload
const int mqttPort = 1883;                       //porta standard per la comunicazione MQTT (non criptata)
WiFiClient wifiClient;                           //inizializzazione dell'oggetto di classe WiFiClient che si occupa dell'invio dei pacchetti
PubSubClient mqttClient(wifiClient);             //inizializzazione dell'oggetto mqttClient che sfrutta l'oggetto wifiClient per l'invio

//chiavi alfanumeriche per accedere al servizio del Broker MQTT
const char* mqttClientID = SECRET_MQTT_CLIENT_ID;
const char* mqttUsername = SECRET_MQTT_USER;
const char* mqttPassword = SECRET_MQTT_PASS;

//ID del canale su ThingSpeak
unsigned long channelID = SECRET_ID_CHANNEL;

bool nuoviDatiPronti = false;

// Variabili globali per i dati e il BLE
BLEDevice peripheral;
float tempEsterna = 0.0;
float umidEsterna = 0.0;
float presEsterna = 0.0;

BLECharacteristic caratTemp;
BLECharacteristic caratUmid;
BLECharacteristic caratPres;

// Prototipi
void connettiWifi();
void connettiMQTT();
void controllaPacchettiBLE();
void tentaConnessioneBLE();
void gestisciStazioneEsterna();
void inviaDatiMQTT();
void aggiornaDisplayLCD();


void setup() {
  Serial.begin(9600);

  lcd.init();          
  lcd.backlight();     
  lcd.print("Avvio sistema...");

  //Effettua la connessione con il wifi
  connettiWifi();
  
  //Collega l'oggetto mqttClient al server e alla porta di ThingSpeak, e poi effettua la connessione
  mqttClient.setServer(mqttServer, mqttPort);
  connettiMQTT();
  
  Serial.println("Arduino Interno (Central) avviato.");

  if (!BLE.begin()) {
    Serial.println("Errore modulo BLE!");
    while (1);
  }

  Serial.println("Avvio scansione BLE continua in background...");
  lcd.clear();
  lcd.print("Cerca BLE...");
  //contralla se ci sono pacchetti di advertise nell'aria
  controllaPacchettiBLE();
}

void loop() {
  //mantenimento delle connessioni di rete
  if (WiFi.status() != WL_CONNECTED) {
    connettiWifi();
  }
  if (!mqttClient.connected()) {
    connettiMQTT();
  }
  
  //funzione KEEP-ALIVE della connessione MQTT, altrimenti si verificherebbe il timeout 
  mqttClient.loop();

  //se non siamo connessi chiama la funzione per ispezionare nuovamente l'aria, altrimenti chiama la funzione per il controllo dei dati in arrivo
  if (!peripheral || !peripheral.connected()) {
    controllaPacchettiBLE();
  } else {
    gestisciStazioneEsterna();
  }
  
  //se i dati sono stati aggiornati viene resettata la variabile nuoviDatiPronti e chiamate le funzioni per invio dati a MQTT e aggiornamento del display LCD
  if (nuoviDatiPronti) {
    nuoviDatiPronti = false;
    inviaDatiMQTT();
    aggiornaDisplayLCD();
  }

  delay(10); // Piccolo delay per non sovraccaricare la CPU
}

/*(RETE)Funzione per la conessione wifi
**E' presente un delay iniziale per dare tempo al modulo wifi di configurarsi
**Finchè non si connette effettua nuovi tentativi ogni 5 secondi*/
void connettiWifi() {
  delay(2000);
  Serial.print("Tentativo di connessione alla rete Wi-Fi: ");
  Serial.println(ssid);
  
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    Serial.print(".");
    delay(5000); 
  }
  
  Serial.println("\n[WI-FI] Connesso con successo!");
  Serial.print("[WI-FI] IP: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.print("WiFi connesso");
}

/*(RETE)Funzione per la conessione con il Broker MQTT di ThingSpeak
**Anche qui in caso di connessione fallita, riprova ogni 5 secondi*/
void connettiMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connessione MQTT ThingSpeak...");
    if (mqttClient.connect(mqttClientID, mqttUsername, mqttPassword)) {
      Serial.println(" OK!");
    } else {
      Serial.print(" Fallito. Riprovo tra 5 sec...");
      delay(5000);
    }
  }
}

/*(BLE)Funzione che attiva lo scan e legge tutti i peripheral disponibili.
**Se trova il peripheral di interesse salva i dati nella variabile peripheral e chiama la funzione per connertercisi.
**Prima di ciò, per migliorare la connettività viene fermato lo scan in modo che venga data la priorità alla connessione*/
void controllaPacchettiBLE() {

  BLE.scan();
  BLEDevice pacchettoInArrivo = BLE.available();
  Serial.println("Controllo i pacchetti...");
  if (pacchettoInArrivo && pacchettoInArrivo.localName() == "StazioneMeteo") {
    Serial.println("\n[BLE] Stazione esterna trovata nell'aria!");
    
    peripheral = pacchettoInArrivo; 
    BLE.stopScan();
    delay(200);
  
    tentaConnessioneBLE(); 
  }
}

/*(BLE)Funzione per connettersi al peripheral trovato nella funzione chiamante.
**Trova gli attributi e si iscrive al NOTIFY delle 3 caratteristiche
**In caso di esiti negativi si disconnette e torna alla funzione chiamante*/
void tentaConnessioneBLE() {
  Serial.println("Tentativo di aggancio...");

  if (peripheral.connect()) {
    Serial.println("Connesso con successo alla stazione esterna!");
  } else {
    Serial.println("Connessione fallita. Riproverò al prossimo segnale utile.");
    return;
  }

  delay(150); // Ritardo di stabilizzazione post-connessione

  Serial.println("Esplorazione degli attributi...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributi scoperti.");
  } else {
    Serial.println("Errore esplorazione. Disconnessione...");
    peripheral.disconnect();
    return;
  }

  caratTemp = peripheral.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");
  caratUmid = peripheral.characteristic("19B10002-E8F2-537E-4F6C-D104768A1214");
  caratPres = peripheral.characteristic("19B10003-E8F2-537E-4F6C-D104768A1214");

  if (!caratTemp || !caratUmid || !caratPres) {
    Serial.println("Errore: Caratteristiche mancanti.");
    peripheral.disconnect();
    return;
  }

  if (caratTemp.subscribe() && caratUmid.subscribe() && caratPres.subscribe()) {
    Serial.println("Iscrizione Notifiche BLE OK!");
    lcd.clear();
    lcd.print("BLE notify ON");
    lcd.setCursor(0 ,1);
    lcd.print("Attendo dati");
  } else {
    Serial.println("Errore iscrizione notifiche.");
    peripheral.disconnect();
  }
}

/*(BLE)Funzione per verificare se i dati delle caratteristiche sono stati aggiornati. In caso positivo porta la variabile nuoviDatiPronti = true e legge i dati*/
void gestisciStazioneEsterna() {
  if (caratTemp.valueUpdated() || caratUmid.valueUpdated() || caratPres.valueUpdated()) {
    nuoviDatiPronti = true;

    caratTemp.readValue(&tempEsterna, sizeof(tempEsterna));
    caratUmid.readValue(&umidEsterna, sizeof(umidEsterna));
    caratPres.readValue(&presEsterna, sizeof(presEsterna));
    
    Serial.println("\n[BLE] Nuovi Dati Ricevuti!");
  }
}


/*(OUTPUT)Funzione per la creazione e invio a MQTT dei payload contenenti i dati aggiornati della stazione meteo*/
void inviaDatiMQTT() {
  String payload = "field1=" + String(tempEsterna) + "&field2=" + String(umidEsterna) + "&field3=" + String(presEsterna);
  String topic = "channels/" + String(channelID) + "/publish";

  if (mqttClient.publish(topic.c_str(), payload.c_str())) {
    Serial.println("[MQTT OK] Dati inviati a ThingSpeak!");
  } else {
    Serial.println("[MQTT ERRORE] Impossibile pubblicare il messaggio.");
  }
}

/*(OUTPUT)Funzione per l'aggiornamento dei dati visibili sul dispaly LCD*/
void aggiornaDisplayLCD() {
  lcd.clear();
  lcd.setCursor(0, 0); 
  lcd.print("T:"); lcd.print(tempEsterna, 1); lcd.print("C ");
  lcd.print("U:"); lcd.print(umidEsterna, 0); lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("P:"); lcd.print(presEsterna, 1); lcd.print("hPa");
}