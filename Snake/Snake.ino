#include "pixellist.h"
#include <FreeSixIMU.h>
#include <FIMU_ADXL345.h>
#include <FIMU_ITG3200.h>
#include <Adafruit_NeoMatrix.h>
#include <Wire.h>

// Inherit from this abstract class to create your cyclic task
class CyclicTask
{
public:
    CyclicTask(uint32_t u32CyclicRate) : m_u32CyclicRate(u32CyclicRate), m_u32CyclicTimer(0) {};

    void vUpdateClk(uint32_t u32CurrTime)
    {
        if (u32CurrTime - m_u32CyclicTimer >= m_u32CyclicRate)
        {
            m_u32CyclicTimer = u32CurrTime;
            vProcess();
        }
    };

    // Implement your functionality in this function
    virtual void vProcess() = 0;

protected:
    uint32_t m_u32CyclicRate;
    uint32_t m_u32CyclicTimer;
};

uint16_t g_au16RainbowPalette[] = { 0xF800, 0xFC00, 0xFFE0, 0x07E0, 0x001F, 0x801F, 0xF81F };

class GyroSense : public CyclicTask
{
public:
    GyroSense(uint32_t u32CyclicRate) : CyclicTask(u32CyclicRate) {};

    void vInit() { m_oGyro.init(); };

    float fGetYaw(void) { return m_afAngles[0]; };
    float fGetPitch(void) { return m_afAngles[1]; };
    float fGetRoll(void) { return m_afAngles[2]; };

protected:
    FreeSixIMU m_oGyro;
    float m_afAngles[3]; // yaw pitch roll

    virtual void vProcess()
    {
        m_oGyro.getYawPitchRoll(m_afAngles);
        //Serial.print(m_afAngles[0]);
        //Serial.print(" | ");
        //Serial.print(m_afAngles[1]);
        //Serial.print(" | ");
        //Serial.println(m_afAngles[2]);
    };
};


class Snake : public CyclicTask
{
public:
    static uint16_t u16ApplyFade(uint16_t u16Color, uint8_t u8FadeFactor)
    {
        uint32_t r = ((u16Color  >> 11) * u8FadeFactor) >> 8;
        uint32_t g = (((u16Color & 0x07E0) >> 5) * u8FadeFactor) >> 8;
        uint32_t b = ((u16Color  & 0x001F) * u8FadeFactor) >> 8;
        return static_cast<int16_t> ((r << 11) + (g << 5) + b);
    }


    enum tenMoveDir { LEFT, RIGHT, UP, DOWN };
    Snake(uint32_t u32CyclicRate, Adafruit_NeoMatrix *poDisplay, GyroSense *poGyro, uint16_t *pau16Palette, uint16_t u16PaletteSize)
        : CyclicTask(u32CyclicRate), m_poDisplay(poDisplay), m_poGyro(poGyro), m_pau16Palette(pau16Palette), m_u16PaletteSize(u16PaletteSize)
    {
        m_oFood.xPos = random(m_poDisplay->width());
        m_oFood.yPos = random(m_poDisplay->height());
        m_u16ColorIdx = 0;
        vResetState();
    };

protected:
    Adafruit_NeoMatrix *m_poDisplay;
    GyroSense *m_poGyro;
    uint16_t   m_u16HeadColor;
    uint16_t   m_u16TailColor;
    uint16_t  *m_pau16Palette;
    uint16_t   m_u16PaletteSize;
    uint16_t   m_u16ColorIdx;
    tenMoveDir m_enDir;
    PixelList  m_oSnake;
    Pixel      m_oFood;
    bool       m_boGrowBigger;

    virtual void vProcess()
    {
        // 1. Clear screen
        m_poDisplay->fillScreen(0);

        // 2. Draw Food
        m_poDisplay->drawPixel(m_oFood.xPos, m_oFood.yPos, m_u16HeadColor);

        // 3. Draw snake
        vDrawSnake();

        // 4. check food against head, set flag to growBigger, generate a new food in different location
        vEatIfYouCan();

        // 5. calculate next position, check for game over if head hits body
        vCalcNewPos();

        m_poDisplay->show();
    };

private:
    void vCalcNewPos(void)
    {
        int iRoll = static_cast<int>(m_poGyro->fGetRoll());
        int iPitch = static_cast<int>(m_poGyro->fGetPitch());
        bool boChangeDir = false;
        Pixel oNewPix(m_oSnake.pHead->xPos, m_oSnake.pHead->yPos);


        //if direction is U or D, movement can only be L/R
        if ((m_enDir == UP) || (m_enDir == DOWN))
        {
            if (iRoll >= 10) {   // +X movement
                boChangeDir = true;
                m_enDir = RIGHT;
                vMoveRight(oNewPix);
            }
            else if (iRoll <= -10) {
                boChangeDir = true;
                m_enDir = LEFT;
                vMoveLeft(oNewPix);
            }
        }
        else if ((m_enDir == LEFT) || (m_enDir == RIGHT)) //if direction is L or R, movement can only be Up/Down
        {
            if (iPitch >= 8) { //+y
                boChangeDir = true;
                m_enDir = DOWN;
                vMoveDown(oNewPix);
            }
            else if (iPitch <= -8) { //-y
                boChangeDir = true;
                m_enDir = UP;
                vMoveUp(oNewPix);
            }
        }

        if (boChangeDir == false)
        {
            switch (m_enDir) {
            case RIGHT:
                vMoveRight(oNewPix);
                break;

            case LEFT:
                vMoveLeft(oNewPix);
                break;

            case DOWN:
                vMoveDown(oNewPix);
                break;

            case UP:
                vMoveUp(oNewPix);
                break;
            }
        }
    
        if (boCheckCollision(oNewPix) == false) //doesn't collide
        {
            m_oSnake.vPushFront(oNewPix);
            if (!m_boGrowBigger) //dont grow bigger, you didnt eat
                m_oSnake.oPopBack();
            else
                m_boGrowBigger = false;
        }
        else
        {
            m_oSnake.vClearList();
            vResetState();
        }
    }

    void vDrawSnake(void)
    {
        Pixel *it = m_oSnake.pHead;
        
        while (it)
        {
            m_poDisplay->drawPixel(it->xPos, it->yPos, (it == m_oSnake.pHead ? m_u16HeadColor : m_u16TailColor) );
            it = it->pNext;
        }

    }

    void vMoveRight(Pixel &oNewPix)
    {
        if (oNewPix.xPos + 1 < m_poDisplay->width())
        {
            oNewPix.xPos += 1;
        }
        else
        {
            oNewPix.xPos = 0; //wrap around imp
        }
    }

    void vMoveLeft(Pixel &oNewPix)
    {
        if (oNewPix.xPos - 1 >= 0)
        {
            oNewPix.xPos -= 1;
        }
        else
        {
            oNewPix.xPos = m_poDisplay->width() - 1; //wrap
        }
    }

    void vMoveUp(Pixel &oNewPix)
    {
        if (oNewPix.yPos - 1 >= 0)
        {
            oNewPix.yPos -= 1;
        }
        else
        {
            oNewPix.yPos = m_poDisplay->height() - 1; //wrap
        }
    }

    void vMoveDown(Pixel &oNewPix)
    {
        if (oNewPix.yPos + 1 < m_poDisplay->height())
        {
            oNewPix.yPos += 1;
        }
        else
        {
            oNewPix.yPos = 0; //wrap
        }
    }

    void vResetState(void)
    {
        //reset condition
        m_enDir = RIGHT;
        m_oSnake.vPushFront(Pixel(0, 0));
        m_oSnake.vPushFront(Pixel(1, 0));
        m_oSnake.vPushFront(Pixel(2, 0));

        if (m_u16ColorIdx + 1 < m_u16PaletteSize)
            ++m_u16ColorIdx;
        else
            m_u16ColorIdx = 0;

        m_u16HeadColor = u16ApplyFade(m_pau16Palette[m_u16ColorIdx], 128);
        m_u16TailColor = u16ApplyFade(m_pau16Palette[m_u16ColorIdx], 64);
        //m_u16HeadColor = m_pau16Palette[m_u16ColorIdx];
        //m_u16TailColor = m_pau16Palette[m_u16ColorIdx];

        vResetFood();
    }

    // returns true IF pixel collides with body
    bool boCheckCollision(Pixel &oPixel)
    {
        Pixel *it = m_oSnake.pHead;

        while (it)
        {
            if ((it->xPos == oPixel.xPos) && (it->yPos == oPixel.yPos))
                return true;
            it = it->pNext;
        }

        return false; //does not collide
    }

    void vEatIfYouCan(void)
    {
        if ((m_oFood.xPos == m_oSnake.pHead->xPos) &&
            (m_oFood.yPos == m_oSnake.pHead->yPos))
        {
            m_boGrowBigger = true;

            vResetFood();
        }
    }

    void vResetFood(void)
    {
        do
        {
            m_oFood.xPos = random(m_poDisplay->width());
            m_oFood.yPos = random(m_poDisplay->height());
        } while (boCheckCollision(m_oFood)); //if new food is inside body, generate a new one
    }
};

#define MATRIX_PIN 6

//Roll is affects movement in X direction
// [0, 90] = +x
// [0, -90] = -x

// Setup for landscape mode
Adafruit_NeoMatrix matrix(8, 5, MATRIX_PIN,
    NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
    NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
    NEO_GRB + NEO_KHZ800);

GyroSense mySensor(50);
Snake myPixel(300, &matrix, &mySensor, g_au16RainbowPalette, sizeof(g_au16RainbowPalette)/sizeof(uint16_t));

void setup() {
    Serial.begin(9600);
    Wire.begin();
    matrix.begin();
    matrix.setBrightness(64);
    matrix.fillScreen(0x0); //black

    randomSeed(analogRead(0));

    delay(5);
    mySensor.vInit();
    delay(5);
}

void loop() {

    mySensor.vUpdateClk(millis());
    myPixel.vUpdateClk(millis());
}

