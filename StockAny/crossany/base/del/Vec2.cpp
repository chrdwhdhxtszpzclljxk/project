#include "Vec2.h"

NS_CROSSANY_BEGIN

Vec2::~Vec2(){}
Vec2::Vec2(): x(0.0f), y(0.0f){}
Vec2::Vec2(float xx, float yy): x(xx), y(yy){}
Vec2::Vec2(const float* array){	set(array);}
Vec2::Vec2(const Vec2& p1, const Vec2& p2){	set(p1, p2);}
Vec2::Vec2(const Vec2& copy){	set(copy);}
void Vec2::set(float xx, float yy){this->x = xx;this->y = yy;}
void Vec2::set(const float* array){	CA_ASSERT(array);x = array[0];y = array[1];}
void Vec2::set(const Vec2& v){this->x = v.x;this->y = v.y;}
void Vec2::set(const Vec2& p1, const Vec2& p2){	x = p2.x - p1.x;y = p2.y - p1.y;}
void Vec2::negate(){x = -x;	y = -y;}
void Vec2::subtract(const Vec2& v){	x -= v.x;	y -= v.y;}
bool Vec2::equals(const Vec2& target) const{
	return (fabs(this->x - target.x) < FLT_EPSILON)
		&& (fabs(this->y - target.y) < FLT_EPSILON);
}

const Vec2 Vec2::ZERO = Vec2(0.0f, 0.0f);
const Vec2 Vec2::ONE = Vec2(1.0f, 1.0f);
const Vec2 Vec2::UNIT_X = Vec2(1.0f, 0.0f);
const Vec2 Vec2::UNIT_Y = Vec2(0.0f, 1.0f);
const Vec2 Vec2::ANCHOR_MIDDLE = Vec2(0.5f, 0.5f);
const Vec2 Vec2::ANCHOR_BOTTOM_LEFT = Vec2(0.0f, 0.0f);
const Vec2 Vec2::ANCHOR_TOP_LEFT = Vec2(0.0f, 1.0f);
const Vec2 Vec2::ANCHOR_BOTTOM_RIGHT = Vec2(1.0f, 0.0f);
const Vec2 Vec2::ANCHOR_TOP_RIGHT = Vec2(1.0f, 1.0f);
const Vec2 Vec2::ANCHOR_MIDDLE_RIGHT = Vec2(1.0f, 0.5f);
const Vec2 Vec2::ANCHOR_MIDDLE_LEFT = Vec2(0.0f, 0.5f);
const Vec2 Vec2::ANCHOR_MIDDLE_TOP = Vec2(0.5f, 1.0f);
const Vec2 Vec2::ANCHOR_MIDDLE_BOTTOM = Vec2(0.5f, 0.0f);

NS_CROSSANY_END