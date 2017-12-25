// pixellist.h

#ifndef _PIXELLIST_h
#define _PIXELLIST_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

struct Pixel
{
    Pixel(int x = 0, int y = 0) : xPos(x), yPos(y), pNext(0), pPrev(0) {};

    int xPos;
    int yPos;
    Pixel *pNext;
    Pixel *pPrev;
};

struct PixelList
{
    PixelList() : pHead(0), pTail(0) {};
    ~PixelList()
    {
        while (pHead != 0)
        {
            Pixel *pTemp = pHead;
            pHead = pHead->pNext;

            //cout << "freeing " << pTemp->xPos << "," << pTemp->yPos << endl;
            free(pTemp);
        }
    };


    void vPushBack(Pixel oPixel)
    {
        Pixel *pNode = (Pixel *)malloc(sizeof(Pixel));
        pNode->xPos = oPixel.xPos;
        pNode->yPos = oPixel.yPos;
        pNode->pNext = 0;
        pNode->pPrev = 0;

        if (pHead == 0) //First entry
        {
            pHead = pNode;
            pTail = pNode;
        }
        else
        {
            pTail->pNext = pNode;
            pNode->pPrev = pTail;
            pTail = pNode;
        }
    }

    void vPushFront(Pixel oPixel)
    {
        Pixel *pNode = (Pixel *)malloc(sizeof(Pixel));
        pNode->xPos = oPixel.xPos;
        pNode->yPos = oPixel.yPos;
        pNode->pNext = 0;
        pNode->pPrev = 0;

        if (pHead == 0)
        {
            pHead = pNode;
            pTail = pNode;
        }
        else
        {
            pNode->pNext = pHead;
            pHead->pPrev = pNode;
            pHead = pNode;
        }
    }

    Pixel oPopFront(void)
    {
        Pixel oReturn(0, 0);

        if (pHead != 0)
        {
            oReturn.xPos = pHead->xPos;
            oReturn.yPos = pHead->yPos;
            Pixel *pTemp = pHead;

            pHead = pHead->pNext;
            if (pHead) {
                pHead->pPrev = 0;
            }
            else { //reached the end
                pTail = 0;
            }

            //cout << "freeing " << pTemp->xPos << "," << pTemp->yPos << endl;
            free(pTemp);
        }

        return oReturn;
    }

    Pixel oPopBack(void)
    {
        Pixel oReturn(0, 0);

        if (pTail != 0)
        {
            oReturn.xPos = pTail->xPos;
            oReturn.yPos = pTail->yPos;
            Pixel *pTemp = pTail;

            pTail = pTail->pPrev;
            if (pTail) { //not yet empty
                pTail->pNext = 0;
            }
            else { //empty
                pHead = 0;
            }
            //cout << "freeing " << pTemp->xPos << "," << pTemp->yPos << endl;
            free(pTemp);
        }

        return oReturn;
    }

    void vClearList(void)
    {
        while (pHead != 0)
        {
            Pixel *pTemp = pHead;
            pHead = pHead->pNext;

            //cout << "freeing " << pTemp->xPos << "," << pTemp->yPos << endl;
            free(pTemp);
        }
        pTail = 0;
    };
    //void vShowContents(void)
    //{
    //    Pixel *pIterator = pHead;
    //    while (pIterator != 0)
    //    {
    //        std::cout << pIterator->xPos << "," << pIterator->yPos << "->";
    //        pIterator = pIterator->pNext;
    //    }
    //    std::cout << "NULL" << std::endl;
    //}

    Pixel *pHead;
    Pixel *pTail;
};

#endif

