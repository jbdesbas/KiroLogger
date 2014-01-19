#include <DS1302.h> //Bibliothque pour l'horloge
#include <Time.h>

#include <SD.h>
File fichier;

#include <OneWire.h>
#include <DallasTemperature.h> //Pour la lecture du thermomètre numerique


#define ONE_WIRE_BUS 5 //Thermo 

//Définition des broches
#define PIN_SS 10 //Pour la carte SD (pin SS), (au choix, ou 4 si shield)
#define LED_TEMOIN 2 //Rouge ou verte
//Pour l'horloge
#define RTC_RST 7 //CE(RST)
#define RTC_IO 8 //IO (DAT)
#define RTC_SCLK 9 //SCLK (CLK)

#define PIN_LASER 6

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

String date_str;
String time_str ;
String temporaire_str ;

int heure ;
int _minute;
int annee;
int mois;
int jour;

int val1;
int val2;
int val3;
int val4;
int time1 = 0;
int time2 = 0;
Time t;

bool mode = false ; //Les lasers sont arrétés au départ

//*************PARAMETRES **********/
float const lat = 32.75;
int const seuil_lumiere = 200; //Luminosité des diode "au repos"
int const seuil_temps = 500; //L'intervalle maxi entre la coupure des rangées 1 et 2
int const heure_start1 = 10 ; //Heure de départ
int const minute_start1 = 45 ; 
int const heure_stop1 = 7 ; //Heure de stop
int const minute_stop1 = 0 ;
int const IntervalleTempe = 60 ; //Intervalle de prise de température (minutes)
//***********************************/

unsigned long time_transmission; //timer pour l'envoi sur le port serie
unsigned long time_writeSD; //timer pour ecriture sur la carte SD
unsigned long time_releveDate;

DS1302 rtc(RTC_RST,RTC_IO,RTC_SCLK); //Selection des pin de l'horloge CE (RST), IO (DAT?) , SCLK (CLK)

   
void setup()
{
	Serial.begin(9600);
	  
	pinMode(PIN_SS, OUTPUT); //Le pin SS toujours en output
	pinMode(PIN_LASER, OUTPUT);
	pinMode(LED_TEMOIN, OUTPUT);
		
	digitalWrite(LED_TEMOIN,LOW);
	
	delay(100);
	while (!SD.begin(PIN_SS)) { //Si pas de carte SD on reste bloqué sur cette boucle en attendant qu'elle soit branchées
		digitalWrite(LED_TEMOIN,HIGH);
		delay(1000);
		digitalWrite(LED_TEMOIN,LOW);
		delay(500);
	}

	sensors.begin(); //Capteur temperature
	
	
	//t=rtc.getTime();
    //setTime(t.hour,t.min,t.sec,t.date,t.mon,t.year); a déplacer dans la loop
	
	for(int i=0;i<=8;i++) //La LED verte clignote pour dire que tout est OK
	{
		digitalWrite(LED_TEMOIN,HIGH);	
		delay(200);
		digitalWrite(LED_TEMOIN,LOW);
		delay(200);
	}
	write_log("Mise en route");
}

void loop()
{
	
	if(millis()-time_releveDate > 30000){//peut probablement être optimiser et voir pour faire un tableau
		date_str = rtc.getDateStr();
			temporaire_str = date_str.substring(0,2);
				jour = temporaire_str.toInt();
			temporaire_str = date_str.substring(3,5);
				mois = temporaire_str.toInt();
			temporaire_str = date_str.substring(6,10);
				annee = temporaire_str.toInt();
		time_str = rtc.getTimeStr();
			temporaire_str = time_str.substring(0,2);
				heure = temporaire_str.toInt();
			temporaire_str = time_str.substring(3,5);
				_minute = temporaire_str.toInt();
				
		
				
		time_releveDate=millis();
	}

	
	if(heure == heure_start1 && _minute==minute_start1 && mode!= true) //Heure de départ
	{
		mode = true;
		write_log("Laser ON");
	}
	else if(heure == heure_stop1 && _minute==minute_stop1 && mode!=false) //Heure de stop
	{
		mode = false;
		write_log("Laser OFF");
	}
	mode = true ; //POUR TESTE A VIRER ENSUITE
	
	if(millis()-time_writeSD >IntervalleTempe*60000) //Ecriture de la température sur la carte SD (selon le pas de temps defini dans les paramètres)
	{
		// Inscription des relevés dans la carde SD
		fichier = SD.open("temp.txt", FILE_WRITE);
		fichier.print(heure); //Date
		fichier.print("\t");
		fichier.print(rtc.getTimeStr()); //Heure
		fichier.print("\t");
		sensors.requestTemperatures(); //On interoge le thermo
		fichier.println(sensors.getTempCByIndex(0)); //On ecrit la température et on passe a la ligne
		fichier.close();
	
		time_writeSD=millis(); 
	}
	if(mode == true) //Si la barrière est activée
	{
		digitalWrite(LED_TEMOIN,HIGH);
		digitalWrite(PIN_LASER, HIGH); //Laser ON
		//Lecture des photodiodes (A0 à A1) A OPTIMISER c'est moche
		val1 = analogRead(0);
		val2 = analogRead(1);
		val3 = analogRead(2);
		val4 = analogRead(3);	

		if(millis()-time_transmission >2000) //On affiche les variables sur le terminal chaque 2 seconde
		{	
			Serial.println(now());
				
			time_transmission=millis(); 
		}
	  
		if(val1 < seuil_lumiere || val2 < seuil_lumiere ){ //Le faisceau 1 ou 2 est coupé (rangée 1)
			time1=millis(); //On note le timestamp où le faisceau est coupé
		}
		
		if(val3 < seuil_lumiere || val4 < seuil_lumiere){ //Le faisceau 3 ou 4 est coupé (rangée 2)
			time2=millis(); //On note le timestamp où le faisceau est coupé
		}
		
		if(abs(time1-time2)<seuil_temps){ //Si les deux rangées sont coupés en mois de Xms (seuil temps) d'écart
			if((time1-time2)>0) //Passage dans le sens sortant
			{
				write_log("Entre");
				delay(500);
			}
			else if((time1-time2)<0) //Passage dans le sens entrant
			{
				write_log("Sortie");
				delay(500);
			}
				time1=0; //On remet les timestamps a zero
				time2=0;
		}
		
	}
	else //En attente, les LED clignotes ensemble
	{
		digitalWrite(PIN_LASER, LOW); //laser OFF
		digitalWrite(LED_TEMOIN,HIGH);
		delay(500);
		digitalWrite(LED_TEMOIN,LOW);
		delay(500);
	}
		
}
 
void write_log(char* message) //regrouper les 2 fonctions ?
{
	fichier = SD.open("log.txt", FILE_WRITE);
	fichier.print(rtc.getDateStr()); //Date
	fichier.print("\t");
	fichier.print(rtc.getTimeStr()); //Heure
	fichier.print("\t");
	fichier.println(message);
	fichier.close();

}

int heure_minute_coucher_lever(int jour, int mois, int annee, float lat, bool l_c, bool h_m) //TRUE pour lever et heure, false pour coucher et minute
{
    float heuredec;
	int out;
	int RangJour = int(mois*275/9)-int((mois+9)/12)*(1+int((annee-4*int(annee/4)+2)/3))+jour-30;
	//int M = (357+RangJour)%360;
	//int C = 1.914*sin(3.14/180*M)+0.02*sin(3.14/180*2*M);
	int L = (280+RangJour)%360; //int L = (280+C+RangJour)%360;
	float declin = asin(0.3978*sin(3.14/180*L))*180/3.14;
	float Ho = (acos((-0.01454-sin(3.14/180*declin)*sin(3.14/180*lat))/(cos(3.14/180*declin)*cos(3.14/180*lat)))*180/3.14)/15;
	
	if(l_c==true)
	{
		heuredec = 12-Ho + 1; //Car GMT+1
	}
	else
	{
		heuredec = 12+Ho + 1; //Car GMT+1
	}
	if(h_m==true)
	{
		out = int(heuredec);
	}
	else
	{
		out = (heuredec - int(heuredec))*60;
	}
	return out;
}

