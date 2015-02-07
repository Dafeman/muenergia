#include "Test1Module.h"
//
#include "driverlib/sysctl.h"
//
#ifndef TARGET_IS_BLIZZARD_RB1
#include "Ethernet.h"
#endif

//MAKE_MODULE(Test1Module)

#ifndef TARGET_IS_BLIZZARD_RB1
EthernetServer server(80);
EthernetClient client;
int statusConfig = 0;

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// Global buffer that holds the command line.
//
//*****************************************************************************
static char g_cCmdBuf[CMD_BUF_SIZE];

void printEthernetData();
void printConfig();
void printIndex();
void printHelp();
#endif

void Test1Module::init()
{
#ifndef TARGET_IS_BLIZZARD_RB1
  Ethernet.begin(0);
  server.begin();
  tivaWare.UART.printf("Connecting to Ethernet....");
  printEthernetData();
#endif
}

void Test1Module::execute()
{
  //String str;
  //str += "test";
  //str += "\n";
  //debug.UART.write(str);

  // Delay for a bit for 10 ms
  //SysCtlDelay(SysCtlClockGet() / 10 / 3);

#ifndef TARGET_IS_BLIZZARD_RB1

  client = server.available();

  if (client)
  {                             // if you get a client,
    UARTprintf("new client on port ");                        // print a message out the serial port
    UARTprintf("%d \n", client.port());
    String currentLine = "";                  // make a String to hold incoming data from the client
    boolean newConnection = true;                             // flag for new connections
    unsigned long connectionActiveTimer;                      // will hold the connection start time

    while (client.connected())
    {       // loop while the client's connected
      if (newConnection)
      {                 // it's a new connection, so
        connectionActiveTimer = millis();                 // log when the connection started
        newConnection = false;                 // not a new connection anymore
      }
      if (!newConnection && connectionActiveTimer + 1000 < millis())
      {
        // if this while loop is still active 1000ms after a web client connected, something is wrong
        break;// leave the while loop, something bad happened
      }

      if (client.available())
      {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // This lockup is because the recv function is blocking.
        UARTprintf("%c", c);
        if (c == '\n')
        {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            break;
          }
          else
          {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r')
        {    // if you got anything else but a carriage return character,
          currentLine += c;    // add it to the end of the currentLine
        }
        if (currentLine.endsWith("GET / "))
        {
          statusConfig = 0;
          printIndex();
        }
        if (currentLine.endsWith("GET /config.html "))
        {
          statusConfig = 1;
          printConfig();
        }
        if (currentLine.endsWith("GET /index.html "))
        {
          statusConfig = 0;
          printIndex();
        }
        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /LED1_H"))
        {
          //digitalWrite(D1_LED, HIGH);
          printConfig();
        }
        if (currentLine.endsWith("GET /LED1_L"))
        {
          //digitalWrite(D1_LED, LOW);
          printConfig();
        }
        if (currentLine.endsWith("GET /LED2_H"))
        {
          //digitalWrite(D2_LED, HIGH);
          printConfig();
        }
        if (currentLine.endsWith("GET /LED2_L"))
        {
          //digitalWrite(D2_LED, LOW);
          printConfig();
        }
      }
    }
    // close the connection:
    client.stop();
    //Serial.println("client disonnected");
  }

#endif
}

void Test1Module::update(Test1Representation& theTest1Representation)
{
}

#ifndef TARGET_IS_BLIZZARD_RB1
void printEthernetData()
{
  // print your IP address:
  UARTprintf("IP Address Information:\n");
  IPAddress ip = Ethernet.localIP();
  UARTprintf("IP Address:\t");
  UARTprintf("%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

  // print your MAC address:

  IPAddress subnet = Ethernet.subnetMask();
  UARTprintf("NetMask:\t");
  UARTprintf("%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

  // print your gateway address:
  IPAddress gateway = Ethernet.gatewayIP();
  UARTprintf("Gateway:\t");
  UARTprintf("%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

  // print your gateway address:
  IPAddress dns = Ethernet.dnsServerIP();
  UARTprintf("DNS:\t\t");
  UARTprintf("%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
}

void printIndex()
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  //  UARTprintf("Index");
  client.println("HTTP/1.1 200 OK");

  client.println("Content-type:text/html");
  client.println();
  client.println(
      "<html><head><title>Energia Ethernet Web Server</title></head><body align=center>");
  client.println("<h1 align=center><font color=\"red\">Welcome To Ethernet Web Server</font></h1>");
  client.println(
      "</br><font size=\"4px\"><table border=\"0\" align=center width=1200px height=590px>");
  client.println("<tr><td align=center width=375></td><td width=450px align=left valign=\"top\">");

  client.println("<p>The Ethernet Web Server provides the");
  client.println("capability to remotely read and write GPIOs ");
  client.println("on/off.</p></br></br>");
  client.println("<p><a href=\"/config.html\">Click here</a> ");
  client.println("to check status and configure the board</p>");
  client.println("<td align=cneter width=375></td></tr></table></font></body></html>");

  client.println();

}

void printConfig()
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.println(
      "<html><head><title>Energia Ethernet Web Server</title></head><body align=center>");
  client.println(
      "<h1 align=center><font color=\"red\">Welcome to the Ethernet Web Server</font></h1>");

  // the content of the HTTP response follows the header:
  // Added: nicer buttons
  client.print("LED 1<button onclick=\"location.href='/LED1_H'\">HIGH</button>");
  client.println(" <button onclick=\"location.href='/LED1_L'\">LOW</button><br>");
  client.print("LED 2 <button onclick=\"location.href='/LED2_H'\">HIGH</button>");
  client.println(" <button onclick=\"location.href='/LED2_L'\">LOW</button><br><br>");

  client.println("PUSH1 ");
  if (digitalRead (PUSH1))
    client.print("is HIGH<br>");
  else
    client.print("is LOW<br>");
  client.println("PUSH2 ");
  if (digitalRead (PUSH2))
    client.print("is HIGH<br>");
  else
    client.print("is LOW<br>");

  client.println("<a href=\"/config.html\" >refresh</a> <br>");
  client.println("<a href=\"/index.html\" >home</a> <br>");
  client.println("</body></html>");
  // The HTTP response ends with another blank line:
  client.println();
  // break out of the while loop:
}

void serialEvent()
{
  //
  // Peek to see if a full command is ready for processing
  //
  if (UARTPeek('#') == -1)
  {
    //
    // If not, return so other functions get a chance to run.
    //
    return;
  }

  while (UARTPeek('#') != -1)
  {
    //
    // Get a user command back
    //
    UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));

  }

  char inChar = g_cCmdBuf[0];
  switch (inChar)
  {
  case 'h':
    printHelp();
    break;
  case 'i':
    printEthernetData();
    break;
  default:
    UARTprintf("Invalid menu option\n");

  }
}

void printHelp()
{
  UARTprintf("+++++++++++++++++++++++++++++++++++++\n");
  UARTprintf("Help menu:\n");
  UARTprintf("\th: This help menu\n");
  UARTprintf("\ti: IP address information\n");
  UARTprintf("+++++++++++++++++++++++++++++++++++++\n");
}

void printHex(int num, int precision)
{
  char tmp[16];
  char format[128];

  sprintf(format, "%%.%dX", precision);

  sprintf(tmp, format, num);
  UARTprintf(tmp);
}

#endif
