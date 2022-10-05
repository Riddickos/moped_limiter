#ifndef TIMER1_H
#define TIMER1_H

#include <avr/io.h>
#include <avr/interrupt.h>

#define byte uint8_t
#define TIMER_FRQ_Hz 16000000.0
#define MAX_OCR1A 65535.0
#define STEP1 (((MAX_OCR1A + 1) * 2 * 1) / TIMER_FRQ_Hz)
#define STEP2 (((MAX_OCR1A + 1) * 2 * 8) / TIMER_FRQ_Hz)
#define STEP3 (((MAX_OCR1A + 1) * 2 * 64) / TIMER_FRQ_Hz)
#define STEP4 (((MAX_OCR1A + 1) * 2 * 256) / TIMER_FRQ_Hz)
#define STEP5 (((MAX_OCR1A + 1) * 2 * 1024) / TIMER_FRQ_Hz)

class timer1
{
private:
    byte m_prescaling;
    uint16_t m_OCR1A;
    double m_duration;

private:
    timer1();

public:
    static timer1& instance() {
        static timer1 object;
        return object;
    }
    timer1(timer1 const&) = delete;
    void operator=(timer1 const&) = delete;

    void init(double duration);
    double duration();
    void start();
    void stop();
};

#endif
