#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Custom OLED Pin Configuration from AGENTS.md
#define OLED_SDA 14 // D5 (GPIO 14)
#define OLED_SCL 12 // D6 (GPIO 12)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Animation State Control
unsigned long lastStateChange = 0;
int currentAnim = 0;
const int ANIM_DURATION = 6000; // 6 seconds per animation

// ==========================================
// 1. Starfield Animation (Space Travel)
// ==========================================
#define NUM_STARS 40
struct Star {
  float x, y, z;
};
Star stars[NUM_STARS];

void initStarfield() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(-64, 64);
    stars[i].y = random(-32, 32);
    stars[i].z = random(1, 128);
  }
}

void drawStarfield() {
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("1. Starfield"));

  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].z -= 2.0; // Move stars closer
    if (stars[i].z <= 0) {
      stars[i].x = random(-64, 64);
      stars[i].y = random(-32, 32);
      stars[i].z = 128;
    }

    // 3D perspective projection
    int k = 60; // Perspective coefficient
    int sx = (int)(stars[i].x * k / stars[i].z) + SCREEN_WIDTH / 2;
    int sy = (int)(stars[i].y * k / stars[i].z) + SCREEN_HEIGHT / 2 + 5; // offset for header

    if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 10 && sy < SCREEN_HEIGHT) {
      // Draw larger dot if star is closer
      if (stars[i].z < 32) {
        display.fillRect(sx, sy, 2, 2, SSD1306_WHITE);
      } else {
        display.drawPixel(sx, sy, SSD1306_WHITE);
      }
    }
  }
  display.display();
}

// ==========================================
// 2. 3D Rotating Cube
// ==========================================
struct Point3D {
  float x, y, z;
};

// 8 vertices of a cube
Point3D cubeVertices[8] = {
  {-14, -14, -14},
  {14, -14, -14},
  {14, 14, -14},
  {-14, 14, -14},
  {-14, -14, 14},
  {14, -14, 14},
  {14, 14, 14},
  {-14, 14, 14}
};

// 12 edges connecting the vertices
const int cubeEdges[12][2] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Back face
  {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Front face
  {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting edges
};

float angleX = 0;
float angleY = 0;
float angleZ = 0;

void drawCube() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("2. 3D Rotating Cube"));

  angleX += 0.03;
  angleY += 0.04;
  angleZ += 0.02;

  Point3D projected[8];

  for (int i = 0; i < 8; i++) {
    // Rotation X
    float y1 = cubeVertices[i].y * cos(angleX) - cubeVertices[i].z * sin(angleX);
    float z1 = cubeVertices[i].y * sin(angleX) + cubeVertices[i].z * cos(angleX);

    // Rotation Y
    float x2 = cubeVertices[i].x * cos(angleY) - z1 * sin(angleY);
    float z2 = cubeVertices[i].x * sin(angleY) + z1 * cos(angleY);

    // Rotation Z
    float x3 = x2 * cos(angleZ) - y1 * sin(angleZ);
    float y3 = x2 * sin(angleZ) + y1 * cos(angleZ);

    // Perspective projection
    float distance = 60;
    float k = 40.0 / (z2 + distance);
    projected[i].x = x3 * k * 60 + SCREEN_WIDTH / 2;
    projected[i].y = y3 * k * 60 + SCREEN_HEIGHT / 2 + 5;
  }

  // Draw edges
  for (int i = 0; i < 12; i++) {
    display.drawLine(projected[cubeEdges[i][0]].x, projected[cubeEdges[i][0]].y,
                     projected[cubeEdges[i][1]].x, projected[cubeEdges[i][1]].y,
                     SSD1306_WHITE);
  }
  
  // Draw vertices as bold dots
  for (int i = 0; i < 8; i++) {
    display.fillCircle(projected[i].x, projected[i].y, 2, SSD1306_WHITE);
  }

  display.display();
}

// ==========================================
// 3. Plexus (Connected Floating Particles)
// ==========================================
#define NUM_PARTICLES 14
struct Particle {
  float x, y;
  float dx, dy;
};
Particle particles[NUM_PARTICLES];

void initPlexus() {
  for (int i = 0; i < NUM_PARTICLES; i++) {
    particles[i].x = random(5, SCREEN_WIDTH - 5);
    particles[i].y = random(15, SCREEN_HEIGHT - 5);
    particles[i].dx = random(-15, 15) / 10.0;
    particles[i].dy = random(-15, 15) / 10.0;
    
    // Prevent zero speed
    if (particles[i].dx == 0) particles[i].dx = 0.7;
    if (particles[i].dy == 0) particles[i].dy = -0.7;
  }
}

void drawPlexus() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("3. Plexus Network"));

  // Move particles and bounce off boundaries
  for (int i = 0; i < NUM_PARTICLES; i++) {
    particles[i].x += particles[i].dx;
    particles[i].y += particles[i].dy;

    if (particles[i].x < 2 || particles[i].x > SCREEN_WIDTH - 2) particles[i].dx *= -1;
    if (particles[i].y < 12 || particles[i].y > SCREEN_HEIGHT - 2) particles[i].dy *= -1;

    particles[i].x = constrain(particles[i].x, 2, SCREEN_WIDTH - 2);
    particles[i].y = constrain(particles[i].y, 12, SCREEN_HEIGHT - 2);
  }

  // Draw connecting lines between close particles
  for (int i = 0; i < NUM_PARTICLES; i++) {
    for (int j = i + 1; j < NUM_PARTICLES; j++) {
      float dist = sqrt(pow(particles[i].x - particles[j].x, 2) + pow(particles[i].y - particles[j].y, 2));
      if (dist < 26) {
        display.drawLine(particles[i].x, particles[i].y, particles[j].x, particles[j].y, SSD1306_WHITE);
      }
    }
  }

  // Draw particle dots
  for (int i = 0; i < NUM_PARTICLES; i++) {
    display.fillCircle(particles[i].x, particles[i].y, 2, SSD1306_WHITE);
  }

  display.display();
}

// ==========================================
// 4. Wave Interference
// ==========================================
float waveOffset = 0;

void drawSineWaves() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("4. Wave Interference"));

  waveOffset += 0.15;

  int prevY1 = -1;
  int prevY2 = -1;

  for (int x = 0; x < SCREEN_WIDTH; x += 2) {
    // First sine wave (moving right)
    float angle1 = (x * 0.1) + waveOffset;
    int y1 = 36 + sin(angle1) * 14;

    // Second cosine wave (moving left)
    float angle2 = (x * 0.15) - waveOffset * 0.8;
    int y2 = 36 + cos(angle2) * 11;

    // Draw lines connecting the two waves vertically at intervals (mesh effect)
    if (x % 8 == 0) {
      display.drawLine(x, y1, x, y2, SSD1306_WHITE);
      display.fillCircle(x, y1, 1, SSD1306_WHITE);
      display.fillCircle(x, y2, 1, SSD1306_WHITE);
    }

    // Connect consecutive points to form continuous curves
    if (prevY1 != -1) {
      display.drawLine(x - 2, prevY1, x, y1, SSD1306_WHITE);
      display.drawLine(x - 2, prevY2, x, y2, SSD1306_WHITE);
    }
    prevY1 = y1;
    prevY2 = y2;
  }

  display.display();
}

// ==========================================
// 5. Radar Scan & Tracking
// ==========================================
float radarAngle = 0;
#define NUM_TARGETS 4
struct Target {
  int x, y;
  float intensity; // for visual decay
};
Target targets[NUM_TARGETS];

void initRadar() {
  for (int i = 0; i < NUM_TARGETS; i++) {
    targets[i].x = random(25, SCREEN_WIDTH - 25);
    targets[i].y = random(20, SCREEN_HEIGHT - 10);
    targets[i].intensity = 0;
  }
}

void drawRadar() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("5. Radar Sweep"));

  int cx = SCREEN_WIDTH / 2;
  int cy = SCREEN_HEIGHT / 2 + 5;
  int r = 26;

  // Draw radar grid
  display.drawCircle(cx, cy, r, SSD1306_WHITE);
  display.drawCircle(cx, cy, r/2, SSD1306_WHITE);
  display.drawCircle(cx, cy, 2, SSD1306_WHITE);

  // Draw crosshairs
  display.drawFastHLine(cx - r - 4, cy, 2 * r + 8, SSD1306_WHITE);
  display.drawFastVLine(cx, cy - r - 4, 2 * r + 8, SSD1306_WHITE);

  // Radar sweep line math
  radarAngle += 0.07;
  if (radarAngle >= 2.0 * PI) radarAngle = 0;

  int lx = cx + r * cos(radarAngle);
  int ly = cy + r * sin(radarAngle);
  display.drawLine(cx, cy, lx, ly, SSD1306_WHITE);

  // Track and pulse targets when radar sweeps past
  for (int i = 0; i < NUM_TARGETS; i++) {
    float targetAngle = atan2(targets[i].y - cy, targets[i].x - cx);
    if (targetAngle < 0) targetAngle += 2.0 * PI;

    // Check angle difference
    float diff = abs(radarAngle - targetAngle);
    if (diff < 0.12) {
      targets[i].intensity = 5.0; // Trigger flare/ping
    }

    if (targets[i].intensity > 0) {
      int size = (int)targets[i].intensity;
      display.fillCircle(targets[i].x, targets[i].y, size, SSD1306_WHITE);
      display.drawCircle(targets[i].x, targets[i].y, size + 2, SSD1306_WHITE);
      targets[i].intensity -= 0.15; // Slow decay
    } else {
      display.drawPixel(targets[i].x, targets[i].y, SSD1306_WHITE);
    }
  }

  display.display();
}

// ==========================================
// Core Settings & Setup
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\nInitializing OLED animations..."));

  Wire.begin(OLED_SDA, OLED_SCL);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  
  // Initialize dynamic arrays
  initStarfield();
  initPlexus();
  initRadar();

  lastStateChange = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // State Transition
  if (currentMillis - lastStateChange >= ANIM_DURATION) {
    currentAnim = (currentAnim + 1) % 5;
    lastStateChange = currentMillis;
    
    // Optional refresh on transition
    if (currentAnim == 0) initStarfield();
    if (currentAnim == 2) initPlexus();
    if (currentAnim == 4) initRadar();
  }

  // Animation Router
  switch (currentAnim) {
    case 0: drawStarfield();   break;
    case 1: drawCube();        break;
    case 2: drawPlexus();      break;
    case 3: drawSineWaves();   break;
    case 4: drawRadar();       break;
  }

  delay(30); // ~33 FPS
}
