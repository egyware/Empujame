#ifndef TIMER_H
#define TIMER_H

template <class T>
class Timer {
    typedef void (*callback_t)(Timer *timer);
public:
    T *userData;
    Timer() {
        m_countdown = 0;
        userData = NULL;
        m_callback = NULL;
    }
    Timer( unsigned int cntdwn, T *userD , callback_t call ) {
        m_countdown = cntdwn;
        userData = userD;
        m_callback = call;
    }
    ~Timer() {
        m_callback = NULL;
    }
    void update(unsigned int delta) {
        if( m_countdown <= delta ) {
            m_countdown = 0;
            if(m_callback != NULL) {
                (*m_callback)(this);
            }
        } else {
            m_countdown -= delta;
        }
    }
    inline void setTimer(unsigned int cntdwn) {
        m_countdown = cntdwn;
    }
    inline void setCallBack( callback_t call ) {
        m_callback = call;
    }
    inline unsigned int getTimer() {
        return m_countdown;
    }
private:
    unsigned int m_countdown;
    callback_t m_callback;
};

#endif // TIMER_H
