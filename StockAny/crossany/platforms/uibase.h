#ifndef __CROSSANY_UIBASE_H__
#define __CROSSANY_UIBASE_H__
#include "CrossAny.h"

NS_CROSSANY_BEGIN

class point{
public:
	point(const double&_x, const double&_y) :x(_x), y(_y){};
	double x, y;
};

class size{
public:
	size(const double&_w, const double&_h) :w(_w), h(_h){};
	double w, h;
};

class rect{
public:
	rect(const double&x, const double&y, const double&width, const double&height) : orgin(x,y),wh(width,height){};
	point orgin; // 全部定义为左下角为原点。
	size wh; // width height; 对角线方向的角的点。
};

class uibase{
public:
	bool drawpt(const point&);
	bool drawpt(const double&x, const double&y){ return drawpt(point(x, y)); };
	bool drawln(const point& p0, const point& p1);
	bool drawln(const double&x0, const double&y0, const double&x1, const double&y1){ return drawln(point(x0, y0), point(x1, y1)); };
	bool drawrc(const rect&);
	bool drawrc(const double&x, const double&y, const double&width, const double&height){ return drawrc(rect(x, y, width, height)); };
	bool fillrc(const rect&);
	bool fillrc(const double&x, const double&y, const double&width, const double&height){ return drawrc(rect(x, y, width, height)); };

};

NS_CROSSANY_END
#endif