#ifndef __CA_VEC2_H_
#define __CA_VEC2_H_
#include "../platforms/CrossAny.h"
#include <algorithm>
#include <functional>
#include <math.h>

NS_CROSSANY_BEGIN

class Vec2{
public:
	float x, y;
public:
	Vec2();
	~Vec2();

	/**
	* Constructs a new vector initialized to the specified values.
	*
	* @param xx The x coordinate.
	* @param yy The y coordinate.
	*/
	Vec2(float xx, float yy);

	/**
	* Constructs a new vector from the values in the specified array.
	*
	* @param array An array containing the elements of the vector in the order x, y.
	*/
	Vec2(const float* array);

	/**
	* Constructs a vector that describes the direction between the specified points.
	*
	* @param p1 The first point.
	* @param p2 The second point.
	*/
	Vec2(const Vec2& p1, const Vec2& p2);

	/**
	* Constructs a new vector that is a copy of the specified vector.
	*
	* @param copy The vector to copy.
	*/
	Vec2(const Vec2& copy);

	inline const Vec2 operator-(const Vec2& v) const;
	inline const Vec2 operator-() const;

	/**
	* Sets the elements of this vector to the specified values.
	*
	* @param xx The new x coordinate.
	* @param yy The new y coordinate.
	*/
	void set(float xx, float yy);

	/**
	* Sets the elements of this vector from the values in the specified array.
	*
	* @param array An array containing the elements of the vector in the order x, y.
	*/
	void set(const float* array);

	/**
	* Sets the elements of this vector to those in the specified vector.
	*
	* @param v The vector to copy.
	*/
	void set(const Vec2& v);

	/**
	* Sets this vector to the directional vector between the specified points.
	*
	* @param p1 The first point.
	* @param p2 The second point.
	*/
	void set(const Vec2& p1, const Vec2& p2);

	/**
	* Negates this vector.
	*/
	void negate();
	/**
	* Subtracts this vector and the specified vector as (this - v)
	* and stores the result in this vector.
	*
	* @param v The vector to subtract.
	*/
	void subtract(const Vec2& v);
	bool equals(const Vec2& target) const;

	/** equals to Vec2(0,0) */
	static const Vec2 ZERO;
	/** equals to Vec2(1,1) */
	static const Vec2 ONE;
	/** equals to Vec2(1,0) */
	static const Vec2 UNIT_X;
	/** equals to Vec2(0,1) */
	static const Vec2 UNIT_Y;
	/** equals to Vec2(0.5, 0.5) */
	static const Vec2 ANCHOR_MIDDLE;
	/** equals to Vec2(0, 0) */
	static const Vec2 ANCHOR_BOTTOM_LEFT;
	/** equals to Vec2(0, 1) */
	static const Vec2 ANCHOR_TOP_LEFT;
	/** equals to Vec2(1, 0) */
	static const Vec2 ANCHOR_BOTTOM_RIGHT;
	/** equals to Vec2(1, 1) */
	static const Vec2 ANCHOR_TOP_RIGHT;
	/** equals to Vec2(1, 0.5) */
	static const Vec2 ANCHOR_MIDDLE_RIGHT;
	/** equals to Vec2(0, 0.5) */
	static const Vec2 ANCHOR_MIDDLE_LEFT;
	/** equals to Vec2(0.5, 1) */
	static const Vec2 ANCHOR_MIDDLE_TOP;
	/** equals to Vec2(0.5, 0) */
	static const Vec2 ANCHOR_MIDDLE_BOTTOM;

};


inline const Vec2 Vec2::operator-() const
{
	Vec2 result(*this);
	result.negate();
	return result;
}

inline const Vec2 Vec2::operator-(const Vec2& v) const
{
	Vec2 result(*this);
	result.subtract(v);
	return result;
}

NS_CROSSANY_END

#endif