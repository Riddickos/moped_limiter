#include "timer1.h"

timer1::timer1() : m_prescaling(0), m_OCR1A(0), m_duration(0.0) {}

void timer1::init(double duration)
{
    m_duration = duration;

    if (duration < STEP1) {
        m_OCR1A = static_cast<unsigned int>(duration * TIMER_FRQ_Hz / (2 * 1) - 1);
        m_prescaling = 1 << CS10;
    } else if (duration < STEP2) {
        m_OCR1A = static_cast<unsigned int>(duration * TIMER_FRQ_Hz / (2 * 8) - 1);
        m_prescaling = 1 << CS11;
    } else if (duration < STEP3) {
        m_OCR1A = static_cast<unsigned int>(duration * TIMER_FRQ_Hz / (2 * 64) - 1);
        m_prescaling = 1 << CS11 | 1 << CS10;
    } else if (duration < STEP4) {
        m_OCR1A = static_cast<unsigned int>(duration * TIMER_FRQ_Hz / (2 * 256) - 1);
        m_prescaling = 1 << CS12;
    } else if (duration < STEP5) {
        m_OCR1A = static_cast<unsigned int>(duration * TIMER_FRQ_Hz / (2 * 1024) - 1);
        m_prescaling = 1 << CS12 | 1 << CS10;
    } else {
        m_duration = 0.0;
    }
}

double timer1::duration()
{
    return m_duration;
}

void timer1::start()
{
    TIMSK1 = 1 << OCIE1A;
    OCR1A = m_OCR1A;
    TCCR1A = 0;
    TCCR1B = 1 << WGM12 | m_prescaling;
}

void timer1::stop()
{
    TCCR1B = 0; // Turn-off timer
}

