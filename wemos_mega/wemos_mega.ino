#include <Adafruit_MAX31865.h>

auto &MySerial = Serial3;

using watch_mask = uint32_t;

static const size_t max_max31865 = 4;
static const size_t max_watch = sizeof(watch_mask) * 8;

unsigned int allocated_max31865 = 0;
Adafruit_MAX31865 *max31865_array_max[max_max31865];
uint8_t max31865_array_pin[max_max31865];

unsigned int allocated_watch = 0;
uint8_t watch_array[max_watch];

watch_mask current_watch = 0;
unsigned int selected_pwm = 0;
String serial_read;

void (*reset)(void) = 0;

void setup() {
  allocated_max31865 = 0;
  allocated_watch = 0;
  
  MySerial.begin(115200);
  MySerial.println("#Ready");
}

void set_mask(watch_mask *mask, size_t bit_number) {
  watch_mask bit = 1 << bit_number;
  if(digitalRead(watch_array[bit_number]) == HIGH)
      *mask |= bit;
    else
      *mask &= ~bit;
}

void loop() {
  watch_mask read_watch = 0;
  for(size_t i = 0; i < allocated_watch; i++)
    set_mask(&read_watch, i);
  if(read_watch != current_watch) {
    current_watch = read_watch;
    MySerial.print(">"); MySerial.print(current_watch); MySerial.print("\n");
  }
  char c = MySerial.read();
  if(c == -1)
    return;
  if(c != '\n') {
    if(c != '\r')
	  serial_read += c;
  }
  else if(serial_read.length() > 0) {
    unsigned int p = atoi(&serial_read[1]);
    size_t i;
    switch(serial_read[0]) {
    case 'I':
    case 'O':
      pinMode(p, serial_read[0] == 'I' ? INPUT_PULLUP: OUTPUT);
      MySerial.print("=\n");
      break;
    case 'H':
    case 'L':
      digitalWrite(p, serial_read[0] == 'H' ? HIGH : LOW);
      MySerial.print("=\n");
      break;
    case 'R':
      MySerial.print(digitalRead(p) == HIGH ? 1 : 0); MySerial.print("\n");
      break;
    case 'A':
      MySerial.print(analogRead(p)); MySerial.print("\n");
      break;
    case 'P':
      selected_pwm = p;
      MySerial.print("=\n");
      break;
    case 'V':
      analogWrite(selected_pwm, p);
      MySerial.print("=\n");
      break;
    case 'C':
      for(i = 0; i < allocated_max31865; i++) {
        if(max31865_array_pin[i] == p)
          break;
      }
      if(i == max_max31865) {
        MySerial.print("! Max exceeded\n");
		break;
	  }
      else if(i == allocated_max31865) {
        max31865_array_pin[allocated_max31865] = p;
        max31865_array_max[allocated_max31865] = new Adafruit_MAX31865(p);
        max31865_array_max[allocated_max31865++]->begin(MAX31865_2WIRE);
      }
      MySerial.print(i); MySerial.print("\n");
      break;
    case 'T':
      if(p >= allocated_max31865)
        MySerial.print("! Wrong reference\n");
      else
        MySerial.print(max31865_array_max[p]->temperature(100., 430.)); MySerial.print("\n");
      break;
    case 'W':
      for(i = 0; i < allocated_watch; i++) {
        if(watch_array[i] == p)
          break;
      }
      if(i == max_watch) {
        MySerial.print("! Max exceeded\n");
		break;
	  }
      else if(i == allocated_watch) {
        watch_array[allocated_watch] = p;
        set_mask(&current_watch, allocated_watch++);
      }
      MySerial.print(i); MySerial.print("\n");
      break;
    case '@':
      for(size_t i = 0; i < allocated_watch; i++) {
        MySerial.print("W "); MySerial.print(i); MySerial.print(": "); MySerial.print(watch_array[i]); MySerial.print("\n");
      }
      for(size_t i = 0; i < allocated_max31865; i++) {
        MySerial.print("C "); MySerial.print(i); MySerial.print(": "); MySerial.print(max31865_array_pin[i]); MySerial.print("\n");
      }
      MySerial.print("@\n");
      break;
	case '-':
	  MySerial.print("-\n");
	  break;
	case '#':
      MySerial.print("=\n");
	  reset();
	  break;
    case '?':
      MySerial.print(
        "Configuration:\n"
        "Ix: configure pin x in input mode\n    returns '='\n"
        "Ox: configure pin x in output mode\n    returns '='\n"
        "Wx: configure pin x in watch state\n    returns bit number affected to status\n"
        "Cx: configure pin x as MAX31865 CS pin\n    returns reference to use\n"
        "Selection:\n"
        "Px: select selected_pwm pin x\n    returns '='\n"
        "Action:\n"
        "Hx: set pin x in HIGH state\n    returns '='\n"
        "Lx: set pin x in LOW state\n    returns '='\n"
        "Vp: send value p to selected_pwm previously selected\n    returns '='\n"
        "Rx: read digital pin x\n    returns 1 for HIGH, 0 for LOW\n"
        "Ax: read analog pin x\n    returns read value\n"
        "Tr: read MAX31865 PT100 temperature at reference r\n    returns read temperature\n"
        "Limits:\n"
	  );
	  MySerial.print(max_max31865); MySerial.print(" MAX31865 devices\n");
	  MySerial.print(max_watch); MySerial.print(" watched input pins\n");
	  MySerial.print("?\n");
	  break;
    default:
      MySerial.print("! Unknown order\n");
      break;
    }
    serial_read.remove(0);
  }
}
