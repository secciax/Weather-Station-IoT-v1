# 🌤️ IoT Weather Station (BLE, WiFi & MQTT)

*Prototipo di stazione meteo IoT per la raccolta e l'analisi dei dati atmosferici.*

## 📋 Descrizione del Progetto
Questo progetto implementa una stazione meteo divisa in due moduli basati su architettura Arduino. I due nodi comunicano a basso consumo tramite protocollo **Bluetooth Low Energy (BLE)**. I dati vengono poi instradati verso un broker **MQTT** (ThingSpeak) per il monitoraggio remoto.

Il progetto rappresenta la "Fase 1" di una tesi di laurea in Ingegneria Informatica, con l'obiettivo futuro di integrare connettività LoRaWAN e modelli di Machine Learning (Edge AI) per previsioni meteorologiche locali.

---

## 🏗️ Architettura di Sistema

Il sistema è composto da due nodi principali:

### 1. Modulo Esterno (Sensor Node / BLE Peripheral)
Posizionato all'esterno, si occupa dell'acquisizione dei dati fisici.
* **Hardware**: Arduino Nano 33 BLE Sense Rev2
* **Sensori**: Temperatura, Umidità, Pressione (integrati on-board)
* **Logica Operativa**: 
  1. Si sveglia dal Deep Sleep.
  2. Acquisisce i dati.
  3. Aggiorna le caratteristiche del servizio BLE `servizioMeteo` (modalità NOTIFY).
  4. Torna in risparmio energetico disattivando la CPU.

### 2. Modulo Interno (Gateway / BLE Central)
Posizionato all'interno, funge da ponte tra la rete PAN (BLE) e la rete WAN (WiFi/MQTT).
* **Hardware**: Arduino Nano RP2040 Connect
* **Display**: Schermo LCD 16x2 (I2C)
* **Logica Operativa**:
  1. Scansiona l'aria in attesa del pacchetto di advertising.
  2. Si iscrive al servizio di NOTIFY del modulo esterno.
  3. Alla ricezione dei dati (ogni 10 minuti), aggiorna il display LCD locale.
  4. Pubblica il payload sul broker MQTT (ThingSpeak).

---

## ⚙️ Installazione e Configurazione

Per replicare questo progetto, clona il repository e configura le credenziali di rete:

1. Apri la cartella `Modulo_Interno`.
2. Troverai un file chiamato `secrets_template.h`.
3. Rinominalo in `secrets.h`.
4. Inserisci all'interno i tuoi dati reali (SSID WiFi, Password, e credenziali MQTT):

```cpp
#define SECRET_SSID "IL_TUO_WIFI"
#define SECRET_PASS "LA_TUA_PASSWORD"
#define SECRET_MQTT_CLIENT_ID "IL_TUO_CLIENT_ID"
// ...
