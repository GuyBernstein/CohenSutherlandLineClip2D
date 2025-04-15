#ifndef COHEN_SUTHERLAND_H
#define COHEN_SUTHERLAND_H

#include <OpenGL/gl.h>
#include <GLUT/glut.h>

// Define the point class
class wcPt2D {
public:
    GLfloat x, y;
};

const GLint winLeftBitCode = 0x1;
const GLint winRightBitCode = 0x2;
const GLint winBottomBitCode = 0x4;
const GLint winTopBitCode = 0x8;

// Declare utility functions
inline GLint costume_round(const GLfloat a) { return GLint(a + 0.5); }
inline GLint inside(GLint code) { return GLint(!code); }
inline GLint reject(GLint code1, GLint code2) { return GLint(code1 & code2); }
inline GLint accept(GLint code1, GLint code2) { return GLint(!(code1 | code2)); }

// Declare the functions defined in the .cpp file
GLubyte encode(wcPt2D pt, wcPt2D winMin, wcPt2D winMax);
void swapPts(wcPt2D *p1, wcPt2D *p2);
void swapCodes(GLubyte *c1, GLubyte *c2);

#endif // COHEN_SUTHERLAND_H
