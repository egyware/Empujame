#define _USE_MATH_DEFINES
#include <cmath>
#include <Common.h>
#include <DebugDraw.h>

#ifdef DEBUG


#include <SDL/SDL_gfxPrimitives.h>

#define RED_COLOR   0xFF0000FF
#define GREEN_COLOR 0x00FF00FF
#define BLUE_COLOR  0x0000FFFF
#define ALFA_MASK 0xFFFFFF7F
SDL_Surface *img;
inline Uint32 toColor(const b2Color &color) {
    return SDL_MapRGB(img->format,
                      static_cast<Uint8>(color.r*0xFF),
                      static_cast<Uint8>(color.g*0xFF),
                      static_cast<Uint8>(color.b*0xFF))<<8|0xFF;

}

DebugDraw::DebugDraw() {
}

DebugDraw::~DebugDraw() {
}

void DebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
    Sint16 vx[b2_maxPolygonVertices];
    Sint16 vy[b2_maxPolygonVertices];
    for(int i=0; i<vertexCount; i++)
        vx[i] = static_cast<Sint16>(INV_SCALE*vertices[i].x);
    for(int i=0; i<vertexCount; i++)
        vy[i] = static_cast<Sint16>(INV_SCALE*vertices[i].y);
    polygonColor(img,vx,vy,vertexCount,toColor(color));
}


void DebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
	Uint32 c = toColor(color);
	 Sint16 vx[b2_maxPolygonVertices];
	 Sint16 vy[b2_maxPolygonVertices];
    for(int i=0; i<vertexCount; i++)
        vx[i] = static_cast<Sint16>(INV_SCALE*vertices[i].x);
    for(int i=0; i<vertexCount; i++)
        vy[i] = static_cast<Sint16>(INV_SCALE*vertices[i].y);
    filledPolygonColor(img,vx,vy,vertexCount,c&ALFA_MASK);
    polygonColor(img,vx,vy,vertexCount,c);
}

void DebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) {
    circleColor(img,
                static_cast<Sint16>(INV_SCALE*center.x),
                static_cast<Sint16>(INV_SCALE*center.y),
                static_cast<Sint16>(INV_SCALE*radius),toColor(color));
}

void DebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) {
    Uint32 c = toColor(color);
    filledCircleColor(img,static_cast<Sint16>(INV_SCALE*center.x),static_cast<Sint16>(INV_SCALE*center.y),
                      static_cast<Sint16>(INV_SCALE*radius),c&ALFA_MASK);
    circleColor(img,static_cast<Sint16>(INV_SCALE*center.x),static_cast<Sint16>(INV_SCALE*center.y),
                static_cast<Sint16>(INV_SCALE*radius),c);

    lineColor(img,static_cast<Sint16>(INV_SCALE*center.x),static_cast<Sint16>(INV_SCALE*center.y),
              static_cast<Sint16>(INV_SCALE*(axis.x*radius+center.x)),static_cast<Sint16>(INV_SCALE*(axis.y*radius+center.y)),c);
}

void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {
    lineColor(img,static_cast<Sint16>(INV_SCALE*p1.x),static_cast<Sint16>(INV_SCALE*p1.y),
              static_cast<Sint16>(INV_SCALE*p2.x),static_cast<Sint16>(INV_SCALE*p2.y),
              toColor(color));
}

void DebugDraw::DrawTransform(const b2Transform& xf) {
    float32 angle = xf.q.GetAngle();
	Sint16 x = static_cast<Sint16>(xf.p.x*INV_SCALE);
	Sint16 y = static_cast<Sint16>(xf.p.y*INV_SCALE);
    Sint16 redx = static_cast<Sint16>(x+10*cos(angle+M_PI_2));
    Sint16 redy = static_cast<Sint16>(y+10*sin(angle+M_PI_2));
    Sint16 greenx = static_cast<Sint16>(x+10*cos(angle));
    Sint16 greeny = static_cast<Sint16>(y+10*sin(angle));
    lineColor(img,x,y,redx,redy,RED_COLOR);
    lineColor(img,x,y,greenx,greeny,GREEN_COLOR);
}

void DebugDraw::paint(SDL_Surface *salida) {
    img = salida;
}

#endif

