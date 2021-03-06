/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2014 Chukong Technologies

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#ifndef __MATH_CCGEOMETRY_H__
#define __MATH_CCGEOMETRY_H__

#include <math.h>
#include "../platforms/CrossAny.h"
#include "Vec2.h"
//#include "platform/CCPlatformMacros.h"
//#include "base/ccMacros.h"
//#include "math/CCMath.h"

NS_CROSSANY_BEGIN

/**
 * @addtogroup data_structures
 * @{
 */

class Size{
public:
    float width, height;
public:
    operator Vec2() const{ return Vec2(width, height); }
public:
    /**
     * @js NA
     */
    Size();
    /**
     * @js NA
     */
    Size(float width, float height);
    /**
     * @js NA
     * @lua NA
     */
    Size(const Size& other);
    /**
     * @js NA
     * @lua NA
     */
    explicit Size(const Vec2& point);
    /**
     * @js NA
     * @lua NA
     */
    Size& operator= (const Size& other);
    /**
     * @js NA
     * @lua NA
     */
    Size& operator= (const Vec2& point);
    /**
     * @js NA
     * @lua NA
     */
    Size operator+(const Size& right) const;
    /**
     * @js NA
     * @lua NA
     */
    Size operator-(const Size& right) const;
    /**
     * @js NA
     * @lua NA
     */
    Size operator*(float a) const;
    /**
     * @js NA
     * @lua NA
     */
    Size operator/(float a) const;
    /**
     * @js NA
     * @lua NA
     */
    void setSize(float _width, float _height);
    /**
     * @js NA
     */
    bool equals(const Size& target) const;
    
    static const Size ZERO;
};

class Rect{
public:
    Vec2 origin;
    Size  size;
public:
    /**
     * @js NA
     */
    Rect();
    /**
     * @js NA
     */
    Rect(float x, float y, float width, float height);
    /**
     * @js NA
     * @lua NA
     */
    Rect(const Rect& other);
    /**
     * @js NA
     * @lua NA
     */
    Rect& operator= (const Rect& other);
    /**
     * @js NA
     * @lua NA
     */
    void setRect(float x, float y, float width, float height);
    /**
     * @js NA
     */
    float getMinX() const; /// return the leftmost x-value of current rect
    /**
     * @js NA
     */
    float getMidX() const; /// return the midpoint x-value of current rect
    /**
     * @js NA
     */
    float getMaxX() const; /// return the rightmost x-value of current rect
    /**
     * @js NA
     */
    float getMinY() const; /// return the bottommost y-value of current rect
    /**
     * @js NA
     */
    float getMidY() const; /// return the midpoint y-value of current rect
    /**
     * @js NA
     */
    float getMaxY() const; /// return the topmost y-value of current rect
    /**
     * @js NA
     */
    bool equals(const Rect& rect) const;
    /**
     * @js NA
     */
    bool containsPoint(const Vec2& point) const;
    /**
     * @js NA
     */
    bool intersectsRect(const Rect& rect) const;
    /**
     * @js NA
     * @lua NA
     */
    Rect unionWithRect(const Rect & rect) const;

    void merge(const Rect& rect);
    
    static const Rect ZERO;
};

// end of data_structure group
/// @}

NS_CROSSANY_END

#endif // __MATH_CCGEOMETRY_H__
