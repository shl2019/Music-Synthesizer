#include <Arduino.h>
#include <U8g2lib.h>
#include <cmath>
#include <math.h>
#include <list>
#include <STM32FreeRTOS.h>
#include <vector>
#include <string>
using namespace std;
class knob
{
    // Access specifier
    private:
    int8_t previous_A=0;
    int8_t previouos_B=0;
    int8_t current_A=0;
    int8_t current_B=0;
    int8_t currentVolume=0;
    int8_t volume=0;
    int8_t scale = 16;
    // Member Functions()
    int find_rotation_variable(){
        if(previous_A!=current_A && previouos_B!=current_B){
        return currentVolume*2;
        }else if(previous_A == 0 && previouos_B == 0){
        return -current_A+current_B;
        }else if(previous_A == 0 && previouos_B == 1){
        return current_A-1+current_B;
        }else if(previous_A == 1 && previouos_B == 0){
        return -current_A-current_B+1;
        }else if(previous_A == 1 && previouos_B == 1){
        return current_A-current_B;
        }else{
        return 0;
        }

    }
    public:
    int8_t updateKnob(int8_t A,int8_t B){
        previous_A=current_A;
        previouos_B = current_B;
        current_A= A;
        current_B = B;
        currentVolume = find_rotation_variable();
        volume = volume - currentVolume;
        if(volume>scale){
            volume=scale;
        }else if(volume<0){
            volume = 0;
        }
        return volume;
    }
    void setVolume(int8_t v){
        volume=v;
    }
    int8_t getVolume(){
        return volume;
    }
    void setScale(int8_t s){
        scale = s;
    }
};