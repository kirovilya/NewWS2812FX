#include <WiFiUdp.h>


//typedef struct
//{
//  uint8_t G; // G,R,B order is determined by WS2812B
//  uint8_t R;
//  uint8_t B;
//} Pixel_t;


// Maximum number of packets to hold in the buffer. Don't change this.
#define BUFFER_LEN 4096
// Toggles FPS output (1 = print FPS over serial, 0 = disable output)
#define PRINT_FPS 1
unsigned int localPort = 7777;
char packetBuffer[BUFFER_LEN];
//static Pixel_t pixels[NUMLEDS];
WiFiUDP rtport;
uint16_t N = 0;
#if PRINT_FPS
    uint16_t fpsCounter = 0;
    uint32_t secondTimer = 0;
#endif

void setup_realtime(){
    rtport.begin(localPort);
    DBG_OUTPUT_PORT.printf("Open port %d for UDP realtime data\n", localPort);
}

void realtime(){
    //DBG_OUTPUT_PORT.println("realtime");
    if (exit_func) {
        exit_func = false;
        return;
    }
    //DBG_OUTPUT_PORT.println("parse packet");
    // Read data over socket
    int packetSize = rtport.parsePacket();
    // If packets have been received, interpret the command
    if (packetSize) {
        //DBG_OUTPUT_PORT.println("read packet");
        int len = rtport.read(packetBuffer, BUFFER_LEN);
        //Serial.printf("get packet %d\n", len);
        //DBG_OUTPUT_PORT.printf("get packet %d : %d\n", len, packetSize);
        for(int i = 0; i < len; i+=5) {
            packetBuffer[len] = 0;
            N = packetBuffer[i] + 256*packetBuffer[i+1];
            if (N >= NUMLEDS) {
                continue;
            }
            //pixels[N].R = (uint8_t)packetBuffer[i+2];
            //pixels[N].G = (uint8_t)packetBuffer[i+3];
            //pixels[N].B = (uint8_t)packetBuffer[i+4];
            //DBG_OUTPUT_PORT.printf("%d %d %d %d\n", N, pixels[N].R, pixels[N].G, pixels[N].B);
            strip.setPixelColor(N, (uint8_t)packetBuffer[i+2], (uint8_t)packetBuffer[i+3], (uint8_t)packetBuffer[i+4]);
        }
        //WS2812FX strip
        //DBG_OUTPUT_PORT.println("write to strip");
        //for (int i=0; i<NUMLEDS; i++) {
        //    strip.setPixelColor(i, pixels[i].R, pixels[i].G, pixels[i].B);
            //delay(wait);
        //    strip.show();
            //delay(wait);
        //}
        strip.show();
        #if PRINT_FPS
            fpsCounter++;
        #endif
    }
    #if PRINT_FPS
        if (millis() - secondTimer >= 1000U) {
            secondTimer = millis();
            //Serial.printf("FPS: %d\n", fpsCounter);
            DBG_OUTPUT_PORT.printf("FPS: %d\n", fpsCounter);
            fpsCounter = 0;
        }   
    #endif
}
