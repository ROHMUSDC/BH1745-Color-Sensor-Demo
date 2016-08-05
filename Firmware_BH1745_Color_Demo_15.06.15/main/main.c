
//*****************************************************************************
// Program:	 ROHM Sensor Platform Firmware for Q112 Lapis Microcontroller
//		 ROHM Semiconductor USA, LLC
//		 US Design Center
// Started:  July 8th, 2014
// Purpose:	 Firmware for Q112 for GPS TRACKER 
// Updated:	 July 8th, 2014
//*****************************************************************************
//#define DebugSensor	16

// ============================= GPS TRACKER Board Specs ============================== 
//	UART to USB/PC:
//		UART to FTDI => B0, B1
//		VBus Det => A2
//	Sensor Interface:
//		I2C => B5, B6
//		ADC => A0, A1
//		GPIO => B2, B3, B4, B7
//	LED Feedback Section:
//		LEDs = C0 to C7
//	Sensor Control Section:
//		DIP Switch = D0 to D3

//***** PREPROCESSOR DIRECTIVES ***********************************************
// INCLUDED FILES...
// Include Path: common;main;irq;timer;clock;tbc;pwm;uart;

	#include	<ML610112.H>	// Lapis Micro ML610Q112 on LaPi Development Board
	#include	<stdlib.h>		// General-purpose utilities
	#include 	<uart.h>		// UART Function Prototypes
	#include 	<common.h>		// Common Definitions
	#include 	<irq.h>			// IRQ Definitions
	#include 	<mcu.h>			// MCU Definition
	#include	<i2c.h>			// I2C Definition
	//#include 	<clock.h>		// Set System Clock API
	#include 	<tbc.h>			// Set TBC (Timer Based Clock) API
	#include 	<timer.h>		// Timer Macros & APIs
	//#include 	<main.h>		// Clear WDT API
	//#include	<ctype.h>		// Character classification and conversion 
	//#include	<errno.h>		// Error identifiers Library
	//#include	<float.h>		// Numerical limits for floating-point numbers
	//#include	<limits.h>		// Numerical limits for integers
	#include	<math.h>		// Mathematical functions
	//#include	<muldivu8.h>	// Multiplication and Division accelerator
	//#include	<setjmp.h>		// Global jump (longjmp)
	//#include	<signal.h>		// Signal handling functions
	//#include	<stdarg.h>		// Variable numbers of arguments
	//#include	<stddef.h>		// Standard types and macros 
	#include	<stdio.h>		// I/O-related processing
	#include	<string.h>		// Character string manipulation routines
	//#include	<yfuns.h>		// 
	//#include	<yvals.h>		// Called for by most Header Files

//===========================================================================
//   MACROS: 
//===========================================================================
#define WelcomeString	("\033[2J\033[1;1H"\
	"*********************************************\n\r"\
	"** Q112 Firmware - Sensor Platform EVK\n\r"\
	"** Revision    : REV00\n\r"\
	"** Release date: " __DATE__ " " __TIME__ "\n\r"\
	"** By          : ROHM Semiconductor USA, LLC\n\r"\
	"*********************************************\n\r"\
)

#define PRINTF(msg)		write(0, msg, sizeof(msg))

// ===== Peripheral setting.=====
#define HSCLK_KHZ	( 8000u )	// 8MHz = 8000kHz (will be multiplied by 1024 to give 8,192,000Hz)
#define FLG_SET		( 0x01u )

// SET DESIRED UART SETTINGS HERE! (Options in UART.h)
//#define UART_BAUDRATE		( UART_BR_115200BPS) 	// Data Bits Per Second - Tested at rates from 2400bps to 512000bps!
#define UART_BAUDRATE		( UART_BR_19200BPS) 	// Data Bits Per Second - Tested at rates from 2400bps to 512000bps!
#define UART_DATA_LENGTH	( UART_LG_8BIT )		// x-Bit Data
#define UART_PARITY_BIT		( UART_PT_NON )		// Parity
#define UART_STOP_BIT		( UART_STP_1BIT )		// x Stop-Bits
#define UART_LOGIC		( UART_NEG_POS )		// Desired Logic
#define UART_DIRECTION		( UART_DIR_LSB )		// LSB or MSB First
//#define _TBC_H_

/**
 * Sensor Interface Header 1
 */
#define SENINTF_HDR1_GPIO0(reg)		PB2##reg
#define SENINTF_HDR1_GPIO1(reg)		PB3##reg
#define SENINTF_HDR1_GPIO2(reg)		PB4##reg
#define SENINTF_HDR1_GPIO3(reg)		PB7##reg

/**
 * LED[7-0]
 */
#define LEDOUT(x)	PCD=x
#define turnSpeed 10
#define forwardSpeed 15
#define LeftServoNeutral 753 // Increase for CW
#define RightServoNeutral 634 // Increase for CCW
#define sumMax 21
//#define colorTolerance 0.009
//===========================================================================
//   STRUCTURES:    
//===========================================================================
static const tUartSetParam  _uartSetParam = {		// UART Parameters
	UART_BAUDRATE,								// Members of Structure...
	UART_DATA_LENGTH,							// Members of Structure...
	UART_PARITY_BIT,							// Members of Structure...
	UART_STOP_BIT,								// Members of Structure...
	UART_LOGIC,									// Members of Structure...
	UART_DIRECTION								// Members of Structure...
};

//===========================================================================
//   FUNCTION PROTOTYPES: 
//	Establishes the name and return type of a function and may specify the 
// 	types, formal parameter names and number of arguments to the function                                 
//===========================================================================
void main_clrWDT( void );			// no return value and no arguments
void Initialization( void );		// no return value and no arguments
void SetOSC( void );				// no return value and no arguments
void PortA_Low( void );				// no return value and no arguments
void PortB_Low( void );				// no return value and no arguments
void PortC_Low( void );				// no return value and no arguments
void PortD_Low( void );				// no return value and no arguments

//RTLU8: Low-level function
int write(int handle, unsigned char *buffer, unsigned int len);
int ADC_Read(unsigned char idx);
void I2C_Read(unsigned char slave_address, unsigned char *reg_address, unsigned char reg_address_size, unsigned char *buffer, unsigned char size);
void I2C_Write(unsigned char slave_address, unsigned char *reg_address, unsigned char reg_address_size, unsigned char *buffer, unsigned char size);

//UART and I2C Functions
void _funcUartFin( unsigned int size, unsigned char errStat );
void _funcI2CFin( unsigned int size, unsigned char errStat );
void main_reqNotHalt( void );
void _intUart( void );
void _intI2c( void );
void _intADC( void );
void NOPms( unsigned int ms );

unsigned char ReverseBits(unsigned char data);
void FlashLEDs(void);

void DeviceSelection(void); // Initializes port D for registering Sensor Control States
void SensorInitialization(void); 
  
void OutputPWM(void);
//void GetUART_Command(void);

void LED_ON();
void LED_OFF();
void RED_ON();
void GREEN_ON();
void BLUE_ON();
void TurnLeft(unsigned int value);
void TurnRight(unsigned int value);
void RGB_OFF(void);
void RGB_OFFSET(void);
void GoStraight(void);
void ServoStop(void);
void PrintToScreen(void);
void RUN_COLOR_DETECTION(void);
void GuessingGame();
int Color_Detection_DEMO();
double getABS(long double a);
//*****************************************************************************
// GLOBALS...
// ADC, UART and I2C Variables
unsigned char	_flgUartFin;
unsigned char 	_flgI2CFin;
unsigned char	_flgADCFin;
unsigned char	_reqNotHalt;

union {
	unsigned char	_uchar;
	unsigned char	_ucharArr[6];
	unsigned int	_uint;
	unsigned int	_uintArr[3];
	int				_intArr[3];
	float			_float;
} uniRawSensorOut;

float flSensorOut[3];

/**
 * ANSI Escape Code
 */
#define ESC_SOL			"\r"
#define ESC_NEWLINE		"\n\r"
#define ESC_PREVLINE	"\033[F"
#define ESC_ERASE2END	"\033[J"
/**
 * Ambient Light Sensors
 */ 
 
 
static unsigned int 			PWMSafeDuty = 8400;						//Value for Safe Duty = Off, right before starting to spin
static unsigned int				PWMPeriod = 17000; 						//Value for Period 

 
/**
 * ML8511 (UV Sensor)
 *		Vout 		: 2.2[V] @ 10[mW/cm2]
 *		Sensitivity	: 0.129[Vcm2/mW]
 */
#define	Voltage2UVIntensity(v)	(v-2.2f)/0.129f+10

/**
 * Temperature Sensors
 *		Vout		: v0[V] @ t0[°C]
 *		Sensitivity	: s[V/°C]
 */
#define Voltage2Temperature(v, v0, t0, s)	(v-(v0))/(s)+(t0)


const unsigned char BH1745_A			= 0x39u; 
const unsigned char BH1745_B			= 0x38u; 

const unsigned char MANUFACTURER_ID		= 0x92u; 
const unsigned char SYSTEM_CONTROL		= 0x40u; 
const unsigned char MODE_CONTROL1		= 0x41u; 
const unsigned char MODE_CONTROL2		= 0x42u; 
const unsigned char MODE_CONTROL3		= 0x43u; 
const unsigned char PERSISTENCE	 		= 0x61u; 
const unsigned char INTERRUPT	 		= 0x60u; 

const unsigned char RED_DATA_LSBs	 		= 0x50u; 
const unsigned char RED_DATA_MSBs	 		= 0x51u; 
 
const unsigned char GREEN_DATA_LSBs	 		= 0x52u; 
const unsigned char GREEN_DATA_MSBs	 		= 0x53u; 
 
const unsigned char BLUE_DATA_LSBs	 		= 0x54u; 
const unsigned char BLUE_DATA_MSBs	 		= 0x55u; 
 
 

static unsigned char SensorPlatformSelection;
static unsigned char SensorIntializationFlag = 1;
static unsigned char LEDFlashFlag = 0;
static unsigned char LEDChangeFlag = 0;

static unsigned char			soundNote[2];
static unsigned char			val[50];//val[800];
static unsigned char			buffer[120]; 
static unsigned char			word[88]; 
static unsigned char            temp;
static unsigned int             flag;
static unsigned int             checkSum;
static unsigned int             wordIndex;
static unsigned int             hexToDecOffset;  
static unsigned char			NewLineChar[3] = {10,13,0}, lineStr[21];

static unsigned int             UTC[3];
static unsigned char            LatDir, LonDir,LatLonValid, Mode[2];
static unsigned  int 			i,SV_ID[12];
static unsigned  int 			j, sigDigits;
static long double 				Latitude, Longitude,HDOP,PDOP,VDOP,MSL,Geoid;
static unsigned int 			fixQuality,numSat,isNeg;
static unsigned int				GSV_numMessage,GSV_index,numSat,PRN_num,Eleveation,Azimuth,SNR,GSV_Info[12];
  
static long double				prevBulbIntensity;
static unsigned int             bulbIntensity, bulbEnable;

static unsigned int            FirstColor,ColorCode;

struct RGBC
{
	float R;
	float G;
	float B;
	float C;
};
struct HSL
{
	float H;
	float S;
	float L;
};
struct hsl_color_db
{
	char color [7];
	float hue;
	float sat;
	float lum;
};
#define color_record	12
struct hsl_color_db	hsl_colors[color_record]; 
static struct RGBC				 rgb_dark,rgb_thresh, rgb_gain, rgb_avg, rgb_min, rgb_max, rgb_offset1,rgb_offset2;		 
static struct HSL 				 hsl_avg;
static struct HSL filter_hsl;
static  int              wordSize,bufferSize, LRC;
void hsl_filter_average();
static int				 sumIndex;
int filter_flag;
static unsigned char	 singleChar[1],buffer[120],line1[21],line2[20],line3[15],line4[20];
static int b,c;
static long double a, configH, configS,configL,  HL,HH,SL,SH,LL,LH,  colorTolerance, prev_hsl_ave, deltaHSL;

static float tempH,tempS,tempL, tolH, tolS, tolL;

int rawR,rawG,rawB,rawC, isMatched;
			
void RGB_dataacq (void);

int cmpfunc (const void * a, const void * b);
void f_sort(int* a, int n );
static int tolerance = 1;
const unsigned char sensor_addr[2] = { 0x38u, 0x39u};
void rgb_2_hsl();
void hsl_load(void);
#define MAX(a,b)		(((a) > (b) ) ? (a) : (b) )
#define MIN(a,b)		(((a) < (b) ) ? (a) : (b) )

//===========================================================================
//  	Start of MAIN FUNCTION
//===========================================================================
int main(void) 
{ 	 
	Initialization(); //Ports, UART, Timers, Oscillator, Comparators, etc.
    colorTolerance = 0.002;
	
	/*
	for(i=0;i<5;i++){
	for(i=0;i<5;i++){
		main_clrWDT(); 
		NOPms(30); 
		FlashLEDs();
	} 
	*/
	
	PB3D = 0; // RGB Sensor ADDR
	PD5D = 1; // RGB Sensor ADDR
	 
	
	temp = 0x03u;
	I2C_Write(0x38u, &PERSISTENCE, 1, &temp, 1); 
	I2C_Write(0x39u, &PERSISTENCE, 1, &temp, 1); 
	temp = 0x00u;
	I2C_Write(0x38u, &MODE_CONTROL1, 1, &temp, 1);  //011 : 1280msec
	I2C_Write(0x39u, &MODE_CONTROL1, 1, &temp, 1); 
	temp = 0x92u;
	I2C_Write(0x38u, &MODE_CONTROL2, 1, &temp, 1); 
	I2C_Write(0x39u, &MODE_CONTROL2, 1, &temp, 1); //16x gain, RGBC_EN
	temp = 0x02u;
	I2C_Write(0x38u, &MODE_CONTROL3, 1, &temp, 1); 
	I2C_Write(0x39u, &MODE_CONTROL3, 1, &temp, 1); 
	 
	FlashLEDs();
	FlashLEDs();
	FlashLEDs();		
	FlashLEDs(); 
	RGB_OFF();					
	
	LED_ON();				//turn ON
	   
	
	for(i=0;i<80;i++){
		buffer[i] = 0;
		main_clrWDT(); 
	}

	bufferSize = sprintf(line1 ,"%c<Color Sensor Demo>",128); 
	bufferSize += sprintf(line2,  "    CHOOSE A COLOR  ");  
	
	strcat(buffer,line1);
	strcat(buffer,line2);
	for(i=45;i<85;i++){
		buffer[i] = 0;
	}  
	write(0,buffer,bufferSize); 
		
	deltaHSL = 10; 
	//hsl_load();
	while(1){ 		 
		main_clrWDT(); 
		RGB_dataacq();	 
		main_clrWDT(); 
			
		 
		PrintToScreen(); 
		 
	}
		
}
//===========================================================================
//  	End of MAIN FUNCTION
//===========================================================================
void f_sort(int* a, int n )
{
	//bubble sort
	// a = a[0]:  first position in float index
	// n 		  number of elements
	int i,j;
	int value = 0;
	
	for(i=0;i<n-1;i++)
	{
		for(j=0;j<n-i-1;j++)
		{
			if(a[j]>a[j+1])
			{
				value=a[j+1];
				a[j+1]=a[j];
				a[j]=value;
			}
		}
	}
}

void rgb_2_hsl()
{
	float fmax, fmin, fdel;		
	
	
	    main_clrWDT(); 
	fmax = MAX( MAX(rgb_avg.R,rgb_avg.G), rgb_avg.B);
	fmin = MIN( MIN(rgb_avg.R,rgb_avg.G), rgb_avg.B);
	fdel = fmax-fmin;
	
	hsl_avg.L = (fmax+fmin)/2;
	
	if(fmax  ==  fmin)
	{
	    main_clrWDT(); 
		hsl_avg.S = 0;
		hsl_avg.H = 0;
	}
	else
	{	//define saturation
	    main_clrWDT(); 
		if(hsl_avg.L > 0.5)
			hsl_avg.S = fdel / (2 - fmax - fmin);
		else
			hsl_avg.S = fdel / (fmax + fmin);
		
		//define Hue		
		if( fmax == rgb_avg.R)
		{
			hsl_avg.H = (rgb_avg.G > rgb_avg.B ? 6 : 0)+(rgb_avg.G - rgb_avg.B)/(fdel);	
		}
		else if( fmax == rgb_avg.G)
		{
			hsl_avg.H = 2 + (rgb_avg.B - rgb_avg.R)/(fmax-fmin);	
		}
		else
		{
			hsl_avg.H = 4 + (rgb_avg.R - rgb_avg.G)/(fmax-fmin);	
		}
		
		hsl_avg.H /= 6;
	}
}
void RGB_dataacq(void)
{	
    main_clrWDT(); 
	while(filter_flag  != sumMax-1 ){ 
		int rgb_s1_R [sumMax];
		int rgb_s1_G [sumMax];
		int rgb_s1_B [sumMax];
		int rgb_s1_C [sumMax];
		 
		int rgb_s1R=0,rgb_s1G=0,rgb_s1B=0,rgb_s1C=0;
		
	    main_clrWDT(); 
		//init arrays to zero
		for(sumIndex = 0; sumIndex < sumMax; sumIndex++)
		{
			 main_clrWDT();
			 rgb_s1_R [sumIndex] = 0;
			 rgb_s1_G [sumIndex] = 0;
			 rgb_s1_B [sumIndex] = 0;
			 rgb_s1_C [sumIndex] = 0;
		}
		
		rgb_avg.R = 0;
		rgb_avg.G = 0;
		rgb_avg.B = 0;
		rgb_avg.C = 0;
		
		
		for(sumIndex = 0; sumIndex < sumMax; sumIndex++){
			main_clrWDT();
			for(i = 0; i < 2; i++)
			{
				main_clrWDT(); 
				I2C_Read(sensor_addr[i], &RED_DATA_LSBs, 1, uniRawSensorOut._ucharArr, 8);
			 
				main_clrWDT();
					rawR = ((int)uniRawSensorOut._ucharArr[0] | ((int)uniRawSensorOut._ucharArr[1])<<8);
					rawG = ((int)uniRawSensorOut._ucharArr[2] | ((int)uniRawSensorOut._ucharArr[3])<<8);
					rawB = ((int)uniRawSensorOut._ucharArr[4] | ((int)uniRawSensorOut._ucharArr[5])<<8);
					rawC = ((int)uniRawSensorOut._ucharArr[6] | ((int)uniRawSensorOut._ucharArr[7])<<8); 
					rgb_s1_R[sumIndex] += 	rawR;
					rgb_s1_G[sumIndex] += 	rawG;
					rgb_s1_B[sumIndex] += 	rawB;
					rgb_s1_C[sumIndex] += 	rawC;
			} 
		}
		
		//Get the middle values and average that out
		//Sort First
	    main_clrWDT(); 
		f_sort(rgb_s1_R, sumMax );
		f_sort(rgb_s1_G, sumMax );
		f_sort(rgb_s1_B, sumMax );
		f_sort(rgb_s1_C, sumMax );

		//Array of size = sumMax = 21 (odd)
		//Add elements around the median
		i = 0;
		for(i = (sumMax-1)/2 - (tolerance -1); i<(sumMax-1)/2 + tolerance; i++ )
		{
			main_clrWDT();
			 rgb_s1R += rgb_s1_R[i]; 
			 rgb_s1G += rgb_s1_G[i]; 
			 rgb_s1B += rgb_s1_B[i]; 
			 rgb_s1C += rgb_s1_C[i]; 
		}
		
		// number of values being avgd = 2*tolerance - 1
		// number of sensors = 2
		//scale values from 0 to 1: Max value 2^16-1 = 65535
	    main_clrWDT(); 
		rgb_avg.R = ((float)rgb_s1R / ( (2*tolerance - 1) * 2  ))/65535;
		rgb_avg.G = ((float)rgb_s1G / ( (2*tolerance - 1) * 2  ))/65535;
		rgb_avg.B = ((float)rgb_s1B / ( (2*tolerance - 1) * 2  ))/65535;
		rgb_avg.C = ((float)rgb_s1C / ( (2*tolerance - 1) * 2  ))/65535;
		
		//initialize to zero
		hsl_avg.H = 0;
		hsl_avg.S = 0;
		hsl_avg.L = 0; 
		rgb_2_hsl();	 
		for(i=0;i<80;i++){
			buffer[i] = 0;
			main_clrWDT(); 
		} 
		hsl_filter_average();
	}
	hsl_avg.H = filter_hsl.H/sumMax;
	hsl_avg.S = filter_hsl.S/sumMax;
	hsl_avg.L = filter_hsl.L/sumMax; 
	
	filter_flag = 0;  
	filter_hsl.H=0;
	filter_hsl.S=0;
	filter_hsl.L=0;  
}

void hsl_load()
{
	//hsl_colors[] is the global array containing this data
	//when adding new color, increase color_record, and put information in the loop
	int i =0;
	
	sprintf(hsl_colors[i].color,"Sun Ray",sizeof("Sun Ray"));
 
	hsl_colors[i].hue =		1.454513859;
	hsl_colors[i].sat =		0.870595585;
	hsl_colors[i].lum =		0.017923247;
	
	i++;
	
	sprintf(hsl_colors[i].color,"DesGlow",sizeof("DesGlow"));
	 
	hsl_colors[i].hue =		1.448456415;
	hsl_colors[i].sat =		0.870871432;
	hsl_colors[i].lum =		0.017370871;
	
	sprintf(hsl_colors[i].color,"BetStar",sizeof("BetStar"));
	 
	hsl_colors[i].hue =		1.625563098;
	hsl_colors[i].sat =		0.870440593;
	hsl_colors[i].lum =		0.023343252;
	
	sprintf(hsl_colors[i].color,"CSplash",sizeof("CSplash"));
	 
	hsl_colors[i].hue =		1.575271748;
	hsl_colors[i].sat =		0.892680291;
	hsl_colors[i].lum =		0.022493324;
	
	sprintf(hsl_colors[i].color,"Shamrok",sizeof("Shamrok"));
	 
	hsl_colors[i].hue =		2.06427171;
	hsl_colors[i].sat =		0.782725769;
	hsl_colors[i].lum =		0.009221027;
	
	sprintf(hsl_colors[i].color,"MsGreen",sizeof("MsGreen"));
	 
	hsl_colors[i].hue =		2.121634892;
	hsl_colors[i].sat =		0.786125297;
	hsl_colors[i].lum =		0.007699448;
	
	sprintf(hsl_colors[i].color,"PnScent",sizeof("PnScent"));
	 
	hsl_colors[i].hue =		2.162181544;
	hsl_colors[i].sat =		0.78320593;
	hsl_colors[i].lum =		0.006306554;
	
	sprintf(hsl_colors[i].color,"PsySpig",sizeof("PsySpig"));
	 
	hsl_colors[i].hue =		2.189071285;
	hsl_colors[i].sat =		0.798537404;
	hsl_colors[i].lum =		0.0066804;
	
	sprintf(hsl_colors[i].color,"D1",sizeof("D1"));
	 
	hsl_colors[i].hue =		1.494513859;
	hsl_colors[i].sat =		0.870595585;
	hsl_colors[i].lum =		0.017923247;
	
	sprintf(hsl_colors[i].color,"D2",sizeof("D2"));
	 
	hsl_colors[i].hue =		1.484513859;
	hsl_colors[i].sat =		0.870595585;
	hsl_colors[i].lum =		0.017923247;
	
	sprintf(hsl_colors[i].color,"D3",sizeof("D3"));
	 
	hsl_colors[i].hue =		1.514513859;
	hsl_colors[i].sat =		0.870595585;
	hsl_colors[i].lum =		0.017923247;
	
	sprintf(hsl_colors[i].color,"D4",sizeof("D4"));
	 
	hsl_colors[i].hue =		1.524513859;
	hsl_colors[i].sat =		0.870595585;
	hsl_colors[i].lum =		0.017923247;
	

	
}

void hsl_filter_average()
{
	
	    main_clrWDT(); 
	filter_hsl.H += hsl_avg.H;
	filter_hsl.S += hsl_avg.S;
	filter_hsl.L += hsl_avg.L;
	filter_flag++;
	
}



void RUN_COLOR_DETECTION(void){  
	/*
		White paper = Program Starts over
		Program 1) scan first color to be compared (msg: "Please scan the first color)
				2) scann the same color (msg: "Please find the matching color)
				3) Display result (msg1: "Correct Color Selected!")
								  (msg2: "The correct color is 'X'")
		Design guide:   Only scan when color is not fluxuating (ie. moving around)
	*/
	main_clrWDT(); 
	//initialize to zero
	hsl_avg.H = 0;
	hsl_avg.S = 0;
	hsl_avg.L = 0;
	
	rgb_2_hsl();	
		/*
		for(i=0;i<80;i++){
			main_clrWDT(); 
			buffer[i] = 0;
		} */
		 
		hsl_filter_average();
		
		if(filter_flag  == sumMax-1 ){
			main_clrWDT(); 
			hsl_avg.H = filter_hsl.H/sumMax;
			hsl_avg.S = filter_hsl.S/sumMax;
			hsl_avg.L = filter_hsl.L/sumMax; 
			//******************************************************************************************** 
			
			//bufferSize = sprintf(buffer,"%c%cH%5.3f S%5.3f L%5.3f",128,221,hsl_avg.H,hsl_avg.S,hsl_avg.L);  
			bufferSize = sprintf(buffer,"%cH%5.3f S%5.3f L%5.3f",128,hsl_avg.H,hsl_avg.S,hsl_avg.L);  
			wordSize = bufferSize; 
			  
			/*
			//********************************* KINKO PRINTS ******************************************* 
			configH = 1.014;
			configS = 0.565;
			configL = 0.010;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"RED A  "); 
			}  
			configH = 1.012;
			configS = 0.550;
			configL = 0.011;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"RED B  "); 
			}  
			configH = 1.012;
			configS = 0.545;
			configL = 0.011;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"RED C  "); 
			}  
			configH = 1.016;
			configS = 0.538;
			configL = 0.012;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"RED D  "); 
			}  
			
			configH = 0.478;
			configS = 0.535;
			configL = 0.006;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Blue A  "); 
			}   
			configH = 0.485;
			configS = 0.558;
			configL = 0.007;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Blue B  "); 
			}   
			configH = 0.485;
			configS = 0.576;
			configL = 0.007;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Blue C  "); 
			}   
			configH = 0.486;
			configS = 0.595;
			configL = 0.008;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Blue D  "); 
			}   
			
			configH = 0.320;
			configS = 0.606;
			configL = 0.014;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Green A  "); 
			}   
			configH = 0.322;
			configS = 0.617;
			configL = 0.014;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Green B  "); 
			}   
			configH = 0.325;
			configS = 0.620;
			configL = 0.015;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Green C  "); 
			}   
			configH = 0.329;
			configS = 0.604;
			configL = 0.018;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Green D  "); 
			}   
			//******************************************************************************************
			*/
			
			wordSize += bufferSize;
			//NOPms(10);
			checkSum = strcmp(lineStr,line4); 
			//if(checkSum){
				//GuessingGame();
				strcat(line4,NewLineChar); // *** ONLY WHEN PRINTING TO PC SCREEN ***
					
				strcat(buffer,line4);
				//strcat(buffer,line4); 
				//strcat(buffer,line2); 
				//strcat(buffer,line3); 
				
					
				for(wordSize;wordSize< 41;wordSize++){
					buffer[wordSize] = ' ';
				}
				buffer[wordSize] = 0; 
				
				write(0,buffer,wordSize);  
			//}
			
			strcpy(lineStr,line4);
			
			filter_flag = 0;  
  
			filter_hsl.H=0;
			filter_hsl.S=0;
			filter_hsl.L=0;
		} 
}

void GuessingGame(){ 
	/*
		White paper = Program Starts over
		Program 1) scan first color to be compared (msg: "Please scan the first color)
				2) scann the same color (msg: "Please find the matching color)
				3) Display result (msg1: "Correct Color Selected!")
								  (msg2: "The correct color is 'X'")
		Design guide:   Only scan when color is not fluxuating (ie. moving around)
	*/
	main_clrWDT(); 
	//initialize to zero
	hsl_avg.H = 0;
	hsl_avg.S = 0;
	hsl_avg.L = 0;
	
	rgb_2_hsl();	
		/*
		for(i=0;i<80;i++){
			main_clrWDT(); 
			buffer[i] = 0;
		} */
		 
		hsl_filter_average();
		
		if(filter_flag  == sumMax-1 ){
			main_clrWDT(); 
			hsl_avg.H = filter_hsl.H/sumMax;
			hsl_avg.S = filter_hsl.S/sumMax;
			hsl_avg.L = filter_hsl.L/sumMax; 
			//******************************************************************************************** 
			
			//bufferSize = sprintf(buffer,"%c%cH%5.3f S%5.3f L%5.3f",128,221,hsl_avg.H,hsl_avg.S,hsl_avg.L);  
			bufferSize = sprintf(buffer,"%cH%5.3f S%5.3f L%5.3f",128,hsl_avg.H,hsl_avg.S,hsl_avg.L);  
			wordSize = bufferSize; 
			  
			/*
			//********************************* KINKO PRINTS ******************************************* 
			configH = 1.014;
			configS = 0.565;
			configL = 0.010;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"RED A  "); 
			}  
			configH = 1.012;
			configS = 0.550;
			configL = 0.011;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"RED B  "); 
			}  
			configH = 1.012;
			configS = 0.545;
			configL = 0.011;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"RED C  "); 
			}  
			configH = 1.016;
			configS = 0.538;
			configL = 0.012;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"RED D  "); 
			}  
			
			configH = 0.478;
			configS = 0.535;
			configL = 0.006;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Blue A  "); 
			}   
			configH = 0.485;
			configS = 0.558;
			configL = 0.007;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Blue B  "); 
			}   
			configH = 0.485;
			configS = 0.576;
			configL = 0.007;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Blue C  "); 
			}   
			configH = 0.486;
			configS = 0.595;
			configL = 0.008;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Blue D  "); 
			}   
			
			configH = 0.320;
			configS = 0.606;
			configL = 0.014;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Green A  "); 
			}   
			configH = 0.322;
			configS = 0.617;
			configL = 0.014;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Green B  "); 
			}   
			configH = 0.325;
			configS = 0.620;
			configL = 0.015;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Green C  "); 
			}   
			configH = 0.329;
			configS = 0.604;
			configL = 0.018;
			if(( (configH-colorTolerance) < hsl_avg.H && hsl_avg.H < (configH+colorTolerance) ) && ( (configS-colorTolerance) < hsl_avg.S  && hsl_avg.S  < (configS+colorTolerance) ) && ( (configL-colorTolerance) < hsl_avg.L  && hsl_avg.L  < (configL+colorTolerance) ))
			{
				
				bufferSize = sprintf(line4,"Green D  "); 
			}   
			//******************************************************************************************
			*/
			
			wordSize += bufferSize;
			//NOPms(10);
			checkSum = strcmp(lineStr,line4); 
			//if(checkSum){
				//GuessingGame();
				strcat(line4,NewLineChar); // *** ONLY WHEN PRINTING TO PC SCREEN ***
					
				strcat(buffer,line4);
				//strcat(buffer,line4); 
				//strcat(buffer,line2); 
				//strcat(buffer,line3); 
				
					
				for(wordSize;wordSize< 41;wordSize++){
					buffer[wordSize] = ' ';
				}
				buffer[wordSize] = 0; 
				
				write(0,buffer,wordSize);  
			//}
			
			strcpy(lineStr,line4);
			
			filter_flag = 0;  
  
			filter_hsl.H=0;
			filter_hsl.S=0;
			filter_hsl.L=0;
		} 
}

double getABS(long double a){
	if(a>0)
		return a;
	else	
		return a*-1.0;
}

void PrintToScreen(void){
	    main_clrWDT(); 
		
		
		if(rawC >500){
			main_clrWDT(); 
			// display "Choose a color"
			
			for(i=0;i<80;i++){
				buffer[i] = 0;
				main_clrWDT(); 
			}
	
			bufferSize = sprintf(line1 ,"%c<Color Sensor Demo>",128); 
			bufferSize += sprintf(line2,"    CHOOSE A COLOR   "); 
			main_clrWDT();  
			
			strcat(buffer,line1);
			strcat(buffer,line2);
			for(i=45;i<85;i++){
				buffer[i] = 0;
			}  
			write(0,buffer,bufferSize); 
			main_clrWDT(); 
			
			while(rawC > 500){ // wait until rawC < 500
				main_clrWDT(); 
				RGB_dataacq();
			} 
			main_clrWDT(); 
			
			
		
			for(i=0;i<80;i++){
				buffer[i] = 0;
				main_clrWDT(); 
			}
	 
			
			
		
			while(deltaHSL > 0.005){
				main_clrWDT(); 
				RGB_dataacq();
				deltaHSL = getABS(hsl_avg.H -prev_hsl_ave);
				prev_hsl_ave = hsl_avg.H;
				NOPms(100);
			}
			main_clrWDT(); 
			NOPms(100);
			NOPms(100);
			NOPms(100);
			main_clrWDT(); 
			NOPms(100);
			NOPms(100);
			main_clrWDT();  
			NOPms(100);
			NOPms(100);
			NOPms(100);
			main_clrWDT(); 
			NOPms(100);
			NOPms(100);
			main_clrWDT(); 
			deltaHSL = 10;
			while(deltaHSL > 0.005){
				main_clrWDT(); 
				RGB_dataacq();
				deltaHSL = getABS(hsl_avg.H -prev_hsl_ave);
				prev_hsl_ave = hsl_avg.H;
				NOPms(100);
			}
			main_clrWDT(); 
			
			for(i=0;i<80;i++){
				buffer[i] = 0;
				main_clrWDT(); 
			}
			tempH = hsl_avg.H;
			tempS = hsl_avg.S;
			tempL = hsl_avg.L;
			main_clrWDT(); 
			bufferSize = sprintf(line1,"%cH%5.3f S%5.3f L%5.3f",128,hsl_avg.H,hsl_avg.S,hsl_avg.L);  
			bufferSize += sprintf(line2,"Scan WHT to continue");  
			
			strcat(buffer,line1);
			strcat(buffer,line2);
			for(i=47;i<85;i++){
				main_clrWDT(); 
				buffer[i] = 0;
			}  
			write(0,buffer,bufferSize);  
			main_clrWDT(); 
			
			while(rawC < 500){ // wait until rawC < 500
				main_clrWDT(); 
				RGB_dataacq();
			} 
			main_clrWDT(); 
			
			bufferSize = sprintf(line1,"%cMatch same color ",128);  
			bufferSize += sprintf(line2,"to SPIN THE WHEEL      ");  
			
			strcat(buffer,line1);
			strcat(buffer,line2);
			for(i=47;i<85;i++){
				main_clrWDT(); 
				buffer[i] = 0;
			}  
			write(0,buffer,bufferSize);  
			main_clrWDT(); 
			
			
			
			while(rawC > 500){ // wait until rawC < 500
				main_clrWDT(); 
				RGB_dataacq();
			} 
			main_clrWDT(); 
			NOPms(100);
			NOPms(100);
			NOPms(100);
			main_clrWDT(); 
			NOPms(100);
			NOPms(100);
			NOPms(100);
			NOPms(100);
			main_clrWDT(); 
			
			for(i=0;i<80;i++){
				buffer[i] = 0;
				main_clrWDT(); 
			} 
			while(deltaHSL > 0.005){
				main_clrWDT(); 
				RGB_dataacq();
				deltaHSL = getABS(hsl_avg.H -prev_hsl_ave);
				prev_hsl_ave = hsl_avg.H;
				NOPms(100);
			}
			main_clrWDT(); 
			NOPms(100);
			NOPms(100);
			NOPms(100);
			main_clrWDT(); 
			NOPms(100);
			NOPms(100);
			main_clrWDT();  
			NOPms(100);
			NOPms(100);
			NOPms(100);
			main_clrWDT(); 
			NOPms(100);
			NOPms(100);
			main_clrWDT(); 
			deltaHSL = 10;
			while(deltaHSL > 0.005){
				main_clrWDT(); 
				RGB_dataacq();
				deltaHSL = getABS(hsl_avg.H -prev_hsl_ave);
				prev_hsl_ave = hsl_avg.H;
				NOPms(100);
			}
			main_clrWDT(); 
			
			
			
			isMatched = 0;
			configH = tempH;
			configS = tempS;
			configL = tempL;
			
			if(hsl_avg.H > 0.470 && hsl_avg.H < 0.495){
				HL = (configH-0.003);
				HH = (configH+0.003);
				SL = (configS-0.005);
				SH = (configS+0.005);
				LL = (configL-0.0005);
				LH = (configL+0.0005);
				main_clrWDT(); 
				if( HL < hsl_avg.H && hsl_avg.H < HH )
					if( SL < hsl_avg.S  && hsl_avg.S  < SH )
						if( LL < hsl_avg.L && hsl_avg.L < LH)
							isMatched = 1;  
				main_clrWDT(); 	
			}else{ 
				HL = (configH-0.0013);
				HH = (configH+0.0013);
				SL = (configS-0.002);
				SH = (configS+0.002);
				LL = (configL-0.0005);
				LH = (configL+0.0005);
				main_clrWDT(); 
				if( HL < hsl_avg.H && hsl_avg.H < HH )
					if( SL < hsl_avg.S  && hsl_avg.S  < SH )
						if( LL < hsl_avg.L && hsl_avg.L < LH)
							isMatched = 1;  
				main_clrWDT(); 
			}
			
			
			
			
				
			tolH = hsl_avg.H - tempH;
			main_clrWDT(); 
			tolS = hsl_avg.S - tempS;
			main_clrWDT(); 
			tolL = hsl_avg.L - tempL;
			main_clrWDT(); 
			
			for(i=0;i<80;i++){
				buffer[i] = 0;
				main_clrWDT(); 
			}
			if(isMatched){
				main_clrWDT(); 
				bufferSize = sprintf(line1,"%cH%5.3f S%5.3f L%5.3f",128,hsl_avg.H,hsl_avg.S,hsl_avg.L);  
				bufferSize += sprintf(line2,"  Matched, you WIN! ");  
				                            
				strcat(buffer,line1);
				strcat(buffer,line2);
				for(i=47;i<85;i++){
					main_clrWDT(); 
					buffer[i] = 0;
				}  
				write(0,buffer,bufferSize); 
				main_clrWDT();  
			}
			else{
				main_clrWDT(); 
				bufferSize = sprintf(line1,"%cH%5.3f S%5.3f L%5.3f",128,hsl_avg.H,hsl_avg.S,hsl_avg.L);  
				bufferSize += sprintf(line2,"Mismatch,         ");  
				
				strcat(buffer,line1);
				strcat(buffer,line2);
				for(i=47;i<85;i++){
					main_clrWDT(); 
					buffer[i] = 0;
				}  
				write(0,buffer,bufferSize); 
			}
			
		}  
		filter_flag = 0;  
		filter_hsl.H=0;
		filter_hsl.S=0;
		filter_hsl.L=0;  
}

// void RGB_OFFSET(void){ 
		// I2C_Read(0x38u, &RED_DATA_LSBs, 1, uniRawSensorOut._ucharArr, 6); 
		// main_clrWDT();  
		// R1 = ((int)uniRawSensorOut._ucharArr[0] | ((int)uniRawSensorOut._ucharArr[1])<<8);
		// G1 = ((int)uniRawSensorOut._ucharArr[2] | ((int)uniRawSensorOut._ucharArr[3])<<8);
		// B1 = ((int)uniRawSensorOut._ucharArr[4] | ((int)uniRawSensorOut._ucharArr[5])<<8); 
		// R1 /= 9;
		// G1 /= 9;
		// B1 /= 9;
		
		// R1_Offset = -R1;
		// G1_Offset = -G1;
		// B1_Offset = -B1;
		 
		// I2C_Read(0x39u, &RED_DATA_LSBs, 1, uniRawSensorOut._ucharArr, 6); 
		// main_clrWDT();  
		
		// R2 = ((int)uniRawSensorOut._ucharArr[0] | ((int)uniRawSensorOut._ucharArr[1])<<8);
		// G2 = ((int)uniRawSensorOut._ucharArr[2] | ((int)uniRawSensorOut._ucharArr[3])<<8);
		// B2 = ((int)uniRawSensorOut._ucharArr[4] | ((int)uniRawSensorOut._ucharArr[5])<<8); 
		// R2 /= 9;
		// G2 /= 9;
		// B2 /= 9;
		
		// R2_Offset = -R2;
		// G2_Offset = -G2;
		// B2_Offset = -B2;
// }

// void TurnLeft(value){ 
			// PFRUN = 0;
			// PCRUN = 0;
			 
			// PWF0D = LeftServoNeutral  + value - forwardSpeed; // Servo
			// PWCD  = RightServoNeutral + value + (forwardSpeed*53/100); 
			 
			// PFRUN = 1;
			// PCRUN = 1;  
// }

// void TurnRight(value){ 
			// PFRUN = 0;
			// PCRUN = 0;
			 
			// PWF0D = LeftServoNeutral  -value - forwardSpeed; // Servo
			// PWCD  = RightServoNeutral -value + (forwardSpeed*53/100); 
			 
			// PFRUN = 1;
			// PCRUN = 1;  
// }

// void ServoStop(void){ 
			// PFRUN = 0;
			// PCRUN = 0;
			
			// PWF0D = 770;//779;
			// PWCD  = 780;//769;
			
			// PFRUN = 1;
			// PCRUN = 1;  
// }
// void GoStraight(void){ 
			// PFRUN = 0;
			// PCRUN = 0;
			
			// PWF0D = 779 + forwardSpeed;  
			// PWCD  = 769 - forwardSpeed; 
			
			// PFRUN = 1;
			// PCRUN = 1;  
// }
 
int Color_Detection_DEMO(){
	int tempo = 0; 
	
// Blue COLOR TEST   
	configH = 0.482;
	configS = 0.533;
	configL = 0.007;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 6;  
	configH = 0.485;
	configS = 0.546;
	configL = 0.008;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 9; 
// Green COLOR TEST 
	configH = 0.319;
	configS = 0.600;
	configL = 0.015;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 13;  
	configH = 0.318;
	configS = 0.589;
	configL = 0.014;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 14; 
// red COLOR TEST 
	configH = 1.020;
	configS = 0.549;
	configL = 0.011;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 15;  
	configH = 1.013;
	configS = 0.556;
	configL = 0.011;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 16;  
	configH = 1.014;
	configS = 0.565;
	configL = 0.010;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 17;   
	  
	  
	  
	   
// RED COLOR TEST
	configH = 0.161;
	configS = 0.349;
	configL = 0.018;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 1;

	configH = 0.175;
	configS = 0.346;
	configL = 0.019;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 2;
 
	configH = 0.192;
	configS = 0.378;
	configL = 0.020;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 3;
 
	configH = 0.199;
	configS = 0.354;
	configL = 0.021;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 4;
	 
	
// GREEN COLOR TEST
	configH = 0.324;
	configS = 0.585;
	configL = 0.027;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 5;
 
 
	configH = 0.324;
	configS = 0.560;
	configL = 0.030;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 7;
 
	configH = 0.334;
	configS = 0.564;
	configL = 0.031;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 8;
	 
	 
	 
// BLUE COLOR TEST 
	configH = 0.438;
	configS = 0.566;
	configL = 0.018;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 10;
 
	configH = 0.436;
	configS = 0.540;
	configL = 0.019;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 11;
 
	configH = 0.437;
	configS = 0.549;
	configL = 0.018;
	HL = (configH-colorTolerance);
	HH = (configH+colorTolerance);
	SL = (configS-colorTolerance);
	SH = (configS+colorTolerance);
	LL = (configL-2);
	LH = (configL+2);
	if( HL < hsl_avg.H && hsl_avg.H < HH )
		if( SL < hsl_avg.S  && hsl_avg.S  < SH )
			if( LL < hsl_avg.L  && hsl_avg.L  < LH )
				return 12;
	 
	return 0;
}

void OutputPWM(void){ 
//Step 1: Set Pin Direction...
//Step 2: Set Pin I/O Type...
//Step 3: Set Pin Purpose...
//Step 4: Select the Clock Mode...
//Step 5: Set the Duty Cycle...
//Step 5: Start the PWM Counter...

//The PWM signals with the periods of approximately 122 ns (@PLLCLK=16.384MHz) to 2s (@LSCLK=32.768kHz)
//  can be generated and output outside of this micro!

      //Direction...    
      PB7DIR = 0;       // PortB Bit0 set to Output Mode...
      PC0DIR = 0;       // PortB Bit0 set to Output Mode...
	  
	  /*
	  PC1DIR = 0;
	  PC2DIR = 0;
	  PA2DIR = 0;
	  */
	  
      //I/O Type...
      PB7C1  = 1;       // PortB Bit0 set to CMOS Output...
      PB7C0  = 1;    
      PC0C1  = 1;       // PortB Bit0 set to CMOS Output...
      PC0C0  = 1;    
	  
	  /*
	  PA2C1  = 1;
	  PA2C0  = 1; 
	  PC1C1  = 1;
	  PC1C0  = 1; 
	  PC2C1  = 1;
	  PC2C0  = 1; 
	  */
	  
      //Purpose...
      PB7MD1  = 1;            // PortC Bit0 set to PWM Output (1,0)...
      PB7MD0  = 1;   
      PC0MD1  = 1;            // PortC Bit0 set to PWM Output (1,0)...
      PC0MD0  = 0;      
	
	  /*
      PA2MD1  = 0;            // PortA Bit0 set to PWM Output (1,0)...
      PA2MD0  = 1;    
      PC1MD1  = 1;            // PortA Bit0 set to PWM Output (1,0)...
      PC1MD0  = 0;    
      PC2MD1  = 1;            // PortA Bit0 set to PWM Output (1,0)...
      PC2MD0  = 0;      
      */

      //Select the Clock Mode...
      PFCS1 = 0;        //00= LS; 01=HS; 10=PLL
      PFCS0 = 1;
      PCCS1 = 0;        //00= LS; 01=HS; 10=PLL
      PCCS0 = 1;

	  //PECS1 = 0;
	  //PECS0 = 1;
	    
      //SET THE PERIOD...(Added June 4th, 2013)
      PWFP = 27000;            // Init Period to (1=255kHz; 10=46kHz; 50=10kHz; 200=2.5kH; ; 3185 = 160Hz; 3400=150Hz; 4250=120Hz; 5000=102Hz)
	  PWCP = 27000;
      //SET THE DUTY CYCLE...(Added June 15th, 2013)

      //PWCD =    10;         //10    ~  0.2  % duty cycle @ 120Hz
      //PWCD =   100;         //100   ~  2.4  % duty cycle @ 120Hz
      //PWCD =  1000;         //1000  ~ 23.5  % duty cycle @ 120Hz
      PWF0D =  10000;         //4000  ~ 94.0  % duty cycle @ 120Hz
	  PWCD = 10000;
      //PWF1D =  10000;         // G
      //PWF2D =  10000;         // G
	  //PWED =  10000;          // R
      //PWCD =  4150;         //4150  ~ 99.0  % duty cycle @ 120Hz
      //PWCD =    20;         //20    ~  0.4  % duty cycle @ 120Hz      
      //PWF0D =    PWMSafeDuty;           //12    ~  0.25 % duty cycle @ 160Hz

      PFRUN = 0;        // OFF to start
      PCRUN = 0;        // OFF to start
}



//===========================================================================
//  	Start of Other Functions...
//===========================================================================
//==========================================================================
//	Initialize Micro to Desired State...
//===========================================================================
static void Initialization(void){

	//Initialize Peripherals	
	//BLKCON2 Control Bits...Manually Set 4/12/2013
	DSIO0 = 1; // 0=> Enables Synchronous Serial Port 0 (initial value).
	DUA0  = 0; // 0=> Enables the operation of UART0 (initial value).
	DUA1  = 0; // 0=> Enables Uart1 (initial value). 
	DI2C1 = 1; // 0=> Enables I2C bus Interface (Slave) (initial value).
	DI2C0 = 0; // 0=> Enables I2C bus Interface (Master) (initial value).	
	
	BLKCON4 = 0x0F; // 0=> Enables SA-ADC
	BLKCON6 = 0x00; // (1=disables; 0=enables) the operation of Timers 8, 9, A, E, F.
	BLKCON7 = 0x00; // (1=disables; 0=enables) the operation of PWM (PWMC, PWMD, PWME, PWMF

	// Port Initialize
	PortA_Low();	//Initialize all 3 Ports of Port A to GPIO-Low
	PortB_Low();	//Initialize all 8 Ports of Port B to GPIO-Low
	PortC_Low();	//Initialize all 8 Ports of Port C to GPIO-Low
	PortD_Low();	//Initialize all 6 Ports of Port D to input GPIO
	
	// Set Oscillator Rate
    SetOSC();
	
	/*
	// Settings for the ADC input (A0, A1)
	PA0DIR = 0;
	PA1DIR = 0;		//GPIO Input
	SACH0 = 0;		//This enables the ADC Channel 0 from A0 Pin
	SACH1 = 0;		//This enables the ADC Channel 1 from A1 Pin
	SALP = 0;		//Single Read or Continuous Read... Single = 0, Consecutive = 1
	*/
	
	// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
	// INTERRUPT SETUP...
	irq_di();	// Disable Interrupts
	irq_init();	// Initialize Interrupts (All Off and NO Requests)

	// INTERRUPT ENABLE REGISTERS...
	IE0 = IE1 = IE2 = IE3 = IE4 = IE5 = IE6 = IE7 = 0;
	// INTERRUPT REQUEST REGISTERS...
	IRQ0 = IRQ1 = IRQ2 = IRQ3 = IRQ4 = IRQ5 = IRQ6 = IRQ7 = 0;

	E2H = 0;	// E2H is the Enable flag for 2Hz TBC Interrupt (1=ENABLED)
				
	irq_setHdr((unsigned char)IRQ_NO_UA0INT, _intUart);
	EUA0 = 1; 	// EUA0 is the enable flag for the UART0 interrupt (1=ENABLED)
	
	irq_setHdr((unsigned char)IRQ_NO_I2CMINT, _intI2c);
	EI2CM = 1;
	QI2CM = 0;
	
	/*
	//Enable ADC Interrupts Handler
	irq_setHdr((unsigned char)IRQ_NO_SADINT, _intADC);
	ESAD = 1;
	QSAD = 0;
	*/
	
	/*
	//Set up xHz TBC Interrupt (Options: 128Hz, 32Hz, 16Hz, 2Hz)
	//(void)irq_setHdr( (unsigned char)IRQ_NO_T2HINT, TBC_ISR );  //Clear interrupt request flag
	
	// TBC...Set Ratio: : 1:1 => 1_1
	(void)tb_setHtbdiv( (unsigned char)TB_HTD_1_1 ); //Set the ratio of dividing frequency of the time base counter
		E2H = 0;	  // Enable x Hz TBC Interrupt (1=ENABLED)
		Q2H = 0;	  // Request flag for the time base counter x Hz interrupt	
	*/
	
	irq_ei(); // Enable Interrupts
	// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII

	// WDT... This will be the triggering condition to return from halt mode
	// We will need to calibrate based on the timing of our loop
	// 0x00 = 125ms
	// 0x01 = 500ms
	// 0x02 = 2sec
	// 0x03 = 8sec
	// 0x04 = 23.4ms
	// 0x05 = 31.25ms
	// 0x06	= 62.5ms
	WDTMOD = 0x01; 	
	main_clrWDT(); 	// Clear WDT
	
	//I2C Initialization...
	//P20C0 = 1;	/* CMOS output */
	//P20C1 = 1;	
	//P20D = 1;		/* write protect enable */
	i2c_init(I2C_MOD_FST, (unsigned short)HSCLK_KHZ, I2C_SYN_OFF);
	
	//UART Initialization...
	uart_init((unsigned char)UART_CS_HSCLK,		/* Generator       */
			  (unsigned short)HSCLK_KHZ,		/* HSCLK frequency */
			   &_uartSetParam );				/* Param... 	 */
	uart_PortSet();
	
	//Write "Program Start" UART Control
	
	ETM8 = 0; //Turn OFF TIMER8/9 ISR for this function...
	ETM9 = 0; //Turn OFF TIMER8/9 ISR for this function... 

}//End Initialization
//===========================================================================

/*******************************************************************************
	Routine Name	: write
	Form			: int write(int handle, unsigned char *buffer, unsigned int len)
	Parameters		: int handle
					  unsigned char *buffer
					  unsigned int len
	Return value	: int
	Initialization	: None.
	Description		: The write function writes len bytes of data from the area specified by buffer to UART0.
******************************************************************************/
int write(int handle, unsigned char *buffer, unsigned int len)
{
	_flgUartFin = 0; 
	uart_stop();
	uart_startSend(buffer, len, _funcUartFin); 
	/*
	while(_flgUartFin != 1)
	{
		main_clrWDT();
	}*/
	return len;
}

/*******************************************************************************
	Routine Name	: ADC_Read
	Form			: int ADC_Read()
	Parameters		: unsigned char idx
	Return value	: int
	Initialization	: None.
	Description		: Read ADC(idx) value
******************************************************************************/
int ADC_Read(unsigned char idx)
{
	_flgADCFin = 0;
	SADMOD0 = (unsigned char)(1<<idx);
	SARUN = 1;
	while(_flgADCFin == 0)
	{
		main_clrWDT();
	}
	switch(idx)
	{
		case 0:		return (SADR0H<<2|SADR0L>>6);
		case 1:		return (SADR1H<<2|SADR1L>>6);
		case 2:		return (SADR2H<<2|SADR2L>>6);
		case 3:		return (SADR3H<<2|SADR3L>>6);
		case 4:		return (SADR4H<<2|SADR4L>>6);
		case 5:		return (SADR5H<<2|SADR5L>>6);
		case 6:		return (SADR6H<<2|SADR6L>>6);
		case 7:		return (SADR7H<<2|SADR7L>>6);
		default:	return 0;
	}
}

/*******************************************************************************
	Routine Name	: I2C_Read
	Form			: void I2C_Read(unsigned char slave_address, unsigned char *address, unsigned char address_size, unsigned char *buffer, unsigned char size)
	Parameters		: unsigned char slave_address
					  unsigned char *address
					  unsigned char address_size
					  unsigned char *buffer
					  unsigned char size
	Return value	: void
	Initialization	: None.
	Description		: 
******************************************************************************/
void I2C_Read(unsigned char slave_address, unsigned char *reg_address, unsigned char reg_address_size, unsigned char *buffer, unsigned char size)
{
	_flgI2CFin = 0;
	i2c_stop();	
	i2c_startReceive(slave_address, reg_address, reg_address_size, buffer, size, (cbfI2c)_funcI2CFin);
	while(_flgI2CFin != 1)
	{
		main_clrWDT();
	}
}

/*******************************************************************************
	Routine Name	: I2C_Write
	Form			: void I2C_Write(unsigned char slave_address, unsigned char *address, unsigned char address_size, unsigned char *buffer, unsigned char size)
	Parameters		: unsigned char slave_address
					  unsigned char *address
					  unsigned char address_size
					  unsigned char *buffer
					  unsigned char size
	Return value	: void
	Initialization	: None.
	Description		: 
******************************************************************************/
void I2C_Write(unsigned char slave_address, unsigned char *reg_address, unsigned char reg_address_size, unsigned char *buffer, unsigned char size)
{
	_flgI2CFin = 0;
	i2c_stop();	
	i2c_startSend(slave_address, reg_address, reg_address_size, buffer, size, (cbfI2c)_funcI2CFin);
	while(_flgI2CFin != 1)
	{
		main_clrWDT();
	}
}
 
/*******************************************************************************
	Routine Name:	main_clrWDT
	Form:			void main_clrWDT( void )
	Parameters:		void
	Return value:	void
	Description:	clear WDT.
******************************************************************************/

void main_clrWDT( void )
{
	//How to clear the Watch Dog Timer:
	// => Write alternately 0x5A and 0xA5 into WDTCON register
	do {
		WDTCON = 0x5Au;
	} while (WDP != 1);
	WDTCON = 0xA5u;
}

/*******************************************************************************
	Routine Name:	_funcUartFin
	Form:			static void _funcUartFin( unsigned int size, unsigned char errStat )
	Parameters:		unsigned int size		 : 
				unsigned char errStat	 : 
	Return value:	void
	Description:	UART transmission completion callback function.
******************************************************************************/
static void _funcUartFin( unsigned int size, unsigned char errStat )
{
	uart_continue();					// Function in UART.c: process to continue send and receive...
	_flgUartFin = (unsigned char)FLG_SET;
	main_reqNotHalt();				// uncommented 5/2/2013
}

/*******************************************************************************
	Routine Name:	_funcI2CFin
	Form:			static void _funcUartFin( unsigned int size, unsigned char errStat )
	Parameters:		unsigned int size		 : 
				unsigned char errStat	 : 
	Return value:	void
	Description:	UART transmission completion callback function.
******************************************************************************/
static void _funcI2CFin( unsigned int size, unsigned char errStat )
{
	i2c_continue();					// Function in UART.c: process to continue send and receive...
	_flgI2CFin = (unsigned char)FLG_SET;
	main_reqNotHalt();				// uncommented 5/2/2013
}

/*******************************************************************************
	Routine Name:	_intI2c
	Form:			static void _intI2c( void )
	Parameters:		void
	Return value:	void
	Description:	I2C handler.
******************************************************************************/
static void _intI2c( void )
{
	i2c_continue();
	main_reqNotHalt();
}

/*******************************************************************************
	Routine Name:	_intADC
	Form:			static void _intADC( void )
	Parameters:		void
	Return value:	void
	Description:	I2C handler.
******************************************************************************/
static void _intADC( void )
{
	_flgADCFin = 1;
}

/*******************************************************************************
	Routine Name:	main_reqNotHalt
	Form:			void reqNotHalt( void )
	Parameters:		void
	Return value:	void
	Description:	request not halt.
******************************************************************************/
void main_reqNotHalt( void )
{
	_reqNotHalt = (unsigned char)FLG_SET;
}

/*******************************************************************************
	Routine Name:	_intUart
	Form:			static void _intUart( void )
	Parameters:		void
	Return value:	void
	Description:	UART handler.
******************************************************************************/
static void _intUart( void )
{
	uart_continue(); 	//in UART.c: process to continue send and receive...
}

//===========================================================================
//	OSC set
//===========================================================================
static void SetOSC(void){

	//FCON0: 			// xMHz PLL (3=1MHz; 2=2MHz; 1=4MHz; 0=8MHz)...
	SYSC0 = 0;			// Used to select the frequency of the HSCLK => 00=8.192MHz.
	SYSC1 = 0;

	OSCM1 = 1;			// 10 => Built-in PLL oscillation mode
	OSCM0 = 0;
   	
	ENOSC = 1;			//1=Enable High Speed Oscillator...
	SYSCLK = 1;			//1=HSCLK; 0=LSCLK 

	LPLL = 1;			//1=Enables the use of PLL oscillation - ADDED 4/30/2013

	__EI();			//INT enable
}
//===========================================================================

//===========================================================================
//	Clear All 3 Bits of Port A
//===========================================================================
void PortA_Low(void){

//Carl's Notes...

//Step 1: Set Pin Direction...
//Step 2: Set Pin I/O Type...
//Step 3: Set Pin Purpose...
//Step 4: Set Pin Data...

	//Direction...	
	PA0DIR = 0;		// PortA Bit0 set to Output Mode...
	PA1DIR = 0;		// PortA Bit1 set to Output Mode...
	PA2DIR = 0;		// PortA Bit2 set to Output Mode...

	//I/O Type...
	PA0C1  = 1;		// PortA Bit0 set to CMOS Output...
	PA0C0  = 1;		
	PA1C1  = 1;		// PortA Bit1 set to CMOS Output...
	PA1C0  = 1;	
	PA2C1  = 1;		// PortA Bit2 set to CMOS Output...
	PA2C0  = 1;	

	//Purpose...
	PA0MD1  = 0;	// PortA Bit0 set to General Purpose Output...
	PA0MD0  = 0;	
	PA1MD1  = 0;	// PortA Bit1 set to General Purpose Output...
	PA1MD0  = 0;	
	PA2MD1  = 0;	// PortA Bit2 set to General Purpose Output...
	PA2MD0  = 0;	

	//Data...
	PA0D = 0;		// A.0 Output OFF....
	PA1D = 0;		// A.1 Output OFF....
	PA2D = 0;		// A.2 Output OFF....

	main_clrWDT(); 	// Clear WDT
}
//===========================================================================

//===========================================================================
//	Clear All 8 Bits of Port B
//===========================================================================
void PortB_Low(void){

//Carl's Notes...

//Step 1: Set Pin Direction...
//Step 2: Set Pin I/O Type...
//Step 3: Set Pin Purpose...
//Step 4: Set Pin Data...

	//Direction...	
	PB0DIR = 0;		// PortB Bit0 set to Output Mode...
	PB1DIR = 0;		// PortB Bit1 set to Output Mode...
	PB2DIR = 0;		// PortB Bit2 set to Output Mode...
	PB3DIR = 0;		// PortB Bit3 set to Output Mode...
	PB4DIR = 0;		// PortB Bit4 set to Output Mode...
	PB5DIR = 0;		// PortB Bit5 set to Output Mode...
	PB6DIR = 0;		// PortB Bit6 set to Output Mode...
	PB7DIR = 0;		// PortB Bit7 set to Output Mode...

	//I/O Type...
	PB0C1  = 1;		// PortB Bit0 set to CMOS Output...
	PB0C0  = 1;		
	PB1C1  = 1;		// PortB Bit1 set to CMOS Output...
	PB1C0  = 1;	
	PB2C1  = 1;		// PortB Bit2 set to CMOS Output...
	PB2C0  = 1;	
	PB3C1  = 1;		// PortB Bit3 set to CMOS Output...
	PB3C0  = 1;		
	PB4C1  = 1;		// PortB Bit4 set to CMOS Output...
	PB4C0  = 1;	
	PB5C1  = 1;		// PortB Bit5 set to CMOS Output...
	PB5C0  = 1;	
	PB6C1  = 1;		// PortB Bit6 set to CMOS Output...
	PB6C0  = 1;	
	PB7C1  = 1;		// PortB Bit7 set to CMOS Output...
	PB7C0  = 1;	

	//Purpose...
	PB0MD1  = 0;	// PortB Bit0 set to General Purpose Output...
	PB0MD0  = 0;	
	PB1MD1  = 0;	// PortB Bit1 set to General Purpose Output...
	PB1MD0  = 0;	
	PB2MD1  = 0;	// PortB Bit2 set to General Purpose Output...
	PB2MD0  = 0;	
	PB3MD1  = 0;	// PortB Bit3 set to General Purpose Output...
	PB3MD0  = 0;	
	PB4MD1  = 0;	// PortB Bit4 set to General Purpose Output...
	PB4MD0  = 0;	
	PB5MD1  = 0;	// PortB Bit5 set to General Purpose Output...
	PB5MD0  = 0;
	PB6MD1  = 0;	// PortB Bit6 set to General Purpose Output...
	PB6MD0  = 0;	
	PB7MD1  = 0;	// PortB Bit7 set to General Purpose Output...
	PB7MD0  = 0;

	//Data...
	PB0D = 0;		// B.0 Output OFF....
	PB1D = 0;		// B.1 Output OFF....
	PB2D = 0;		// B.2 Output OFF....
	PB3D = 0;		// B.3 Output OFF....
	PB4D = 0;		// B.4 Output OFF....
	PB5D = 0;		// B.5 Output OFF....
	PB6D = 0;		// B.6 Output OFF....
	PB7D = 0;		// B.7 Output OFF....

	main_clrWDT(); 	// Clear WDT
}
//===========================================================================

//===========================================================================
//	Clear All 8 Bits of Port C
//===========================================================================
void PortC_Low(void){

//Carl's Notes...

//Step 1: Set Pin Direction...
//Step 2: Set Pin I/O Type...
//Step 3: Set Pin Purpose...
//Step 4: Set Pin Data...

	//Direction...	
	PC0DIR = 0;		// PortC Bit0 set to Output Mode...
	PC1DIR = 0;		// PortC Bit1 set to Output Mode...
	PC2DIR = 0;		// PortC Bit2 set to Output Mode...
	PC3DIR = 0;		// PortC Bit3 set to Output Mode...
	PC4DIR = 0;		// PortC Bit4 set to Output Mode...
	PC5DIR = 0;		// PortC Bit5 set to Output Mode...
	PC6DIR = 0;		// PortC Bit6 set to Output Mode...
	PC7DIR = 0;		// PortC Bit7 set to Output Mode...

	//I/O Type...
	PC0C1  = 1;		// PortC Bit0 set to High-Impedance Output...
	PC0C0  = 1;		
	PC1C1  = 1;		// PortC Bit1 set to High-Impedance Output...
	PC1C0  = 1;	
	PC2C1  = 1;		// PortC Bit2 set to High-Impedance Output...
	PC2C0  = 1;	
	PC3C1  = 1;		// PortC Bit3 set to High-Impedance Output...
	PC3C0  = 1;		
	PC4C1  = 1;		// PortC Bit4 set to High-Impedance Output...
	PC4C0  = 1;	
	PC5C1  = 1;		// PortC Bit5 set to High-Impedance Output...
	PC5C0  = 1;	
	PC6C1  = 1;		// PortC Bit6 set to High-Impedance Output...
	PC6C0  = 1;	
	PC7C1  = 1;		// PortC Bit7 set to High-Impedance Output...
	PC7C0  = 1;	

	//Purpose...
	PC0MD1  = 0;	// PortC Bit0 set to General Purpose Output...
	PC0MD0  = 0;	
	PC1MD1  = 0;	// PortC Bit1 set to General Purpose Output...
	PC1MD0  = 0;	
	PC2MD1  = 0;	// PortC Bit2 set to General Purpose Output...
	PC2MD0  = 0;	
	PC3MD1  = 0;	// PortC Bit3 set to General Purpose Output...
	PC3MD0  = 0;	
	PC4MD1  = 0;	// PortC Bit4 set to General Purpose Output...
	PC4MD0  = 0;	
	PC5MD1  = 0;	// PortC Bit5 set to General Purpose Output...
	PC5MD0  = 0;
	PC6MD1  = 0;	// PortC Bit6 set to General Purpose Output...
	PC6MD0  = 0;	
	PC7MD1  = 0;	// PortC Bit7 set to General Purpose Output...
	PC7MD0  = 0;

	//Data...
	PC0D = 0;		// C.0 Output OFF....
	PC1D = 0;		// C.1 Output OFF....
	PC2D = 0;		// C.2 Output OFF....
	PC3D = 0;		// C.3 Output OFF....
	PC4D = 0;		// C.4 Output OFF....
	PC5D = 0;		// C.5 Output OFF....
	PC6D = 0;		// C.6 Output OFF....
	PC7D = 0;		// C.7 Output OFF....

	main_clrWDT(); 	// Clear WDT

}
//===========================================================================

//===========================================================================
//	Clear All 6 Bits of Port D
//===========================================================================
void PortD_Low(void){

	//Carl's Notes...

	//Step 1: Set Pin Direction...
	//Step 2: Set Pin I/O Type...
	//Step 3: Set Pin Data...

	//Direction...	
	PD0DIR = 0;		// PortD Bit0 set to Input Mode...
	PD1DIR = 0;		// PortD Bit1 set to Input Mode...
	PD2DIR = 0;		// PortD Bit2 set to Input Mode...
	PD3DIR = 0;		// PortD Bit3 set to Input Mode...
	PD4DIR = 0;		// PortD Bit4 set to Input Mode...
	PD5DIR = 0;		// PortD Bit5 set to Input Mode...

	//I/O Type...
	PD0C1= 1;		// PortD Bit0 set to High-impedance input...
	PD0C0= 1;		
	PD1C1= 1;		// PortD Bit1 set to High-impedance input...
	PD1C0= 1;	
	PD2C1= 1;		// PortD Bit2 set to High-impedance input...
	PD2C0= 1;	
	PD3C1= 1;		// PortD Bit3 set to High-impedance input...
	PD3C0= 1;		
	PD4C1= 1;		// PortD Bit4 set to High-impedance input...
	PD4C0= 1;	
	PD5C1= 1;		// PortD Bit5 set to High-impedance input...
	PD5C0= 1;	

	//Data...
	PD0D = 0;		// D.0 Input OFF....
	PD1D = 0;		// D.1 Input OFF....
	PD2D = 0;		// D.2 Input OFF....
	PD3D = 0;		// D.3 Input OFF....
	PD4D = 0;		// D.4 Input OFF....
	PD5D = 0;		// D.5 Input OFF....

	main_clrWDT(); 	// Clear WDT
}
//===========================================================================

/*******************************************************************************
	Routine Name:	NOPms
	Form:			void NOP1000( unsigned int ms )
	Parameters:		unsigned int sec = "Number of seconds where the device is not doing anything"
	Return value:	void
	Description:	NOP for x seconds. Uses HTB* clock (512kHz) and timer 8+9 (max 0xFFFF)
					*(HTBCLK = 1/16 * HSCLK = (1/16)*8192kHz = 512kHz, see HTBDR to change if we need an even smaller increment timer...)
					1/(512kHz) * 0xFFFF = 127ms
					
******************************************************************************/
void NOPms( unsigned int ms )
{
	unsigned int timerThres;
	unsigned char TimeFlag;
	unsigned int TempSec;
	unsigned int timer;
	unsigned int timertest;

	TempSec = ms;
	TimeFlag = 0;

	tm_init(TM_CH_NO_AB);
	tm_setABSource(TM_CS_HTBCLK);
	tm_setABData(0xffff);

	if(ms < 128){
		timerThres = 0x1FF * ms;
		TimeFlag = 0;
	}
	if(ms == 128){
		timerThres = 0xFFFF;
		TimeFlag = 0;
	}
	if(ms > 128){
		while(TempSec > 128){
			TempSec -= 128;
			TimeFlag++;
		}
		if(TempSec != 0){
			timerThres = 0x1FF * TempSec;
		}
		else{
			timerThres = 0xFFFF;
			TimeFlag--;
		}
	}

TimerRestart:
	main_clrWDT();	
	//tm_restart89();	//using LSCLK, the maximum delay time we have is ~2 secs
	tm_startAB();
	timer = tm_getABCounter();
	while(timer < timerThres){
		timer = tm_getABCounter();
		timertest = timer;
	}
	if(TimeFlag !=0){
		tm_stopAB();
		TimeFlag--;
		timerThres = 0xFFFF;
		goto TimerRestart;
	}
}
	
/*******************************************************************************
	Routine Name:	ReverseBits
	Form:			unsigned char ReverseBits(unsigned char data)
	Parameters:		unsigned char data
	Return value:	unsigned char
	Description:	Reverse bits order of data
******************************************************************************/	
unsigned char ReverseBits(unsigned char data)
{
__asm("\n\
	MOV r1,r0\n\
	MOV r0,#0\n\
	MOV r2,#8\n\
_ReverseBits_loop:\n\
	SLL r0,#1\n\
	SRL r1,#1\n\
	BGE _ReverseBits_next\n\
	OR r0,#1\n\
_ReverseBits_next:\n\
	ADD	r2,	#0ffh\n\
	CMP	r2,	#00h\n\
	BGT _ReverseBits_loop\n\
");
}

void RED_ON(){
	PC1D = 0;
	PC2D = 1;
	PA2D = 1; 
}
void GREEN_ON(){
	PC1D = 1;
	PC2D = 0;
	PA2D = 1; 
}
void BLUE_ON(){
	PC1D = 1;
	PC2D = 1;
	PA2D = 0; 
}
							
/*******************************************************************************
	Routine Name:	FlashLEDs
	Form:			unsigned char FlashLEDs(unsigned char data)
	Parameters:		unsigned char data
	Return value:	unsigned char
	Description:	Flash LEDs instead of always ON
******************************************************************************/	
void FlashLEDs(void)
{
	main_clrWDT();
	//LEDs are OFF at the end of this routines
	
	LED_ON();
	PA0D = 1;
	NOPms(20);
	PA0D = 0;
	NOPms(20);
	PA0D = 1;
	NOPms(20);
	PA0D = 0;
	NOPms(20);
	PA0D = 1;
	NOPms(20);
	PA0D = 0;
	NOPms(20);
	PA0D = 1;
	NOPms(20);
	PA0D = 0;
	NOPms(20);
	
	RED_ON(); 
	NOPms(100);
	NOPms(100);
	NOPms(100);
	NOPms(100);
	NOPms(100);
	GREEN_ON();
	NOPms(100);
	NOPms(100);
	NOPms(100);
	NOPms(100);
	NOPms(100);
	BLUE_ON();
	NOPms(100);
	NOPms(100);
	NOPms(100);
	NOPms(100);
	NOPms(100);
	RGB_OFF();
	NOPms(100);
	NOPms(100);
	NOPms(100);
	NOPms(100);
	NOPms(100); 
	LED_OFF();
}

void RGB_OFF(){
	PC1D = 1;
	PC2D = 1;
	PA2D = 1; 
}

void LED_ON(){
	PA0D = 1;
	NOPms(50); 
}
void LED_OFF(){
	NOPms(50);
	PA0D = 0;
}
	