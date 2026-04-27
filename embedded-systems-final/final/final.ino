// Justin Gu, Jaeden Russell, Chloe Facchinetti
// CPE 301 Final Project

#define RDA 0x80
#define TBE 0x20

volatile unsigned char *myUCSR0A = (unsigned char *) 0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *) 0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *) 0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *)  0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *) 0x00C6;

volatile unsigned char *my_ADMUX    = (unsigned char *) 0x7C;
volatile unsigned char *my_ADCSRB   = (unsigned char *) 0x7B;
volatile unsigned char *my_ADCSRA   = (unsigned char *) 0x7A;
volatile unsigned int  *my_ADC_DATA = (unsigned int *)  0x78;

volatile unsigned char *my_DDRE  = (unsigned char *) 0x2D;
volatile unsigned char *my_PORTE = (unsigned char *) 0x2E;
volatile unsigned char *my_DDRG  = (unsigned char *) 0x33;
volatile unsigned char *my_PORTG = (unsigned char *) 0x34;
volatile unsigned char *my_DDRD  = (unsigned char *) 0x2A;
volatile unsigned char *my_PORTD = (unsigned char *) 0x2B;

#define GREEN_BIT     0x08
#define RED_BIT       0x20
#define BLUE_BIT      0x10
#define YELLOW_BIT    0x20
#define ON_BUTTON_BIT 0x08

#define MOISTURE_ERROR_LOW  200
#define MOISTURE_GOOD_MAX   380
#define MOISTURE_DRY_MAX    425

#define DEBUG_INTERVAL 3000

typedef enum {
  STATE_OFF,
  STATE_IDLE,
  STATE_ACTIVE,
  STATE_ERROR
} SystemState;

volatile SystemState currentState = STATE_OFF;
volatile bool onButtonPressed = false;
unsigned long lastDebugPrint = 0;

void onButtonISR()
{
  if (currentState == STATE_OFF)
  {
    onButtonPressed = true;
  }
}

void setup()
{
  U0init(9600);
  adc_init();

  *my_DDRE |= GREEN_BIT;
  *my_DDRE |= RED_BIT;
  *my_DDRE |= BLUE_BIT;
  *my_DDRG |= YELLOW_BIT;

  *my_DDRD  &= ~ON_BUTTON_BIT;
  *my_PORTD |=  ON_BUTTON_BIT;
  attachInterrupt(digitalPinToInterrupt(18), onButtonISR, FALLING);

  setLED(STATE_OFF);
  printString("System OFF. Press ON button to start.\n");
}

void loop()
{
  if (onButtonPressed)
  {
    onButtonPressed = false;
    currentState = STATE_IDLE;
    setLED(STATE_IDLE);
    printString("ON pressed - now in IDLE\n");
  }

  if (currentState == STATE_OFF)
  {
    return;
  }

  unsigned int moisture = adc_read(0);

  unsigned long now = millis();
  if (now - lastDebugPrint >= DEBUG_INTERVAL)
  {
    lastDebugPrint = now;
    printString("Moisture: ");
    printNumber(moisture);
    printString("\n");
  }

  SystemState newState;
  if (moisture < MOISTURE_ERROR_LOW || moisture > MOISTURE_DRY_MAX)
  {
    newState = STATE_ERROR;
  }
  else if (moisture > MOISTURE_GOOD_MAX)
  {
    newState = STATE_ACTIVE;
  }
  else
  {
    newState = STATE_IDLE;
  }

  if (newState != currentState)
  {
    currentState = newState;
    setLED(currentState);
    logStateChange(currentState, moisture);
  }
}

void setLED(SystemState state)
{
  *my_PORTE &= ~GREEN_BIT;
  *my_PORTE &= ~RED_BIT;
  *my_PORTE &= ~BLUE_BIT;
  *my_PORTG &= ~YELLOW_BIT;

  if (state == STATE_OFF)
  {
    *my_PORTE |= RED_BIT;
  }
  else if (state == STATE_IDLE)
  {
    *my_PORTE |= GREEN_BIT;
  }
  else if (state == STATE_ACTIVE)
  {
    *my_PORTG |= YELLOW_BIT;
  }
  else if (state == STATE_ERROR)
  {
    *my_PORTE |= BLUE_BIT;
  }
}

void logStateChange(SystemState state, unsigned int moisture)
{
  if (state == STATE_IDLE)
  {
    printString("State: IDLE  moisture: ");
  }
  else if (state == STATE_ACTIVE)
  {
    printString("State: ACTIVE  moisture: ");
  }
  else if (state == STATE_ERROR)
  {
    printString("State: ERROR  moisture: ");
  }
  printNumber(moisture);
  printString("\n");
}

void adc_init()
{
  *my_ADCSRA |= 0x80;
  *my_ADCSRA &= ~0x20;
  *my_ADCSRA &= ~0x08;
  *my_ADCSRA &= ~0x07;
  *my_ADCSRB &= ~0x08;
  *my_ADCSRB &= ~0x07;
  *my_ADMUX  &= ~0x80;
  *my_ADMUX  |=  0x40;
  *my_ADMUX  &= ~0x20;
  *my_ADMUX  &= ~0x1F;
}

unsigned int adc_read(unsigned char channel)
{
  *my_ADMUX  &= ~0x1F;
  *my_ADCSRB &= ~0x08;
  *my_ADMUX  |= (channel & 0x07);
  *my_ADCSRA |= 0x40;
  while ((*my_ADCSRA & 0x40) != 0)
  {
  }
  return *my_ADC_DATA;
}

void U0init(unsigned long baud)
{
  unsigned long FCPU = 16000000;
  unsigned int tbaud = (FCPU / 16 / baud - 1);
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0  = tbaud;
}

void printString(const char *str)
{
  int i = 0;
  while (str[i] != '\0')
  {
    while ((*myUCSR0A & TBE) == 0)
    {
    }
    *myUDR0 = str[i];
    i++;
  }
}

void printNumber(unsigned int val)
{
  if (val == 0)
  {
    while ((*myUCSR0A & TBE) == 0)
    {
    }
    *myUDR0 = '0';
    return;
  }

  char digits[5];
  int i = 0;
  while (val > 0)
  {
    digits[i] = (val % 10) + '0';
    val /= 10;
    i++;
  }

  for (int j = i - 1; j >= 0; j--)
  {
    while ((*myUCSR0A & TBE) == 0)
    {
    }
    *myUDR0 = digits[j];
  }
}