#include <JeeLib.h>

#define SEND_BUFFER_SIZE 200
#define INPUT_BUFFER_SIZE 200
#define LED_PIN 9

static char inputBuffer[INPUT_BUFFER_SIZE];
static unsigned int sendBuffer[SEND_BUFFER_SIZE];
static int sendBufferPointer = 0;

static void activityLed (byte on) {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, !on);
}

char readInput() {
  while (Serial.available() == 0) ;
  return Serial.readBytesUntil('\n', inputBuffer, INPUT_BUFFER_SIZE);
}

/*
 * Converts a hex string to a buffer. Not hex characters will be skipped
 * Returns the hex bytes found. Single-Nibbles wont be converted.
 */
int
fromhex(const char *in, unsigned char *out, int buflen)
{
  uint8_t *op = out, c, h = 0, fnd, step = 0;
  while((c = *in++)) {
    fnd = 0;
    if(c >= '0' && c <= '9') { h |= c-'0';    fnd = 1; }
    if(c >= 'A' && c <= 'F') { h |= c-'A'+10; fnd = 1; }
    if(c >= 'a' && c <= 'f') { h |= c-'a'+10; fnd = 1; }
    if(!fnd) {
      if(c != ' ')
        break;
      continue;
    }
    if(step++) {
      *op++ = h;
      if(--buflen <= 0)
        return (op-out);
      step = 0;
      h = 0;
    } else {
      h <<= 4;
    }
  }
  return op-out;
}

static const char hexString[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

char* printhex(int data) {
  Serial.print(hexString[(data >> 12) & 0xF]);
  Serial.print(hexString[(data >> 8) & 0xF]);
  Serial.print(hexString[(data >> 4) & 0xF]);
  Serial.println(hexString[(data) & 0xF]);
}


/**
 * Add a flank length to the send buffer
 */
void
addRawFlank(unsigned int flank) {
  // Check if it fits in one byte
  if (flank > (254 * 16)) {
    // No, signal two byte value by adding 255-byte
    sendBuffer[sendBufferPointer++] = 255;					// Mark long pulse
    sendBuffer[sendBufferPointer++] = flank >> (8 + 4);		// High byte
    sendBuffer[sendBufferPointer++] = (flank >> 4) & 0xFF;	// Lo Byte
  }
  else {
    // Flank fits in one byte
    sendBuffer[sendBufferPointer++] = (flank >> 4) & 0xFF;// One Byte
  }
}

/**
 * Add a new pulse to the transmit buffer.
 * \param in parameter string: "Ammmmssss" where "mmmm" is the mark flank length in us
 * in HEX format and "ssss" is the following space flank
 */
void
addRawPulse(char *in) {
  unsigned char pulseByte;
  unsigned int pulseWord;

  if (sendBufferPointer >= SEND_BUFFER_SIZE) {
    Serial.println('e');
    return;
  }

  // Add the Mark flank
  fromhex(in+1, &pulseByte, 1);					// Read high byte
  pulseWord = pulseByte << 8;
  fromhex(in+3, &pulseByte, 1);					// Read low byte
  pulseWord += pulseByte;
  addRawFlank(pulseWord);

  // Add the Space flank
  fromhex(in+5, &pulseByte, 1);					// Read high byte
  pulseWord = pulseByte << 8;
  fromhex(in+7, &pulseByte, 1);					// Read low byte
  pulseWord += pulseByte;
  addRawFlank(pulseWord);
  Serial.print('o');
  printhex(sendBufferPointer);
}

void printVersion() {
  Serial.println("V 1.0 JeeLink");
}

/**
 * Reset send buffer write pointer
 */
void
resetSendBuffer(char* in) {
  sendBufferPointer = 0;
  Serial.print('o');
  printhex(sendBufferPointer);
}

/**
 * Transmit the content of the sendBuffer via RF
 */
void
sendRawMessage(char* in) {
  delay(500);
  Serial.print('o');
  printhex(sendBufferPointer);
}

void setup() {
  Serial.begin(9600);
  Serial.println("RF Demo");
  // rf12_initialize(0, RF12_433MHZ);
  Serial.println("Initialized RF12");
}


void loop() {  
  char size = readInput();
  activityLed(1);
  if (inputBuffer[0] == 'A' && size >= 9) {
    addRawPulse(&inputBuffer[0]);
  } else if (inputBuffer[0] == 'S' && size >= 1) {
    sendRawMessage(&inputBuffer[0]);
  } else if (inputBuffer[0] == 'E' ) {
    resetSendBuffer(&inputBuffer[0]);
  } else if (inputBuffer[0] == 'V' ) {
    printVersion();
  } else {
    Serial.println("e");
  }
  Serial.flush();
  activityLed(0);  
}
