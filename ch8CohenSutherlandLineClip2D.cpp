#include "ch8CohenSutherlandLineClip2D.h"

/*
 * Definitions for the functions declared in cohen_sutherland.h
 */
GLubyte encode (wcPt2D pt, wcPt2D winMin, wcPt2D winMax)
{
  GLubyte code = 0x00;

  if (pt.x < winMin.x)
    code = code | winLeftBitCode;
  if (pt.x > winMax.x)
    code = code | winRightBitCode;
  if (pt.y < winMin.y)
    code = code | winBottomBitCode;
  if (pt.y > winMax.y)
    code = code | winTopBitCode;
  return (code);
}

void swapPts (wcPt2D * p1, wcPt2D * p2)
{
  wcPt2D tmp;

  tmp = *p1; *p1 = *p2; *p2 = tmp;
}

void swapCodes (GLubyte * c1, GLubyte * c2)
{
  GLubyte tmp;

  tmp = *c1; *c1 = *c2; *c2 = tmp;
}
