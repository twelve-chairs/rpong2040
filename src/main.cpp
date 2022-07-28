#include <SPI.h>
#include <Adafruit_ST7789.h>      // Hardware-specific library for ST7789
#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <Adafruit_NeoPixel.h>
#include <iostream>

#define SD_CS     8   // SD card select pin
#define TFT_DC    9
#define TFT_CS    10
#define TFT_RST   -1  // Or set to -1 and connect to Arduino RESET pin

#define ST77XX_PINK       0xFB9B
#define ST77XX_DARKGREEN  0x0BA8

#define USE_SD_CARD

#define PIN         16
#define NUMPIXELS   1

#define POTENTIOMETER_PIN1 A1
#define POTENTIOMETER_PIN2 A3


const uint16_t width = 320;
const uint16_t height = 170;

const uint16_t STARTX1 = (width / 4) - 12;
const uint16_t STARTX2 = (width - STARTX1) - 6;

const uint16_t backgroundColor = ST77XX_DARKGREEN;

struct Player1 {
    int16_t x = 10;
    int16_t y = 60;
    int16_t width = 5;
    int16_t height = 50;
    int16_t color = ST77XX_PINK;
    int16_t score = 0;
};

struct Player2 {
    int16_t x = 305;
    int16_t y = 80;
    int16_t width = 5;
    int16_t height = 50;
    int16_t color = ST77XX_CYAN;
    int16_t score = 0;
};

struct Ball {
    int16_t x = 10;
    int16_t y = 60;
    int16_t radius = 4;
    int16_t ballDirection = 1;
    int16_t ballUp = 1;
};


void updateScore1(Adafruit_ST7789 &tft, const Player1 &player1){
    tft.setTextSize(2);

    tft.fillRect(STARTX1, 2, 30, 18, backgroundColor);

    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(STARTX1 + 1, 5);
    tft.print(player1.score);

    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(STARTX1, 4);
    tft.print(player1.score);
}

void updateScore2(Adafruit_ST7789 &tft, const Player2 &player2){
    tft.setTextSize(2);

    tft.fillRect(STARTX2, 2, 30, 18, backgroundColor);

    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(STARTX2 + 1, 5);
    tft.print(player2.score);

    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(STARTX2, 4);
    tft.print(player2.score);
}

void changeDirectionX(Ball &ball, bool &ledOn){
    ball.ballDirection *= -1;
    ledOn = true;
}

void changeDirectionY(Ball &ball, bool &ledOn){
    ball.ballUp *= -1;
    ledOn = true;
}

void drawField(Adafruit_ST7789 &tft){
    tft.drawFastHLine(0, 0, width, ST77XX_WHITE);
    tft.drawFastHLine(0, height-1, width, ST77XX_WHITE);

    tft.drawFastHLine(0, 20, width, ST77XX_WHITE);
    tft.drawFastHLine(0, height-20, width, ST77XX_WHITE);

    tft.drawFastVLine(0, 0, height, ST77XX_WHITE);
    tft.drawFastVLine(width - 1, 0, height, ST77XX_WHITE);

    tft.drawFastVLine(width/2, 0, height, ST77XX_WHITE);
}

bool checkCollision(Adafruit_ST7789 &tft, Ball &ball, Player1 &player1, Player2 &player2, int &lastPlayer, bool &ledOn){
    bool playerCollision = false;

    // Player 1 paddle collision
    if (((ball.x - ball.radius == player1.x + player1.width + 2) ||
         (ball.x + ball.radius == player1.x - player1.width)) &&
        (ball.y >= player1.y && ball.y <= (player1.y + player1.height))){
        playerCollision = true;
        lastPlayer = 1;
        changeDirectionX(ball, ledOn);
    }

    // Player 2 paddle collision
    if (((ball.x + ball.radius == player2.x - 2) ||
         (ball.x - ball.radius == player2.x + player2.width)) &&
        (ball.y >= player2.y && ball.y <= (player2.y + player2.height))){
        playerCollision = true;
        lastPlayer = 2;
        changeDirectionX(ball, ledOn);
    }

    // Left wall collision
    if (ball.x == 0){
        if (lastPlayer == 2){
            if (player2.score < 99){
                player2.score++;
            }
            else {
                player2.score = 0;
            }
        }
        updateScore2(tft, player2);
        changeDirectionX(ball, ledOn);
    }

    // Right wall collision
    if (ball.x >= width){
        if (lastPlayer == 1){
            if (player1.score < 99){
                player1.score++;
            }
            else {
                player1.score = 0;
            }
        }
        updateScore1(tft, player1);
        changeDirectionX(ball, ledOn);
    }

    // Top wall collision
    if (ball.y == 0){
        changeDirectionY(ball, ledOn);
    }

    // Bottom wall collision
    if (ball.y >= height){
        changeDirectionY(ball, ledOn);
    }

    return playerCollision;
}

void cycleLed(const bool &ledOn, Adafruit_NeoPixel &pixels){
    if (ledOn){
        pixels.setPixelColor(0, Adafruit_NeoPixel::Color(255, 255, 255));
    }
    else {
        pixels.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0));
    }
    pixels.show();
}

void setup(Adafruit_ST7789 &tft, unsigned long &localTime, const Player1 &player1, const Player2 &player2) {
    tft.init(height, width);
    tft.setRotation(3);
    tft.fillScreen(backgroundColor);

    SdFat SD;                         // SD card filesystem
    Adafruit_ImageReader reader(SD);  // Image-reader object, pass in SD filesys

    if(!SD.begin(SD_CS, SD_SCK_MHZ(10))) { // Breakouts require 10 MHz limit due to longer wires
        Serial.println(F("SD begin() failed"));
        exit(1);
    }

    localTime = millis() - localTime;

    // Splash screen
    reader.drawBMP("/sofia.bmp", tft, 14, 20);
    tft.setCursor(140, 30);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(4);
    tft.println("Sofia's");
    tft.setCursor(140, 70);
    tft.println("Super");
    tft.setCursor(140, 110);
    tft.print("Tennis!");
    delay(5000);

    // Game screen init
    tft.fillScreen(backgroundColor);
    updateScore1(tft, player1);
    updateScore2(tft, player2);
}

void loop(bool &ledOn, Adafruit_ST7789 &tft, Ball &ball, Player1 &player1, Player2 &player2, int &lastPlayer, Adafruit_NeoPixel &pixels) {
    ledOn = false;
    long player1Pin = map(analogRead(POTENTIOMETER_PIN1), 0, 1023, 2, 120);
    long player2Pin = map(analogRead(POTENTIOMETER_PIN2), 1023, 0, 2, 120);

    drawField(tft);

    // Ball
    tft.fillCircle(ball.x, ball.y, ball.radius + 1, backgroundColor);
    ball.x = ball.x + ball.ballDirection;
    ball.y = ball.y + ball.ballUp;
    tft.drawCircle(ball.x, ball.y, ball.radius + 1, ST77XX_BLACK);
    tft.fillCircle(ball.x, ball.y, ball.radius, ST77XX_YELLOW);

    // Players
    if (checkCollision(tft, ball, player1, player2, lastPlayer, ledOn)
        || (player1.y != player1Pin)
        || (player2.y != player2Pin) ){
        tft.fillRect(player1.x, player1.y, player1.width, player1.height, backgroundColor);
        player1.y = static_cast<int16_t>(player1Pin);
        tft.fillRect(player1.x, player1.y, player1.width, player1.height, player1.color);

        tft.fillRect(player2.x, player2.y, player2.width, player2.height, backgroundColor);
        player2.y = static_cast<int16_t>(player2Pin);
        tft.fillRect(player2.x, player2.y, player2.width, player2.height, player2.color);
    }

    // Score board
    if (ball.y >= 2 && ball.y <= 22){
        if (ball.x >= STARTX1 - 10 && ball.x <= STARTX1 + 30){
            updateScore1(tft, player1);
        }
        if (ball.x >= STARTX2 - 10 && ball.x <= STARTX2 + 30){
            updateScore2(tft, player2);
        }
    }

    cycleLed(ledOn, pixels);

}

int main(){
    auto tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
    Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

    bool ledOn = true;

    int lastPlayer = 1;

    unsigned long localTime = millis();
    Player1 player1;
    Player2 player2;
    Ball ball;


    setup(tft, localTime, player1, player2);
    while(true){
        try {
            loop(ledOn, tft, ball, player1, player2, lastPlayer, pixels);
        }
        catch (const std::exception &e){
            std::cout << e.what();
            return 0;
        }
    }
}