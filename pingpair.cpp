/**
 * Example of using Ack Payloads
 *
 * This is an example of how to do two-way communication without changing
 * transmit/receive modes.  Here, a payload is set to the transmitter within
 * the Ack packet of each transmission.  Note that the payload is set BEFORE
 * the sender's message arrives.
 */

#include "nRF24L01.h"
#include "RF24.h"


// Hardware configuration
// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

// sets the role of this unit in hardware.
// Connect to GND to be the 'pong' receiver
// Leave open to be the 'ping' transmitter
const short role_pin = D5;


// Topology
// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

// Role management
// This sketch uses the same software for all the nodes in this
// system.  Doing so greatly simplifies testing.  The hardware itself specifies
// which node it is.
//
// This is done through the role_pin

// The various roles supported by this sketch
typedef enum { role_sender = 1, role_receiver } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Sender", "Receiver"};

// The role of the current running sketch
role_e role;

void setup(void)
{
  // set up the role pin
  pinMode(role_pin, INPUT);
  digitalWrite(role_pin,HIGH);
  delay(20); // Just to get a solid reading on the role pin

  // read the address pin, establish our role
  if ( digitalRead(role_pin) )
    role = role_sender;
  else
    role = role_receiver;

  // Print preamble
  Serial.begin(9600);
  Serial.println("nrRF24 single pipe ping test");
  Serial.print("ROLE: "); Serial.println(role_friendly_name[role]);

  
  // Setup and configure rf radio
    radio.begin();

  // We will be using the Ack Payload feature, so please enable it
  radio.enableAckPayload();

  // Open pipes to other nodes for communication
  // This simple sketch opens a single pipes for these two nodes to communicate
  // back and forth.  One listens on it, the other talks to it.
  if ( role == role_sender )
  {
    radio.openWritingPipe(pipe);
  }
  else
  {
    radio.openReadingPipe(1,pipe);
  }

  // Start listening
  if ( role == role_receiver )
    radio.startListening();

  // Dump the configuration of the rf unit for debugging
  radio.printDetails();
}

void loop(void)
{
  static uint32_t message_count = 0;

  // Sender role.  Repeatedly send the current time
  if (role == role_sender)
  {
    // Take the time, and send it.  This will block until complete
    unsigned long time = millis();
    Serial.print("Now sending "); Serial.print(time); Serial.println("...");
	
    radio.write( &time, sizeof(unsigned long) );

    if ( radio.isAckPayloadAvailable() )
    {
      radio.read(&message_count,sizeof(message_count));
      Serial.print("Ack: ["); Serial.print(message_count); Serial.println("]");
    }
   	Serial.println("OK");

    // Try again soon
    delay(2000);
  }

  // Receiver role.  Receive each packet, dump it out, add ack payload for next time
  if ( role == role_receiver )
  {
    // if there is data ready
    if ( radio.available() )
    {
      // Dump the payloads until we've gotten everything
      static unsigned long got_time;
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read( &got_time, sizeof(unsigned long) );

        // Spew it
		Serial.print("Got payload "); Serial.println(got_time);
      }

      // Add an ack packet for the next time around.  This is a simple
      // packet counter
      radio.writeAckPayload( 1, &message_count, sizeof(message_count) );
      ++message_count;
    }
  }
}

