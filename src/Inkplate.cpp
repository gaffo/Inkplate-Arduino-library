#include "Inkplate.h"

#define CL 0x01
#define CL_SET                                                                                                         \
    {                                                                                                                  \
        GPIO.out_w1ts = CL;                                                                                            \
    }
#define CL_CLEAR                                                                                                       \
    {                                                                                                                  \
        GPIO.out_w1tc = CL;                                                                                            \
    }
#define CKV 0x01
#define CKV_SET                                                                                                        \
    {                                                                                                                  \
        GPIO.out1_w1ts.val = CKV;                                                                                      \
    }
#define CKV_CLEAR                                                                                                      \
    {                                                                                                                  \
        GPIO.out1_w1tc.val = CKV;                                                                                      \
    }
#define SPH 0x02
#define SPH_SET                                                                                                        \
    {                                                                                                                  \
        GPIO.out1_w1ts.val = SPH;                                                                                      \
    }
#define SPH_CLEAR                                                                                                      \
    {                                                                                                                  \
        GPIO.out1_w1tc.val = SPH;                                                                                      \
    }
#define LE 0x04
#define LE_SET                                                                                                         \
    {                                                                                                                  \
        GPIO.out_w1ts = LE;                                                                                            \
    }
#define LE_CLEAR                                                                                                       \
    {                                                                                                                  \
        GPIO.out_w1tc = LE;                                                                                            \
    }
#define OE 0
#define OE_SET                                                                                                         \
    {                                                                                                                  \
        digitalWriteMCP(OE, HIGH);                                                                                     \
    }
#define OE_CLEAR                                                                                                       \
    {                                                                                                                  \
        digitalWriteMCP(OE, LOW);                                                                                      \
    }
#define GMOD 1
#define GMOD_SET                                                                                                       \
    {                                                                                                                  \
        digitalWriteMCP(GMOD, HIGH);                                                                                   \
    }
#define GMOD_CLEAR                                                                                                     \
    {                                                                                                                  \
        digitalWriteMCP(GMOD, LOW);                                                                                    \
    }
#define SPV 2
#define SPV_SET                                                                                                        \
    {                                                                                                                  \
        digitalWriteMCP(SPV, HIGH);                                                                                    \
    }
#define SPV_CLEAR                                                                                                      \
    {                                                                                                                  \
        digitalWriteMCP(SPV, LOW);                                                                                     \
    }
#define WAKEUP 3
#define WAKEUP_SET                                                                                                     \
    {                                                                                                                  \
        digitalWriteMCP(WAKEUP, HIGH);                                                                                 \
    }
#define WAKEUP_CLEAR                                                                                                   \
    {                                                                                                                  \
        digitalWriteMCP(WAKEUP, LOW);                                                                                  \
    }
#define PWRUP 4
#define PWRUP_SET                                                                                                      \
    {                                                                                                                  \
        digitalWriteMCP(PWRUP, HIGH);                                                                                  \
    }
#define PWRUP_CLEAR                                                                                                    \
    {                                                                                                                  \
        digitalWriteMCP(PWRUP, LOW);                                                                                   \
    }
#define VCOM 5
#define VCOM_SET                                                                                                       \
    {                                                                                                                  \
        digitalWriteMCP(VCOM, HIGH);                                                                                   \
    }
#define VCOM_CLEAR                                                                                                     \
    {                                                                                                                  \
        digitalWriteMCP(VCOM, LOW);                                                                                    \
    }

#define GPIO0_ENABLE 8

#define DATA 0x0E8C0030

Inkplate::Inkplate(uint8_t _mode) : Adafruit_GFX(E_INK_WIDTH, E_INK_HEIGHT), Graphics(E_INK_WIDTH, E_INK_HEIGHT)
{
    setDisplayMode(_mode);
    for (uint32_t i = 0; i < 256; ++i)
        pinLUT[i] = ((i & B00000011) << 4) | (((i & B00001100) >> 2) << 18) | (((i & B00010000) >> 4) << 23) |
                    (((i & B11100000) >> 5) << 25);
}

void Inkplate::begin(void)
{
    if (_beginDone == 1)
        return;

    Wire.begin();

    mcpBegin(MCP23017_ADDR, mcpRegsInt);
    pinModeMCP(VCOM, OUTPUT);
    pinModeMCP(PWRUP, OUTPUT);
    pinModeMCP(WAKEUP, OUTPUT);
    pinModeMCP(GPIO0_ENABLE, OUTPUT);
    digitalWriteMCP(GPIO0_ENABLE, HIGH);

    // CONTROL PINS
    pinMode(0, OUTPUT);
    pinMode(2, OUTPUT);
    pinMode(32, OUTPUT);
    pinMode(33, OUTPUT);
    pinModeMCP(OE, OUTPUT);
    pinModeMCP(GMOD, OUTPUT);
    pinModeMCP(SPV, OUTPUT);

    // DATA PINS
    pinMode(4, OUTPUT); // D0
    pinMode(5, OUTPUT);
    pinMode(18, OUTPUT);
    pinMode(19, OUTPUT);
    pinMode(23, OUTPUT);
    pinMode(25, OUTPUT);
    pinMode(26, OUTPUT);
    pinMode(27, OUTPUT); // D7

    // TOUCHPAD PINS
    pinModeMCP(10, INPUT);
    pinModeMCP(11, INPUT);
    pinModeMCP(12, INPUT);

    // Battery voltage Switch MOSFET
    pinModeMCP(9, OUTPUT);

    DMemoryNew = (uint8_t *)ps_malloc(600 * 100);
    _partial = (uint8_t *)ps_malloc(600 * 100);
    _pBuffer = (uint8_t *)ps_malloc(120000);
    D_memory4Bit = (uint8_t *)ps_malloc(240000);

    if (DMemoryNew == NULL || _partial == NULL || _pBuffer == NULL || D_memory4Bit == NULL)
    {
        do
        {
            delay(100);
        } while (true);
    }
    memset(DMemoryNew, 0, 60000);
    memset(_partial, 0, 60000);
    memset(_pBuffer, 0, 120000);
    memset(D_memory4Bit, 255, 240000);

    precalculateGamma(gammaLUT, 1);

    _beginDone = 1;
}

void Inkplate::precalculateGamma(uint8_t *c, float gamma)
{
    for (int i = 0; i < 256; ++i)
    {
        c[i] = int(round((pow(i / 255.0, gamma)) * 15));
    }
}

void Inkplate::clearDisplay()
{
    if (getDisplayMode() == 0)
        memset(_partial, 0, 60000);
    else if (getDisplayMode() == 1)
        memset(D_memory4Bit, 255, 240000);
}

void Inkplate::display()
{
    if (getDisplayMode() == 0)
        display1b();
    else if (getDisplayMode() == 1)
        display3b();
}

void Inkplate::display1b()
{
    memcpy(DMemoryNew, _partial, 60000);

    uint32_t _send;
    uint8_t data;
    uint8_t dram;
    einkOn();
    cleanFast(0, 1);
    cleanFast(1, 16);
    cleanFast(2, 1);
    cleanFast(0, 11);
    cleanFast(2, 1);
    cleanFast(1, 16);
    cleanFast(2, 1);
    cleanFast(0, 11);
    for (int k = 0; k < 4; ++k)
    {
        uint8_t *DMemoryNewPtr = DMemoryNew + 59999;
        vscan_start();
        for (int i = 0; i < 600; ++i)
        {
            dram = *(DMemoryNewPtr--);
            data = LUTB[dram >> 4];
            _send = pinLUT[data];
            hscan_start(_send);
            data = LUTB[dram & 0x0F];
            _send = pinLUT[data];
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;

            for (int j = 0; j < 99; ++j)
            {
                dram = *(DMemoryNewPtr--);
                data = LUTB[dram >> 4];
                _send = pinLUT[data];
                GPIO.out_w1ts = (_send) | CL;
                GPIO.out_w1tc = DATA | CL;
                data = LUTB[dram & 0x0F];
                _send = pinLUT[data];
                GPIO.out_w1ts = (_send) | CL;
                GPIO.out_w1tc = DATA | CL;
            }
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;
            vscan_end();
        }
        delayMicroseconds(230);
    }

    uint16_t _pos = 59999;
    vscan_start();
    for (int i = 0; i < 600; ++i)
    {
        dram = *(DMemoryNew + _pos);
        data = LUT2[dram >> 4];
        _send = pinLUT[data];
        hscan_start(_send);
        data = LUT2[dram & 0x0F];
        _send = pinLUT[data];
        GPIO.out_w1ts = (_send) | CL;
        GPIO.out_w1tc = DATA | CL;
        _pos--;
        for (int j = 0; j < 99; ++j)
        {
            dram = *(DMemoryNew + _pos);
            data = LUT2[dram >> 4];
            _send = pinLUT[data];
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;
            data = LUT2[dram & 0x0F];
            _send = pinLUT[data];
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;
            _pos--;
        }
        GPIO.out_w1ts = (_send) | CL;
        GPIO.out_w1tc = DATA | CL;
        vscan_end();
    }
    delayMicroseconds(230);

    vscan_start();
    for (int i = 0; i < 600; ++i)
    {
        dram = *(DMemoryNew + _pos);
        data = 0;
        _send = pinLUT[data];
        hscan_start(_send);
        data = 0;
        GPIO.out_w1ts = (_send) | CL;
        GPIO.out_w1tc = DATA | CL;
        for (int j = 0; j < 99; ++j)
        {
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;
        }
        GPIO.out_w1ts = (_send) | CL;
        GPIO.out_w1tc = DATA | CL;
        vscan_end();
    }
    delayMicroseconds(230);

    vscan_start();
    einkOff();
    _blockPartial = 0;
}

void Inkplate::display3b()
{
    einkOn();
    cleanFast(0, 1);
    cleanFast(1, 16);
    cleanFast(2, 1);
    cleanFast(0, 11);
    cleanFast(2, 1);
    cleanFast(1, 16);
    cleanFast(2, 1);
    cleanFast(0, 11);

    for (int k = 0; k < 8; ++k)
    {
        uint8_t *dp = D_memory4Bit + 239999;
        uint32_t _send;
        uint8_t pix1;
        uint8_t pix2;
        uint8_t pix3;
        uint8_t pix4;
        uint8_t pixel;
        uint8_t pixel2;

        vscan_start();
        for (int i = 0; i < 600; ++i)
        {
            pixel = 0;
            pixel2 = 0;
            pix1 = *(dp--);
            pix2 = *(dp--);
            pix3 = *(dp--);
            pix4 = *(dp--);
            pixel |= (waveform3Bit[pix1 & 0x07][k] << 6) | (waveform3Bit[(pix1 >> 4) & 0x07][k] << 4) |
                     (waveform3Bit[pix2 & 0x07][k] << 2) | (waveform3Bit[(pix2 >> 4) & 0x07][k] << 0);
            pixel2 |= (waveform3Bit[pix3 & 0x07][k] << 6) | (waveform3Bit[(pix3 >> 4) & 0x07][k] << 4) |
                      (waveform3Bit[pix4 & 0x07][k] << 2) | (waveform3Bit[(pix4 >> 4) & 0x07][k] << 0);

            _send = pinLUT[pixel];
            hscan_start(_send);
            _send = pinLUT[pixel2];
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;

            for (int j = 0; j < 99; ++j)
            {
                pixel = 0;
                pixel2 = 0;
                pix1 = *(dp--);
                pix2 = *(dp--);
                pix3 = *(dp--);
                pix4 = *(dp--);
                pixel |= (waveform3Bit[pix1 & 0x07][k] << 6) | (waveform3Bit[(pix1 >> 4) & 0x07][k] << 4) |
                         (waveform3Bit[pix2 & 0x07][k] << 2) | (waveform3Bit[(pix2 >> 4) & 0x07][k] << 0);
                pixel2 |= (waveform3Bit[pix3 & 0x07][k] << 6) | (waveform3Bit[(pix3 >> 4) & 0x07][k] << 4) |
                          (waveform3Bit[pix4 & 0x07][k] << 2) | (waveform3Bit[(pix4 >> 4) & 0x07][k] << 0);

                _send = pinLUT[pixel];
                GPIO.out_w1ts = (_send) | CL;
                GPIO.out_w1tc = DATA | CL;

                _send = pinLUT[pixel2];
                GPIO.out_w1ts = (_send) | CL;
                GPIO.out_w1tc = DATA | CL;
            }
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;
            vscan_end();
        }
        delayMicroseconds(230);
    }
    cleanFast(3, 1);
    vscan_start();
    einkOff();
}

void Inkplate::partialUpdate()
{
    if (getDisplayMode() == 1)
        return;
    if (_blockPartial == 1)
        display1b();

    uint16_t _pos = 59999;
    uint32_t _send;
    uint8_t data = 0;
    uint8_t diffw, diffb;
    uint32_t n = 119999;

    for (int i = 0; i < 600; ++i)
    {
        for (int j = 0; j < 100; ++j)
        {
            diffw = *(DMemoryNew + _pos) & ~*(_partial + _pos);
            diffb = ~*(DMemoryNew + _pos) & *(_partial + _pos);
            _pos--;
            *(_pBuffer + n) = LUTW[diffw >> 4] & (LUTB[diffb >> 4]);
            n--;
            *(_pBuffer + n) = LUTW[diffw & 0x0F] & (LUTB[diffb & 0x0F]);
            n--;
        }
    }

    einkOn();
    for (int k = 0; k < 5; ++k)
    {
        vscan_start();
        n = 119999;
        for (int i = 0; i < 600; ++i)
        {
            data = *(_pBuffer + n);
            _send = pinLUT[data];
            hscan_start(_send);
            n--;
            for (int j = 0; j < 199; ++j)
            {
                data = *(_pBuffer + n);
                _send = pinLUT[data];
                GPIO.out_w1ts = _send | CL;
                GPIO.out_w1tc = DATA | CL;
                n--;
            }
            GPIO.out_w1ts = _send | CL;
            GPIO.out_w1tc = DATA | CL;
            vscan_end();
        }
        delayMicroseconds(230);
    }
    cleanFast(2, 2);
    cleanFast(3, 1);
    vscan_start();
    einkOff();

    memcpy(DMemoryNew, _partial, 60000);
}

void Inkplate::clean()
{
    einkOn();
    int m = 0;
    cleanFast(0, 1);
    m++;
    cleanFast((waveform[m] >> 30) & 3, 8);
    m++;
    cleanFast((waveform[m] >> 24) & 3, 1);
    m++;
    cleanFast((waveform[m]) & 3, 8);
    m++;
    cleanFast((waveform[m] >> 6) & 3, 1);
    m++;
    cleanFast((waveform[m] >> 30) & 3, 10);
}

void Inkplate::cleanFast(uint8_t c, uint8_t rep)
{
    einkOn();
    uint8_t data = 0;
    if (c == 0)
        data = B10101010;
    else if (c == 1)
        data = B01010101;
    else if (c == 2)
        data = B00000000;
    else if (c == 3)
        data = B11111111;

    uint32_t _send = pinLUT[data];
    for (int k = 0; k < rep; ++k)
    {
        vscan_start();
        for (int i = 0; i < 600; ++i)
        {
            hscan_start(_send);
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;
            for (int j = 0; j < 99; ++j)
            {
                GPIO.out_w1ts = (_send) | CL;
                GPIO.out_w1tc = DATA | CL;
                GPIO.out_w1ts = (_send) | CL;
                GPIO.out_w1tc = DATA | CL;
            }
            GPIO.out_w1ts = (_send) | CL;
            GPIO.out_w1tc = DATA | CL;
            vscan_end();
        }
        delayMicroseconds(230);
    }
}

// Turn off epaper power supply and put all digital IO pins in high Z state
void Inkplate::einkOff()
{
    if (getPanelState() == 0)
        return;
    OE_CLEAR;
    GMOD_CLEAR;
    GPIO.out &= ~(DATA | LE | CL);
    CKV_CLEAR;
    SPH_CLEAR;
    SPV_CLEAR;

    VCOM_CLEAR;
    delay(6);
    PWRUP_CLEAR;
    WAKEUP_CLEAR;
    delay(50);
    pinsZstate();
    setPanelState(0);
}

// Turn on supply for epaper display (TPS65186) [+15 VDC, -15VDC, +22VDC, -20VDC, +3.3VDC, VCOM]
void Inkplate::einkOn()
{
    if (getPanelState() == 1)
        return;
    WAKEUP_SET;
    delay(1);
    Wire.beginTransmission(0x48);
    Wire.write(0x09);
    Wire.write(B00011011); // Power up seq.
    Wire.write(B00000000); // Power up delay (3mS per rail)
    Wire.write(B00011011); // Power down seq.
    Wire.write(B00000000); // Power down delay (6mS per rail)
    Wire.endTransmission();

    PWRUP_SET;

    // Enable all rails
    Wire.beginTransmission(0x48);
    Wire.write(0x01);
    Wire.write(B00111111);
    Wire.endTransmission();
    pinsAsOutputs();
    LE_CLEAR;
    OE_CLEAR;
    CL_CLEAR;
    SPH_SET;
    GMOD_SET;
    SPV_SET;
    CKV_CLEAR;
    OE_CLEAR;
    VCOM_SET;
    Wire.beginTransmission(0x48);
    Wire.write(0x0D);
    Wire.write(B10000000);
    Wire.endTransmission();
    delay(5);

    Wire.beginTransmission(0x48);
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.requestFrom(0x48, 1);
    setTemperature(Wire.read());

    pinModeMCP(7, INPUT_PULLUP);
    while (!digitalReadMCP(7))
        ;
    OE_SET;
    setPanelState(1);
}

// LOW LEVEL FUNCTIONS

void Inkplate::vscan_start()
{
    CKV_SET;
    delayMicroseconds(7);
    SPV_CLEAR;
    delayMicroseconds(10);
    CKV_CLEAR;
    delayMicroseconds(0);
    CKV_SET;
    delayMicroseconds(8);
    SPV_SET;
    delayMicroseconds(10);
    CKV_CLEAR;
    delayMicroseconds(0);
    CKV_SET;
    delayMicroseconds(18);
    CKV_CLEAR;
    delayMicroseconds(0);
    CKV_SET;
    delayMicroseconds(18);
    CKV_CLEAR;
    delayMicroseconds(0);
    CKV_SET;
}

void Inkplate::hscan_start(uint32_t _d)
{
    SPH_CLEAR;
    GPIO.out_w1ts = (_d) | CL;
    GPIO.out_w1tc = DATA | CL;
    SPH_SET;
    CKV_SET;
}

void Inkplate::vscan_end()
{
    CKV_CLEAR;
    LE_SET;
    LE_CLEAR;
    delayMicroseconds(0);
}

void Inkplate::pinsZstate()
{
    pinMode(0, INPUT);
    pinMode(2, INPUT);
    pinMode(32, INPUT);
    pinMode(33, INPUT);
    pinModeMCP(OE, INPUT);
    pinModeMCP(GMOD, INPUT);
    pinModeMCP(SPV, INPUT);

    pinMode(4, INPUT);
    pinMode(5, INPUT);
    pinMode(18, INPUT);
    pinMode(19, INPUT);
    pinMode(23, INPUT);
    pinMode(25, INPUT);
    pinMode(26, INPUT);
    pinMode(27, INPUT);
}

void Inkplate::pinsAsOutputs()
{
    pinMode(0, OUTPUT);
    pinMode(2, OUTPUT);
    pinMode(32, OUTPUT);
    pinMode(33, OUTPUT);
    pinModeMCP(OE, OUTPUT);
    pinModeMCP(GMOD, OUTPUT);
    pinModeMCP(SPV, OUTPUT);

    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(18, OUTPUT);
    pinMode(19, OUTPUT);
    pinMode(23, OUTPUT);
    pinMode(25, OUTPUT);
    pinMode(26, OUTPUT);
    pinMode(27, OUTPUT);
}