#include "../CCGL.h"
#include "../uibase.h"

NS_CROSSANY_BEGIN

bool uibase::drawpt(const point& pt){
	glBegin(GL_POINTS);
	glVertex2d(pt.x, pt.y);
	glEnd();
	return true;
}
bool uibase::drawln(const point& p0, const point& p1){
	glBegin(GL_LINE_STRIP);
	glVertex2d(p0.x, p0.y); glVertex2d(p1.y, p1.y);
	glEnd();
	return true;
}
bool uibase::drawrc(const rect& rc){
	glBegin(GL_LINE_LOOP);
	glVertex2d(rc.orgin.x, rc.orgin.y); glVertex2d(rc.orgin.x + rc.wh.w, rc.orgin.y);
	glVertex2d(rc.orgin.x + rc.wh.w, rc.orgin.y + rc.wh.h); glVertex2d(rc.orgin.x , rc.orgin.y + rc.wh.h);
	glEnd();
	return true;
}
bool uibase::fillrc(const rect& rc){
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);
	glBegin(GL_POLYGON);
	glVertex2d(rc.orgin.x, rc.orgin.y); glVertex2d(rc.orgin.x + rc.wh.w, rc.orgin.y);
	glVertex2d(rc.orgin.x + rc.wh.w, rc.orgin.y + rc.wh.h); glVertex2d(rc.orgin.x, rc.orgin.y + rc.wh.h);
	glEnd();
	return true;
}
NS_CROSSANY_END