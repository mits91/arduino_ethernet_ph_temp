// Libraries
#include <Wire.h>                 //I2C communication
#include <LiquidCrystal_I2C.h>    //interfacing with I2C-based LCD displays
#include <DallasTemperature.h>    //interface with Dallas temperature sensors
#include <Ethernet.h>             //interface with ethernet

// Constants
#define pH_pin A0               //the analog pin connected to the pH sensor
#define ONE_WIRE_BUS 2          //the digital pin connected to the OneWire bus for temperature sensors
#define X 1.2                   //calibrating ph sensor
#define samplingInterval 20     //collect the value every 20ms
#define printInterval 1000      //print every 1000ms
#define ArrayLenth 50           //put the collect value in the array

int pHArray[ArrayLenth];                                    //Store the average value of the sensor feedback
int pHArrayIndex = 0;                                       //initialize the value
const LiquidCrystal_I2C lcd(0x27, 16, 2);                   //the I2C address of the LCD (0x27), the number of columns (16), and the number of rows (2)
const OneWire oneWire(ONE_WIRE_BUS);                        //the digital pin (defined earlier as 2) connected to the OneWire bus for temperature sensors
const DallasTemperature sensors(&oneWire);                  //allows the sensors object to communicate with Dallas temperature sensors using the OneWire bus

const byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};    //declares constant byte array initializes with the hexadecimal values typically used as the MAC address of the Ethernet module.
const IPAddress ip();                                       //declares constant IPAddress object called ip initializes  with the IP address ("") the desired IP address for the Ethernet module.
const IPAddress myDns();                                    //declares constant IPAddress object called myDns initializes with the IP address ("") the IP address of the DNS  server that will be used for name resolution.
EthernetClient client;                                      //declares an EthernetClient object called client, used to establish a client connection to a server over Ethernet.

const int    HTTP_PORT   = 80;                              //declares a constant integer variable  HTTP_PORT ,the port number  used for the HTTP communication.
const String HTTP_METHOD = "GET";                           //declares a constant String variable HTTP_METHOD with the value "GET",the HTTP used for communication with server.
const char   HOST_NAME[] = "";                              //declares a constant character array HOST_NAME initializes with "", IP address or domain name of server that client connects.
const String PATH_NAME   = "file.php";                      //declares a constant String variable PATH_NAME with the value "", path or endpoint on server the client send data


void setup() {          //This function is defined with the return type void, indicating that it doesn't return a value
  Serial.begin(9600);   //communication between the Arduino board and a computer through the USB connection
  lcd.init();           // initialize the lcd 
  lcd.backlight();      // turn light of the LCD on
  lcd.begin(16, 2);     // Initialize LCD in pins 16 columns 2 rows
  sensors.begin();      // Initialize the Dallas temperature sensor
  init_ethernet();      // Initialize ethernet
}

void init_ethernet() {
  Ethernet.begin(mac, ip, myDns);                       //calls the begin() function of Ethernet lib initialize the Ethernet module ip configures the Ethernet module with provided network settings.
  Serial.print("Connected Ethernet : ");                //Serial object to print to serial monitor if connection ok
  Serial.println(Ethernet.localIP());                   //prints the local IP address of Ethernet to serial monitor

  
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
    Serial.println("Ethernet shield was not found.");   // Check for Ethernet hardware.
  if (Ethernet.linkStatus() == LinkOFF)
    Serial.println("Ethernet cable is not connected."); // Check for Ethernet cable connection.

}

void send_data(float pH, float temp) {                                          //sends data to server using HTTP protocol and prints the server's response.
  if(client.connect(HOST_NAME, HTTP_PORT)) {                                    //connect to web server on port 80:
    Serial.println("Connected to server");                                      //if connected show on serial monitor
    String queryString = "?pH=" + String(pH) + "&temp=" + String(temp);         //String variable queryString holds query parameters be sent in HTTP request
    client.println(HTTP_METHOD + " " + PATH_NAME + queryString + " HTTP/1.1");  //make a HTTP request to server 
    client.println("Host: " + String(HOST_NAME));                               //host header apecify ip address
    client.println("Connection: close");                                        //sends the "Connection" header in HTTP request, connection be closed after server's response is received.
    client.println();                                                           //end HTTP header with empty line

    while(client.connected()) {                                                 //a while loop that runs as long as the client is connected to the server.
      if(client.available()) {                                                  //checks if there is incoming data available from the server
        char c = client.read();                                                 //read an incoming byte from the server and print it to serial monitor to see servers response
        Serial.print(c);
      }
    }

    client.stop();                            //stops the client's connection to the server
    Serial.println();                         //prints a blank line to serial monitor
    Serial.println("disconnected");           //prints disconnected to serial monitor
  } else {                                    // if not connected:
    Serial.println("connection failed");
  }
}

void loop() {                                //function is called repeatedly
  float pHVal, voltVal;                      //floating-point variables, pHVal and voltVal, are declared
  mean_average_pH(pHVal, voltVal);           //calculates the mean average pH value and updates the pHVal and voltVal variables with the result
  float temperature = readTemperature();     //calls function readTemperature returns a float value temperature stored in variable temperature
  displayData(pHVal, temperature);           //displays the pH value and temperature on LCD
  serialData(pHVal, voltVal, temperature);   //sends the pH value, voltage value, and temperature data over the serial communication
  send_data(pHVal, temperature);             //sends data
  delay(2000);                               //delay 2 sec each loop
}

void mean_average_pH(float &pHVal, float &voltVal) {             //two float variables pHVal and voltVal passed by reference (&) to allow their values to be modified inside the function
  static unsigned long samplingTime = millis();                  //static unsigned long variables, samplingTime and printTime
  static unsigned long printTime = millis();                     //millis returns the number of milliseconds that have passed since the Arduino board started running
  float pHValue, voltage;                                        //declares float variables, pHValue and voltage
  do {                                                           //continuously collects pH values, calculates the mean average, and updates the pHVal and voltVal variables
    if(millis() - samplingTime > samplingInterval)               //collect value every 20ms if enough time passed since last sampling executes the code block inside the curly braces.
    {             
      pHArray[pHArrayIndex++] = analogRead(pH_pin);              //reads the analog value from the pin and stores in the next position of the pHArray
      if(pHArrayIndex == ArrayLenth) pHArrayIndex = 0;           //reinitialize the array when is full
      voltage = avergearray(pHArray, ArrayLenth) * 5.0 / 1024;   //turn analog signal to voltage
      pHValue = 3.5 * voltage + X;                               //turn voltage to pH value
      samplingTime = millis();                                   //counter of time
    }
    if(millis() - printTime > printInterval)                     //checks if enough time passed since last print, print the value every 800ms
    {                   
      pHVal = pHValue;                                           //values of pHVal and voltVal updated with most recent every time executed the mean_average_pH function.
      voltVal = voltage;
      break;
    }
  } while(true);
}

double avergearray(int* arr, int number) {                        //defines function avergearray calculates average of array of integers type of double it returns a floating-point value two parameters: a pointer to an integer array arr, an integer  length of the array.
  int i;                                                          //defines variables
  int max,min;
  double avg;
  long amount=0;
  if (number <= 0) {                                              //error message when nothing in the array
    Serial.println("Error number for the array to averaging!/n");
    return 0;
  }
  if (number < 5) {                                               //checks if the number parameter is less than 5
    for(i = 0; i < number; i++) { amount +=arr [i]; }             //calculates the average of the array directly by summing
    avg = amount / number;                                        //dividing the sum by the length of the array
    return avg;
  } else {
    if (arr[0] < arr[1]) { min = arr[0];max=arr[1]; }             //determines minimum and maximum values among the first two elements of the array
    else { min=arr[1];max=arr[0]; }                              
    for(i = 2; i < number; i++){
      if(arr[i] < min){                                           //If element arr[i] smaller than current minimum (min), updates min and adds the previous minimum value to amount.
        amount += min;                   
        min = arr[i];                                             //collects min
      } else {
        if(arr[i] > max){                                         //If element arr[i] greater than current maximum (max), updates max and adds the previous maximum value to amount.
          amount += max;                  
          max = arr[i];                                           //collects max
        } else {
          amount += arr[i];                                       //sum up the index without max and min
        }
      }
    }
    avg = (double) amount / (number - 2);                         //Deduct the maximum and minimum values ​​and average
  }
  return avg;
}

void displayData(float pHValue, float temperature) {                //defines a function named displayData that displays pH value and temperature on an LCD screen
  lcd.setCursor(0, 0);                                              //sets the cursor position on the LCD screen to the first row and the first column
  lcd.print("pH   : ");                                             //prints the string "pH : " on the LCD screen   
  lcd.print(pHValue, 2);                                            //prints the value of pHValue with two decimal

  lcd.setCursor(0, 1);                                              //sets the cursor position on the LCD screen to the second row and the first column
  lcd.print("Temp : ");                                             //prints the string "Temp : " on the LCD screen
  lcd.print(temperature, 2);                                        //prints the value of temperarure with two decimal places
  lcd.print(" C");                                                  //prints the string " C"
}

void serialData(float pHValue, float voltage, float temperature) {  //outputs pH value, voltage, and temperature data to the serial monitor
  Serial.println("*****************");                              //outputs a line of asterisks to the serial monitor.just to be easier to read
  Serial.print("pH      : ");                                       //outputs ph from phValue  two decimal
  Serial.println(pHValue, 2);
  Serial.print("Voltage : ");                                       //outputs voltage from voltage  two decimal
  Serial.println(voltage, 2);
  Serial.print("Temp    : ");                                       //outputs temp from temperature  two decimal
  Serial.print(temperature, 2);
  Serial.println(" C");
  Serial.println("*****************\n");                            //outputs a line of asterisks to the serial monitor.just to be easier to read
}

float readpH() {                                                    // just for debugging, we dont need it
  int pH_raw = analogRead(pH_pin);
  float pH = 3.5 * pH_raw * 5.0 / 1024.0 + X;
  return pH;
}

float readTemperature() {                                           //sends a request to the DallasTemperature sensor to measure the temperature
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);                   //retrieves temperature in Celsius from sensor, getTempCByIndex(0) used to get temperature of first sensor connected to the DallasTemperature library
  return temperature;
}
