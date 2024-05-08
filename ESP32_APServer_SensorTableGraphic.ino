#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <QTRSensors.h>

// Credenciales del AccessPoint
const char *ssid = "GoCar";
const char *password = "123456789";

// Inicializa el servidor web 
WiFiServer server(80);

// Variables de QTR-8RC
QTRSensors qtr;
const uint8_t SensorCount = 6;
uint16_t sensorValues[SensorCount];

// Variables para almacenar las últimas 20 lecturas de los sensores
uint16_t sensorReadings[20][SensorCount];
int currentIndex = 0;

void setup() {
  ///////////////////////////////////////////Configuración del AP
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Soft AP creation failed.");
    while(1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");

  //////////////////////////////////////////Configuración de QTR-8RC
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){22, 21, 19, 18, 5, 4}, SensorCount);
  delay(500);

  for (uint16_t i = 0; i < 400; i++){
    qtr.calibrate();
  }
}

void loop() {
  // Inicializa el loop de QTR-8RC
  uint16_t position = qtr.readLineBlack(sensorValues);

  // Guarda los últimos valores de los sensores
  for (int i = 0; i < SensorCount; i++) {
    sensorReadings[currentIndex][i] = sensorValues[i];
  }
  currentIndex = (currentIndex + 1) % 20; // Avanza al siguiente índice, ciclando por las 20 posiciones

  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.println("<!DOCTYPE HTML>");
            client.println("<html><head><meta charset=\"UTF-8\"><meta http-equiv=\"refresh\" content=\"0.25\"></head><body>");
            client.println("<h1>Valores de los Sensores</h1>");
            client.println("<table border=\"1\"><tr><th>Sensor 1</th><th>Sensor 2</th><th>Sensor 3</th><th>Sensor 4</th><th>Sensor 5</th><th>Sensor 6</th></tr>");
            
            // Muestra las últimas 20 lecturas de los sensores en la tabla
            for (int i = 0; i < 20; i++) {
              client.print("<tr>");
              for (int j = 0; j < SensorCount; j++) {
                client.print("<td>");
                client.print(sensorReadings[(currentIndex + i) % 20][j]); // Muestra los valores desde currentIndex hacia atrás
                client.print("</td>");
              }
              client.println("</tr>");
            }
            client.println("</table>");

            // Grafica de barras para los valores de los sensores
            client.println("<h1>Gráfico de Barras</h1>");
            client.println("<svg width=\"500\" height=\"200\">");

            // Eje X
            client.println("<line x1=\"50\" y1=\"150\" x2=\"450\" y2=\"150\" style=\"stroke:black;stroke-width:2\" />");
            for (int i = 0; i < SensorCount; i++) {
              int xPos = 50 + (i + 1) * 75;
              client.print("<text x=\"");
              client.print(xPos);
              client.print("\" y=\"170\" font-size=\"14\" fill=\"black\">Sensor ");
              client.print(i + 1);
              client.println("</text>");
            }

            // Eje Y
            client.println("<line x1=\"50\" y1=\"50\" x2=\"50\" y2=\"150\" style=\"stroke:black;stroke-width:2\" />");
            for (int i = 0; i <= 10; i++) {
              int yPos = 150 - i * 10;
              client.print("<text x=\"30\" y=\"");
              client.print(yPos + 5);
              client.print("\" font-size=\"12\" fill=\"black\">");
              client.print(i * 100);
              client.println("</text>");
            }

            // Barras
            int barWidth = 50;
            int barSpacing = 10;
            for (int i = 0; i < SensorCount; i++) {
              int barHeight = sensorValues[i] / 10; // Ajusta la altura de la barra
              int xPos = 50 + (i + 1) * 75 - barWidth / 2;
              int yPos = 150 - barHeight;
              client.print("<rect x=\"");
              client.print(xPos);
              client.print("\" y=\"");
              client.print(yPos);
              client.print("\" width=\"");
              client.print(barWidth);
              client.print("\" height=\"");
              client.print(barHeight);
              client.println("\" fill=\"blue\" />");
            }
            client.println("</svg>");

            client.println("</body></html>");

            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
