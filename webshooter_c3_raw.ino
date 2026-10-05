/* -------------------------------------------------
  Web Shooter - ESP32-C3 Super Mini, works with MPU6050 AND MPU6500 clones
  (no MPU library needed; reads the gyro directly over I2C)

  Works only till ESP32 core (boards) version 3.2.0.
  Board: ESP32C3 Dev Module, USB CDC On Boot = Enabled
  Library: the same BleMouse library you used before.

  Wiring:
    MPU  VCC->3V3  GND->GND  SDA->GPIO6  SCL->GPIO7
    Touch sensor (TTP223 module)  VCC->3V3  GND->GND  OUT->GPIO1
    Clutch button between GPIO2 and GND (optional)
          hold = freeze cursor, hold 2 s = recalibrate
-------------------------------------------------*/

#include <Wire.h>
#include <BleMouse.h>

#define SDA_PIN      6
#define SCL_PIN      7
#define SHOOT_PIN    1       // touch sensor OUT pin
#define TOUCH_ACTIVE_HIGH 1  // TTP223 default: OUT goes HIGH when touched (set 0 if yours is inverted)
#define CLUTCH_PIN   2
#define MPU_ADDR     0x68

#define SPEED        14.0    // cursor speed (gyro in rad/s), raise = faster
#define DEADZONE     0.04    // rad/s ignored
#define SMOOTH       0.5     // 0..1, higher = smoother but laggier
#define HIT_COOLDOWN 200     // ms between shots
#define TICK_MS      20      // 50 Hz

BleMouse bleMouse("WebShooter", "DIY", 100);

bool mpuAwake = false;
bool everConnected = false;
bool shootWas = false;
unsigned long disconnectedAt = 0, lastTick = 0, lastHit = 0, clutchDown = 0;
float biasX = 0, biasZ = 0;
float fx = 0, fy = 0, remX = 0, remY = 0;

void mpuSleep(bool sleep) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(sleep ? 0x40 : 0x01);   // 0x40 = sleep, 0x01 = awake (PLL clock)
  Wire.endTransmission();
}

// gyro in rad/s
bool readGyro(float &gx, float &gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MPU_ADDR, 6, 1) != 6) return false;
  int16_t rx = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read();          // skip Y
  int16_t rz = (Wire.read() << 8) | Wire.read();
  gx = rx / 131.0 * 0.0174533;       // deg/s -> rad/s
  gz = rz / 131.0 * 0.0174533;
  return true;
}

void calibrate() {
  Serial.println("Calibrating... keep still");
  float sx = 0, sz = 0, x, z;
  int n = 0;
  for (int i = 0; i < 400; i++) {
    if (readGyro(x, z)) { sx += x; sz += z; n++; }
    delay(5);
  }
  if (n) { biasX = sx / n; biasZ = sz / n; }
  fx = fy = remX = remY = 0;
  Serial.println("Calibration done");
}

void sendMove(int dx, int dy) {
  while (dx != 0 || dy != 0) {
    int sx = constrain(dx, -127, 127);
    int sy = constrain(dy, -127, 127);
    bleMouse.move(sx, sy);
    dx -= sx;
    dy -= sy;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(CLUTCH_PIN, INPUT_PULLUP);
  pinMode(SHOOT_PIN, TOUCH_ACTIVE_HIGH ? INPUT_PULLDOWN : INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  Wire.setTimeOut(20);

  bleMouse.begin();
  delay(1000);

  // check that something answers at the MPU address
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println("No MPU found at 0x68, check wiring");
    while (1) delay(10);
  }
  Serial.println("MPU found!");

  // keep MPU asleep until Bluetooth is connected (it can disturb pairing)
  mpuSleep(true);
}

void loop() {
  if (!bleMouse.isConnected()) {
    if (mpuAwake) {                       // link dropped: sleep MPU again
      mpuSleep(true);
      mpuAwake = false;
      disconnectedAt = millis();
      Serial.println("Disconnected");
    }
    if (everConnected && disconnectedAt && millis() - disconnectedAt > 15000) {
      Serial.println("Rebooting");
      ESP.restart();
    }
    delay(50);
    return;
  }

  if (!mpuAwake) {                        // just connected
    everConnected = true;
    delay(3000);
    Serial.println("MPU awakened!");
    mpuSleep(false);
    delay(1500);                          // let the gyro settle before calibrating
    calibrate();                          // keep your hand still for a moment
    mpuAwake = true;
  }

  unsigned long now = millis();
  if (now - lastTick < TICK_MS) return;
  float dt = (now - lastTick) / 1000.0;
  lastTick = now;

  // clutch: hold = freeze cursor, hold 2 s = recalibrate
  bool clutch = digitalRead(CLUTCH_PIN) == LOW;
  if (clutch) {
    if (!clutchDown) clutchDown = now;
    if (now - clutchDown > 2000) { calibrate(); clutchDown = now + 100000; }
  } else {
    clutchDown = 0;
  }

  float gx, gz;
  if (!readGyro(gx, gz)) return;

  // auto drift correction: when the hand is nearly still, slowly re-learn the bias
  if (fabs(gz - biasZ) < 0.06 && fabs(gx - biasX) < 0.06) {
    biasZ += (gz - biasZ) * 0.01;
    biasX += (gx - biasX) * 0.01;
  }

  // swap axes or flip the signs if the direction feels wrong
  float vx = -(gz - biasZ);
  float vy = -(gx - biasX);
  if (fabs(vx) < DEADZONE) vx = 0;
  if (fabs(vy) < DEADZONE) vy = 0;
  fx = SMOOTH * fx + (1 - SMOOTH) * vx;
  fy = SMOOTH * fy + (1 - SMOOTH) * vy;

  if (!clutch) {
    remX += fx * SPEED * dt * 50;
    remY += fy * SPEED * dt * 50;
    int dx = (int)remX, dy = (int)remY;
    remX -= dx;
    remY -= dy;
    if (dx || dy) sendMove(dx, dy);
  }

  // touch sensor: one click each time you touch it
  bool shootDown = digitalRead(SHOOT_PIN) == (TOUCH_ACTIVE_HIGH ? HIGH : LOW);
  if (shootDown && !shootWas && now - lastHit > HIT_COOLDOWN) {
    lastHit = now;
    Serial.println("Shoot");
    bleMouse.click(MOUSE_LEFT);
  }
  shootWas = shootDown;
}
