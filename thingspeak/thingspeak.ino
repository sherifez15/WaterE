// Simple demo for feeding some random data to Pachube.
// 2011-07-08 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// Handle returning code and reset ethernet module if needed
// 2013-10-22 hneiraf@gmail.com

// Modifing so that it works on my setup for www.thingspeak.com.
// Arduino pro-mini 5V/16MHz, ETH modul on SPI with CS on pin 10.
// Also added a few changes found on various forums. Do not know what the 
// res variable is for, tweaked it so it works faster for my application
// 2015-11-09 dani.lomajhenic@gmail.com
#include <PinChangeInt.h>
#include <EtherCard.h>

// change these settings to match your own setup
//#define FEED "000"
#define APIKEY "07A3EGOY67ASUMWC" // put your key here
#define ethCSpin 10 // put your CS/SS pin here.

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
const char website[] PROGMEM = "api.thingspeak.com";
byte Ethernet::buffer[700];
uint32_t timer;
Stash stash;
byte session;

int sensors[]={'flow','shutoff','open'};
volatile int pulses=0;
double current_Rate;
double previous_Rate;  
double sensitivity;
String message_out;
//timing variable
int res = 100; // was 0



void setup () {
  Serial.begin(9600);

  //Initialize Ethernet
  initialize_ethernet();
  sensors['flow']=5;
  pinMode(sensors['flow'],INPUT);
  volatile int pulses=0;
  double current_Rate;
  double previous_Rate;  
  sensitivity=0.15;
  previous_Rate=0; 
  String message_out;
  PCintPort::attachInterrupt(sensors['flow'], pulsecount,RISING);
}


void loop () { 
  int response = get_reading(); 
  PCintPort::detachInterrupt(sensors['flow']);
  Serial.println(current_Rate);
  if (response!=-4){
      Serial.print("flowRate: ");
      Serial.println(current_Rate); 
      ethersend_custom(current_Rate);
      delay(1000);
  }

}

void ethersend_custom(double reading){
  //if correct answer is not received then re-initialize ethernet module
for(res=180;res<=222;res++){
  if (res > 220){
    initialize_ethernet(); 
  }
  
  Serial.println("res is");
  Serial.println(res);
  ether.packetLoop(ether.packetReceive());
  
  //200 res = 10 seconds (50ms each res)
  if (res==200) {


    byte sd = stash.create();
    stash.print("field1=");
    stash.print(reading);
    stash.save();

    //Display data to be sent
    Serial.println(reading);


    // generate the header with payload - note that the stash size is used,
    // and that a "stash descriptor" is passed in as argument using "$H"
    Stash::prepare(PSTR("POST /update HTTP/1.0" "\r\n"
      "Host: $F" "\r\n"
      "Connection: close" "\r\n"
      "X-THINGSPEAKAPIKEY: $F" "\r\n"
      "Content-Type: application/x-www-form-urlencoded" "\r\n"
      "Content-Length: $D" "\r\n"
      "\r\n"
      "$H"),
    website, PSTR(APIKEY), stash.size(), sd);

    // send the packet - this also releases all stash buffers once done
    session = ether.tcpSend(); 

 // added from: http://jeelabs.net/boards/7/topics/2241
 int freeCount = stash.freeCount();
    if (freeCount <= 3) {   Stash::initMap(56); } 

    return;
  }
  
   const char* reply = ether.tcpReply(session);
   
   if (reply != 0) {
     res = 0;
     Serial.println(F(" >>>REPLY recieved...."));
     Serial.println(reply);
     return;
   }
   delay(300); 
 }
}


void initialize_ethernet(void){  
  for(;;){ // keep trying until you succeed 

    if (ether.begin(sizeof Ethernet::buffer, mymac, ethCSpin) == 0){ 
      Serial.println( "Failed to access Ethernet controller");
      continue;
    }

    if (!ether.dhcpSetup()){
      Serial.println("DHCP failed");
      continue;
    }

    ether.printIp("IP:  ", ether.myip);
    ether.printIp("GW:  ", ether.gwip);  
    ether.printIp("DNS: ", ether.dnsip);
   

    if (!ether.dnsLookup(website))
      Serial.println("DNS failed");

    ether.printIp("SRV: ", ether.hisip);

    //reset init value
    res = 180;
    
    break;
  }
}

double get_reading(){
  PCintPort::attachInterrupt(sensors['flow'], pulsecount,RISING);
  delay(1000);
  //PCintPort::detachInterrupt(sensors['flow']);
  //Serial.println("Reading will be taken");
  current_Rate = (pulses *60)/7.5;
  if(abs(((current_Rate-previous_Rate)/previous_Rate))>sensitivity){
      previous_Rate=current_Rate;
      pulses=0; //reset pulse
      Serial.println("Reading taken");
      return current_Rate;
    }
    else{
      pulses=0; //reset pulses
      return -4; // -4 instead of false because zero is one of the readings desired
      } 
}

void pulsecount(){
  //Serial.println("pulse");
  pulses++;
}
