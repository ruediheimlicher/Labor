#ifndef _TWISLAVE_H
#define _TWISLAVE_H

#include <util/twi.h> //enth�lt z.B. die Bezeichnungen f�r die Statuscodes in TWSR
#include <avr/interrupt.h> //dient zur behandlung der Interrupts
#include <stdint.h> //definiert den Datentyp uint8_t
#ifndef _ADC_H
#define _ADC_H


//#ifndef _LCD_H
//#define _LCD_H

//#include "adc.c"
//#include <avr/wdt.h>
/*
Dieses Programm in einer separaten Datei (z.B. twislave.c) abspeichern und in das eigene Programm
einbinden.

Betrieb eines AVRs mit Hardware-TWI-Schnittstelle als Slave. Zu Beginn muss init_twi_slave mit der gew�nschten
Slave-Adresse als Parameter aufgerufen werden. Der Datenaustausch mit dem Master erfolgt �ber die Buffer 
rxbuffer und txbuffer, auf die von Master und Slave zugegriffen werden kann. 
rxbuffer und txbuffer sind globale Variablen (Array aus uint8_t). 
Die Ansteuerung des rxbuffers, in den der Master schreiben kann, erfolgt �hnlich wie bei einem normalen I2C-EEPROM.
Man sendet zun�chst die Bufferposition, an die man schreiben will, und dann die Daten. Die Bufferposition wird 
automatisch hochgez�hlt, sodass man mehrere Datenbytes hintereinander schreiben kann, ohne jedesmal
die Bufferadresse zu schreiben.
Um den txbuffer vom Master aus zu lesen, �bertr�gt man zun�chst in einem Schreibzugriff die gew�nschte Bufferposition und
liest dann nach einem repeated start die Daten aus. Die Bufferposition wird automatisch hochgez�hlt, sodass man mehrere
Datenbytes hintereinander lesen kann, ohne jedesmal die Bufferposition zu schreiben.

Autor: Uwe Gro�e-Wortmann (uwegw)
Status: Testphase, keine Garantie f�r ordnungsgem��e Funktion! 
letze �nderungen: 
23.03.07 Makros f�r TWCR eingef�gt. Abbruch des Sendens, wenn der TXbuffer komplett gesendet wurde. 
24.03.07 verbotene Buffergr��en abgefangen
25.03.07 n�tige externe Bibliotheken eingebunden


Abgefangene Fehlbedienung durch den Master:
- Lesen �ber die Grenze des txbuffers hinaus
- Schreiben �ber die Grenzen des rxbuffers hinaus
- Angabe einer ung�ltigen Schreib/Lese-Adresse
- Lesezuggriff, ohne vorher Leseadresse geschrieben zu haben


 */ 
 



//%%%%%%%% von Benutzer konfigurierbare Einstellungen %%%%%%%%

#define buffer_size 8			//Gr��e der Buffer in Byte (2..128)
#define maxBuffer_Size 128		//Gr�ssere Zahlen sind Befehle des Masters an den Slave
//

//%%%%%%%% Globale Variablen, die vom Hauptprogramm genutzt werden %%%%%%%%

/*Der Buffer, in dem die empfangenen Daten gespeichert werden. Der Slave funktioniert �hnlich  wie ein normales
 Speicher-IC [I2C-EEPROM], man sendet die Adresse, an die man schreiben will, dann die Daten, die interne Speicher-Adresse
 wird dabei automatisch hochgez�hlt*/
volatile uint8_t rxbuffer[buffer_size];

//	Markierung von eingegangenen Bits
volatile uint8_t rxdata=0;
/*Der Sendebuffer, der vom Master ausgelesen werden kann.*/
extern volatile uint8_t txbuffer[buffer_size];

/*Datenbuffer fuer Werte vom ADC*/
volatile uint16_t adcbuffer[4];

//%%%%%%%% Funktionen, die vom Hauptprogramm aufgerufen werden k�nnen %%%%%%%%
 
/*Initaliserung des TWI-Inteface. Muss zu Beginn aufgerufen werden, sowie bei einem Wechsel der Slave Adresse
Parameter: adr: gew�nschte Slave-Adresse*/
void init_twi_slave (uint8_t adr);
extern uint16_t readKanalOrig(uint8_t derKanal, uint8_t num);
extern uint16_t readKanal(uint8_t derKanal);
void twidelay_ms(unsigned int ms);

extern void lcd_cls(void);
extern void lcd_putint(uint8_t zahl);
extern void lcd_gotoxy(uint8_t x,uint8_t y);
void lcd_puts(const char *s);
void lcd_putint1(uint8_t zahl);
void lcd_clr_line(uint8_t zahl);
void lcd_puthex(uint8_t zahl);

uint8_t lcd_delay=1;
extern volatile uint8_t TWI_Pause;
extern volatile uint8_t twi;
//%%%%%%%% ab hier sind normalerweise keine weiteren �nderungen erforderlich! %%%%%%%%//
//____________________________________________________________________________________//

#include <util/twi.h> //enth�lt z.B. die Bezeichnungen f�r die Statuscodes in TWSR


//Bei zu alten AVR-GCC-Versionen werden die Interrupts anders genutzt, daher in diesem Fall mit Fehlermeldung abbrechen
#if (__GNUC__ * 100 + __GNUC_MINOR__) < 304
	#error "This library requires AVR-GCC 3.4.5 or later, update to newer AVR-GCC compiler !"
#endif

//Schutz vor unsinnigen Buffergr��en
#if (buffer_size > maxBuffer_Size)
	#error Buffer zu gro� gew�hlt! Maximal 254 Bytes erlaubt.
#endif

#if (buffer_size < 2)
	#error Buffer muss mindestens zwei Byte gro� sein!
#endif


void BlinkD3(uint8_t anz)
{
	uint8_t k=0;
	for (k=0;k<anz;k++)
	{
		PORTD |=(1<<PD3);
		twidelay_ms(50);
		PORTD &=~(1<<PD3);
		twidelay_ms(100);
		
	}
	PORTD &=~(1<<PD3);
}


void twidelay_ms(unsigned int ms)
/* delay for a minimum of <ms> */
{
	// we use a calibrated macro. This is more
	// accurate and not so much compiler dependent
	// as self made code.
	while(ms){
		_delay_ms(0.96);
		ms--;
	}
}


volatile uint8_t buffer_adr; //"Adressregister" f�r den Buffer

/*Initaliserung des TWI-Inteface. Muss zu Beginn aufgerufen werden, sowie bei einem Wechsel der Slave Adresse
Parameter: adr: gew�nschte Slave-Adresse
*/
void init_twi_slave (uint8_t adr)
{
	TWAR= adr; //Adresse setzen
	TWCR &= ~(1<<TWSTA)|(1<<TWSTO);
	TWCR |= (1<<TWEA) | (1<<TWEN)|(1<<TWIE); 	
	buffer_adr=0xFF;  
	sei();
	
	//tybuffer mit Werten f�llen, die der Master auslesen soll
	for(uint8_t ii=0;ii<buffer_size;ii++)
	{
		txbuffer[ii]=0x00;
	}
	
	/*
	txbuffer[0]=0x01;
	txbuffer[1]=0x03;
	txbuffer[2]=0x07;
	txbuffer[3]=0x0F;
	txbuffer[4]=0x80;
	txbuffer[5]=0xC0;
	txbuffer[6]=0xE0;
	txbuffer[7]=0xF0;
	*/
	for(uint8_t ii=0;ii<4;ii++)
	{
		adcbuffer[ii]=0x00;
	}

}


//Je nach Statuscode in TWSR m�ssen verschiedene Bitmuster in TWCR geschrieben werden(siehe Tabellen im Datenblatt!). 
//Makros f�r die verwendeten Bitmuster:

//ACK nach empfangenen Daten senden/ ACK nach gesendeten Daten erwarten
#define TWCR_ACK TWCR = (1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC);  
//NACK nach empfangenen Daten senden/ NACK nach gesendeten Daten erwarten     
#define TWCR_NACK TWCR = (1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC);
//switched to the non adressed slave mode...
#define TWCR_RESET TWCR = (1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC);  

//Die Bitmuster f�r TWCR_ACK und TWCR_RESET sind gleich. Dies ist kein Fehler und dient nur der �bersicht!


/*ISR, die bei einem Ereignis auf dem Bus ausgel�st wird. Im Register TWSR befindet sich dann 
ein Statuscode, anhand dessen die Situation festgestellt werden kann.
*/
ISR (TWI_vect)  
{
	wdt_reset();
	uint8_t data=0;
	//		BlinkD3(2);
	lcd_cls();
	lcd_puts("ISR\0");
	//lcd_puthex(TW_STATUS);
	//twidelay_ms(lcd_delay);
	TWI_Pause=0; // Servo ausschalten
	rxdata=1;
	switch (TW_STATUS) //TWI-Statusregister pr�fen und n�tige Aktion bestimmen 
	{
			
		case TW_SR_SLA_ACK: // 0x60 Slave Receiver, wurde adressiert	
			//		lcd_cls();
			//		lcd_puts("TWI-GO\0");
			//		twidelay_ms(lcd_delay);
			//		lcd_cls();
			//		lcd_gotoxy(0,0);
			//		lcd_puts("R \0");
			twi |=(1<<0);
			TWCR_ACK; // n�chstes Datenbyte empfangen, ACK danach
			
			//		buffer_adr=0xFF; //Bufferposition ist undefiniert
			
			break;
			
		case TW_SR_DATA_ACK: // 0x80 Slave Receiver, Daten empfangen
			data=TWDR; //Empfangene Daten auslesen
			twi |=(1<<1);
			if (buffer_adr == 0xFF) //erster Zugriff, Bufferposition setzen
			{
				//			lcd_gotoxy(0,1);
				//			lcd_puts("First \0");
				//			lcd_putint(buffer_adr);
				
				//			twidelay_ms(lcd_delay);
				
				//Kontrolle ob gew�nschte Adresse im erlaubten bereich
				if(data<buffer_size)
				{
					buffer_adr= data; //Bufferposition wie adressiert setzen
				}
				else
				{
					buffer_adr=0; //Adresse auf Null setzen. Ist das sinnvoll?
				}				
				TWCR_ACK;	// n�chstes Datenbyte empfangen, ACK danach, um n�chstes Byte anzufordern
			}
			else //weiterer Zugriff, Daten empfangen
			{
				if (buffer_adr==0)
				{
					//lcd_cls();
					
				}
				/*
				 lcd_gotoxy(2,0);
				 lcd_puts("w adr: \0");
				 
				 lcd_gotoxy(8,0);
				 lcd_putint(buffer_adr);
				 lcd_putc(' ');
				 lcd_putint(data);
				 //				twidelay_ms(lcd_delay);
				 
				 
				 */			
				rxbuffer[buffer_adr]=data;	//	Daten in Buffer schreiben
				//			lcd_gotoxy(0,0);
				//			lcd_puts("Bytes: \0");
				//			lcd_gotoxy(8,0);
				//			lcd_putint(buffer_adr);
				//			delay_ms(2000);
				//rxdata |= (1<<buffer_adr);	//	Bit markieren
				rxdata=1;
				/*
				 lcd_gotoxy(0,1);
				 lcd_puts("rxbuf: \0");
				 lcd_putint(rxbuffer[buffer_adr]);
				 twidelay_ms(lcd_delay);
				 */			
				buffer_adr++;				//	Buffer-Adresse weiterz�hlen f�r n�chsten Schreibzugriff
				if(buffer_adr<(buffer_size)) //im Buffer ist noch Platz f�r mehr als ein Byte
				{
					//				lcd_gotoxy(16,0);
					//				lcd_puts("ACK \0");
					//				twidelay_ms(lcd_delay);
					
					TWCR_ACK;// n�chstes Datenbyte empfangen, ACK danach, um n�chstes Byte anzufordern
				}
				else   //es kann nur noch ein Byte kommen, dann ist der Buffer voll
				{
					//				lcd_gotoxy(7,0);
					//				lcd_putint(buffer_adr);
					//				lcd_gotoxy(16,0);
					//				lcd_puts("NACK\0");
					//				lcd_gotoxy(0,1);
					//				lcd_puts("B0: \0");
					//				lcd_putint(rxbuffer[0]);
					//				lcd_puts(" \0");
					//				lcd_puts("B1: \0");
					//				lcd_putint(rxbuffer[1]);
					//				lcd_puts(" \0");				
					//				twidelay_ms(lcd_delay);
					buffer_adr = 0xFF;		// buffer_adr wieder als undefiniert setzen
					TWCR_NACK;//letztes Byte lesen, dann NACK, um vollen Buffer zu signaliseren
					rxdata=1;
				}
			}
			break;
			
			case TW_ST_SLA_ACK: //?!?
			case TW_ST_DATA_ACK: //0xB8 Slave Transmitter, weitere Daten wurden angefordert
			//		lcd_gotoxy(0,0);
			//		lcd_puts("W\0");
			//		twidelay_ms(lcd_delay);
			twi |=(1<<2);
			if (buffer_adr == 0xFF) //zuvor keine Leseadresse angegeben! 
			{
				
				buffer_adr=0;
			}	
			//			lcd_gotoxy(3,0);
			//		lcd_puts("adr: \0");
			//		lcd_putint(buffer_adr);
			//		lcd_putc(' ');
			//		lcd_putint(txbuffer[buffer_adr]);
			
			TWDR = txbuffer[buffer_adr]; //Datenbyte senden 
			buffer_adr++; //bufferadresse f�r n�chstes Byte weiterz�hlen
			if(buffer_adr<(buffer_size-1)) //im Buffer ist mehr als ein Byte, das gesendet werden kann
			{
				twi |=(1<<3);
				TWCR_ACK; //n�chstes Byte senden, danach ACK erwarten
			}
			else
			{
				twi |=(1<<4);
				//				lcd_gotoxy(16,0);
				//				lcd_puts("NACK\0");
				
				TWCR_NACK; //letztes Byte senden, danach NACK erwarten
				buffer_adr=0xFF; //Bufferposition ist undefiniert
			}
			break;
			
			case TW_ST_DATA_NACK: //0xC0 Keine Daten mehr gefordert 
			
			
			
			case TW_SR_DATA_NACK: //0x88 
			case TW_ST_LAST_DATA: //0xC8  Last data byte in TWDR has been transmitted (TWEA = �0�); ACK has been received
			case TW_SR_STOP: // 0xA0 STOP empfangen
			default: 	
			TWCR_RESET; //�bertragung beenden, warten bis zur n�chsten Adressierung
			//		buffer_adr=0xFF; //Bufferposition ist undefiniert
			TWI_Pause=1;
			break;
			
			
	} //end.switch (TW_STATUS)
} //end.ISR(TWI_vect)

#endif
#endif //#ifdef _TWISLAVE_H

//#endif //ifdef _LCD_H
////Ende von twislave.c////

