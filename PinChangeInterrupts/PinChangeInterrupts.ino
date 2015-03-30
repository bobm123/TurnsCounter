/*
Code Example showing the use a A0, A1 and A2 pins as interrupt driven inputs. From
http://www.geertlangereis.nl/Electronics/Pin_Change_Interrupts/PinChange_en.html

Normal Aruino code is restricted to 2 interrupts. This demostrates using AVR pin
change interrupts. The ide is to put all the interrupt inputs on one port, enable
the pin change interrupt and that checks which needs service and then calls 
the code to handle the specific pin.

*/

void setup()
{
  Serial.begin(115200);
  Serial.println("Pin Change Interrupt");
  InitialiseIO();
  InitialiseInterrupt();
}


void loop() {
  /* Nothing to do: the program jumps automatically to Interrupt Service Routine "blink"
   in case of a hardware interrupt  */
}  


void InitialiseIO(){
  pinMode(A0, INPUT);	   // Pin A0 is input to which a switch is connected
  digitalWrite(A0, HIGH);   // Configure internal pull-up resistor
  pinMode(A1, INPUT);	   // Pin A1 is input to which a switch is connected
  digitalWrite(A1, HIGH);   // Configure internal pull-up resistor
  pinMode(A2, INPUT);	   // Pin A2 is input to which a switch is connected
  digitalWrite(A2, HIGH);   // Configure internal pull-up resistor
}


void InitialiseInterrupt(){
  cli();		// switch interrupts off while messing with their settings  
  PCICR =0x02;          // Enable PCINT1 interrupt
  PCMSK1 = 0b00000111;  // Mask lower 3 bits
  sei();		// turn interrupts back on
}


ISR(PCINT1_vect) {
  // Interrupt service routine. Every single PCINT8..14 (=ADC0..5) change
  // will generate an interrupt: but this will always be the same interrupt routine
  if (digitalRead(A0)==0)  Serial.println("A0");
  if (digitalRead(A1)==0)  Serial.println("A1");
  if (digitalRead(A2)==0)  Serial.println("A2");
}
