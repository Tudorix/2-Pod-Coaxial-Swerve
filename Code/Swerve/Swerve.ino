#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

#define PIN_IN1 5
#define PIN_IN2 4

#define PIN_IN3 14
#define PIN_IN4 12

#define PIN_LEFT 13
#define PIN_RIGHT 15

Servo left, right;

ESP8266WebServer server(80);

const char* ssid = "Swerve";
const char* password = "12345678";

// Last received joystick values
float joyX = 0.0;
float joyY = 0.0;
float joyR = 0.0;

//Range Parameters
float newMin = 0;
float newMax = 180;

float oldMax = 1.0;
float oldMin = -1.0;


const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Robot Controller</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #111;
      color: white;
      text-align: center;
      padding: 20px;
    }
    .box {
      background: #1e1e1e;
      border-radius: 16px;
      padding: 20px;
      max-width: 420px;
      margin: auto;
    }
    .value {
      font-size: 22px;
      margin: 10px 0;
    }
    .status {
      margin-top: 15px;
      font-size: 18px;
      color: #aaa;
    }
    button {
      font-size: 18px;
      padding: 12px 18px;
      border-radius: 12px;
      border: none;
      margin-top: 10px;
    }
  </style>
</head>
<body>
  <div class="box">
    <h1>Swerve Robot Control</h1>
    <p>Pair controller to phone first, then press Start.</p>

    <button onclick="startReading()">Start</button>

    <div class="value">X: <span id="xVal">0.00</span></div>
    <div class="value">Y: <span id="yVal">0.00</span></div>
    <div class="value">R: <span id="rVal">0.00</span></div>
    
    <div class="status" id="status">Waiting...</div>
  </div>

  <script>
    let intervalId = null;
    let lastSentX = 0;
    let lastSentY = 0;
    let lastSentR = 0;

    function clampDeadband(v, deadband = 0.15) {
      if (Math.abs(v) < deadband) return 0;
      return v;
    }
j
    function startReading() {
      if (intervalId !== null) return;

      document.getElementById("status").innerText = "Reading controller...";
      
      intervalId = setInterval(() => {
        const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
        const gp = gamepads[0];

        if (!gp) {
          document.getElementById("status").innerText = "No controller detected";
          return;
        }

        // Left stick: axes[0] = X, axes[1] = Y
        let x = clampDeadband(gp.axes[0] || 0);
        let y = clampDeadband(gp.axes[1] || 0);
        let r = clampDeadband(gp.axes[2] || 0);
        

        // Optional: invert Y so pushing stick forward gives positive value
        y = -y;

        document.getElementById("xVal").innerText = x.toFixed(2);
        document.getElementById("yVal").innerText = y.toFixed(2);
        document.getElementById("rVal").innerText = r.toFixed(2);
        document.getElementById("status").innerText = "Controller connected";

        // Only send if changed enough
        if (Math.abs(x - lastSentX) > 0.03 || Math.abs(y - lastSentY) > 0.03 || Math.abs(r - lastSentR) > 0.03) {
          lastSentX = x;
          lastSentY = y;
          lastSentR = r;

          fetch(`/drive?x=${x.toFixed(2)}&y=${y.toFixed(2)}&r=${r.toFixed(2)}`)
            .catch(err => {
              document.getElementById("status").innerText = "Send failed";
              console.log(err);
            });
        }
      }, 100);
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", webpage);
}

void handleDrive() {
  if (server.hasArg("x")) {
    joyX = server.arg("x").toFloat();
  }
  if (server.hasArg("y")) {
    joyY = server.arg("y").toFloat();
  }
  if (server.hasArg("r")) {
    joyR = server.arg("r").toFloat();
  }

  Serial.print("Received -> X: ");
  Serial.print(joyX, 2);
  Serial.print("  Y: ");
  Serial.println(joyY, 2);
  Serial.print("  R: ");
  Serial.println(joyR, 2);

  if(joyR == 0){
    if(joyY > 0){
      digitalWrite(PIN_IN1, LOW);
      digitalWrite(PIN_IN2, HIGH); 

      digitalWrite(PIN_IN3, HIGH);
      digitalWrite(PIN_IN4, LOW);
    }else if(joyY < 0){
      digitalWrite(PIN_IN1, HIGH);
      digitalWrite(PIN_IN2, LOW); 

      digitalWrite(PIN_IN3, LOW);
      digitalWrite(PIN_IN4, HIGH);
    }else{
      digitalWrite(PIN_IN1, LOW); 
      digitalWrite(PIN_IN2, LOW); 
      digitalWrite(PIN_IN3, LOW);
      digitalWrite(PIN_IN4, LOW);
    }
    if(joyY < 0){
        newMax = 0;
        newMin = 180;
      }else{
        newMax = 180;
        newMin = 0;
      }
      float servo_pos = newMin + (joyX - oldMin) * (newMax - newMin) / (oldMax - oldMin);
      left.write(servo_pos);
      right.write(servo_pos);
      Serial.print(servo_pos);
  }else if(joyR > 0){
      left.write(90);
      right.write(90);
      digitalWrite(PIN_IN1, LOW);
      digitalWrite(PIN_IN2, HIGH); 

      digitalWrite(PIN_IN3, LOW);
      digitalWrite(PIN_IN4, HIGH);
  }else{
      left.write(90);
      right.write(90);
      digitalWrite(PIN_IN1, HIGH);
      digitalWrite(PIN_IN2, LOW); 

      digitalWrite(PIN_IN3, HIGH);
      digitalWrite(PIN_IN4, LOW);
  }

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("Access Point started");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/drive", handleDrive);

  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

  left.attach(PIN_LEFT);
  right.attach(PIN_RIGHT);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}