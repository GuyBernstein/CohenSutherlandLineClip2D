#include <string>
#include <cmath>
#include <cstdio>
#include "ch8CohenSutherlandLineClip2D.h"

// Constants for animation
const int ANIM_DELAY = 2000;  // Animation delay in milliseconds
const int TEXT_BASE_Y = 200; // Base Y position for text

// Global variables
GLsizei winWidth = 1000, winHeight = 800;
GLfloat xwcMin = 0.0, xwcMax = 225.0;
GLfloat ywcMin = 0.0, ywcMax = 225.0;
bool showColoredLines = false;  // Flag to control visibility of colored line segments
bool needToEraseLines = false;  // Flag to indicate when to erase previous colored lines
wcPt2D eraseLine_p1, eraseLine_p2;  // Points for the line segment to erase
wcPt2D eraseLine_p3, eraseLine_p4;  // Points for the second line segment to erase
bool showLines = true;  // Flag to control visibility of all lines during transitions

// Flag to track when animation is complete and final line should be shown
bool showFinalLine = false;

// Clipping rectangle
wcPt2D winMin = {50.0, 50.0};  // Bottom-left corner
wcPt2D winMax = {150.0, 150.0}; // Top-right corner

// Line endpoints
wcPt2D p1 = {20.0, 120.0};
wcPt2D p2 = {180.0, 30.0};

// Animation state
enum AnimationState { IDLE = 0, RUNNING = 1 };
int animState = IDLE;
int animStep = 0;
std::string statusMsg; // Status message to display
bool swapInProgress = false;

// Animation variables
wcPt2D orig_p1, orig_p2;     // Original line endpoints
wcPt2D curr_p1, curr_p2;     // Current line endpoints during animation
wcPt2D prev_p1;              // Previous p1 (for drawing interim lines)
GLubyte code1, code2;        // Region codes
bool done = false;           // Clipping algorithm termination flag
bool plotLine = false;       // Should the final line be plotted
GLfloat m;                   // Line slope
bool swapped = false;        // Whether points were swapped

// Edge tracking for colored visualization
enum ClipEdge { NONE = 0, LEFT, RIGHT, BOTTOM, TOP };
ClipEdge currentEdge = NONE;
ClipEdge prevEdge = NONE;
wcPt2D prev_p2;              // Previous p2 (for drawing interim lines)
ClipEdge eraseEdge = NONE;  // Edge type of the line to erase

// Track original line for erasing
bool originalLineErased = false;

// Function prototypes
void displayFcn(void);
void animateClippingStep();
void timerFunc(int value);
void showBlackLineTimer(int value);

// Bresenham's line drawing algorithm
void lineBres(int x0, int y0, int x1, int y1) {
    glColor3f(0.0, 0.0, 0.0);
    glPointSize(2.0);

    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy, e2;

    glBegin(GL_POINTS);
        while (true) {
            glVertex2i(x0, y0);
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 < dx) { err += dx; y0 += sy; }
        }
    glEnd();

    glPointSize(1.0);
}

// Display region code for a point
void displayRegionCode(wcPt2D pt, GLubyte code, bool isP1) {
    glColor3f(0.0, 0.0, 0.0);
    char codeStr[9];
    snprintf(codeStr, sizeof(codeStr), "%d%d%d%d",
        (code & winTopBitCode) ? 1 : 0,
        (code & winBottomBitCode) ? 1 : 0,
        (code & winRightBitCode) ? 1 : 0,
        (code & winLeftBitCode) ? 1 : 0);

    glRasterPos2f(pt.x + 5, pt.y + 5);
    for (int i = 0; codeStr[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, codeStr[i]);
    }

    char pointStr[10];
    snprintf(pointStr, sizeof(pointStr), "P%d", isP1 ? 1 : 2);
    glRasterPos2f(pt.x - 15, pt.y - 5);
    for (int i = 0; pointStr[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, pointStr[i]);
    }
}

// Draw a line using OpenGL
void drawLine(wcPt2D p1, wcPt2D p2, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_LINES);
        glVertex2f(p1.x, p1.y);
        glVertex2f(p2.x, p2.y);
    glEnd();
}

// Draw a point using OpenGL
void drawPoint(wcPt2D p, float r, float g, float b) {
    glColor3f(r, g, b);
    glPointSize(6.0);
    glBegin(GL_POINTS);
        glVertex2f(p.x, p.y);
    glEnd();
    glPointSize(1.0);
}

// Get color for current edge
void setColorForEdge(ClipEdge edge) {
    switch (edge) {
        case LEFT:
            glColor3f(1.0, 0.0, 0.0); // Red for LEFT edge
            break;
        case RIGHT:
            glColor3f(0.0, 0.7, 0.0); // Green for RIGHT edge
            break;
        case BOTTOM:
            glColor3f(0.0, 0.0, 1.0); // Blue for BOTTOM edge
            break;
        case TOP:
            glColor3f(1.0, 0.5, 0.0); // Orange for TOP edge
            break;
        default:
            glColor3f(0.5, 0.5, 0.5); // Gray for undefined
    }
}

// Draw a clipping line with edge-specific color
void drawClippingLine(wcPt2D p1, wcPt2D p2, ClipEdge edge) {
    setColorForEdge(edge);
    glLineWidth(2.0); // Make clipping lines thicker
    glBegin(GL_LINES);
        glVertex2f(p1.x, p1.y);
        glVertex2f(p2.x, p2.y);
    glEnd();
    glLineWidth(1.0); // Reset line width
}

// Draw the clipping rectangle with dotted lines
void drawClippingWindow() {
    glColor3f(0.0, 0.0, 1.0);  // Blue color

    // Enable line stippling (dotted lines)
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x00FF);  // Pattern: 0000000011111111 (dotted line)

    glBegin(GL_LINE_LOOP);
    glVertex2f(winMin.x, winMin.y);
    glVertex2f(winMax.x, winMin.y);
    glVertex2f(winMax.x, winMax.y);
    glVertex2f(winMin.x, winMax.y);
    glEnd();

    // Disable line stippling after drawing
    glDisable(GL_LINE_STIPPLE);
}

// Draw text at a specific position
void drawText(const char* text, float x, float y) {
    glColor3f(0.0, 0.0, 0.0);
    glRasterPos2f(x, y);
    for (int i = 0; text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, text[i]);
    }
}

// Return edge name as string
const char* getEdgeName(ClipEdge edge) {
    switch (edge) {
        case LEFT: return "LEFT";
        case RIGHT: return "RIGHT";
        case BOTTOM: return "BOTTOM";
        case TOP: return "TOP";
        default: return "NONE";
    }
}

// Timer callback for animation
void timerFunc(int value) {
    if (animState == RUNNING && !done) {
        animateClippingStep();
        glutPostRedisplay();

        // Schedule the next step if not done
        if (!done) {
            glutTimerFunc(ANIM_DELAY, timerFunc, 0);
        } else {
            // Animation is complete, show the final line if accepted
            showFinalLine = plotLine;
            glutPostRedisplay();
        }
    }
}

// Timer function that will hide the colored lines after a delay
void coloredLinesTimer(int value) {
    // First, hide all lines by setting both flags to false
    showColoredLines = false;  // Hide colored lines
    showLines = false;         // Hide all lines temporarily
    glutPostRedisplay();       // Request a redraw to show a clean white screen

    // Schedule another timer to show the black line after a brief delay
    glutTimerFunc(50, showBlackLineTimer, 0);
}

void showBlackLineTimer(int value) {
    showLines = true;          // Show lines again (will be the black line)
    glutPostRedisplay();       // Request another redraw
}

// Initialize animation
void startAnimation() {
    animState = RUNNING;
    animStep = 0;
    done = false;
    plotLine = false;
    orig_p1 = p1;
    orig_p2 = p2;
    curr_p1 = p1;
    curr_p2 = p2;
    prev_p1 = p1;      // Initialize prev_p1
    prev_p2 = p2;      // Initialize prev_p2
    swapped = false;
    swapInProgress = false;
    currentEdge = NONE;
    prevEdge = NONE;
    showColoredLines = false;  // Initialize colored lines visibility to false
    needToEraseLines = false;  // Initialize erase flag to false
    showFinalLine = false;     // Don't show final line until animation completes
    originalLineErased = false; // Track if original line has been erased - CHANGED: keep original line

    // Initialize erase line points to invalid values
    eraseLine_p1 = eraseLine_p2 = eraseLine_p3 = eraseLine_p4 = {-1.0, -1.0};
    eraseEdge = NONE;

    // Calculate initial codes using the function from the header
    code1 = encode(curr_p1, winMin, winMax);
    code2 = encode(curr_p2, winMin, winMax);

    statusMsg = "Animation started...";

    // REMOVED: Don't erase the original line
    // glutTimerFunc(ANIM_DELAY / 4, eraseOriginalLineTimer, 0);

    // Start the animation timer
    glutTimerFunc(ANIM_DELAY, timerFunc, 0);
}

// Function to draw white lines to erase previous colored segments
void drawEraseLine(wcPt2D p1, wcPt2D p2) {
    // Draw a white line to "erase" a previously drawn colored line
    glColor3f(1.0, 1.0, 1.0);  // White color for erasing
    glLineWidth(4.0);          // Make it wider than the original lines to ensure complete erasure
    glBegin(GL_LINES);
        glVertex2f(p1.x, p1.y);
        glVertex2f(p2.x, p2.y);
    glEnd();
    glLineWidth(1.0);          // Reset line width
}

// Perform one step of the Cohen-Sutherland algorithm
void animateClippingStep() {
    if (done) return;

    // First, save the current points for potential erasing in the next step
    prev_p1 = curr_p1;  // Save the previous points for drawing
    prev_p2 = curr_p2;  // Track p2's previous position
    prevEdge = currentEdge; // Save the previous edge

    // Step 1: Check for trivial accept/reject using functions from header
    if (animStep == 0) {
        // Clear the swapInProgress flag in step 0
        swapInProgress = false;

        if (accept(code1, code2)) {
            done = true;
            plotLine = true;
            showColoredLines = false;  // Ensure colored lines are not shown

            // Ensure any final colored lines are cleared when drawing the final result
            drawEraseLine(eraseLine_p1, eraseLine_p2);
            drawEraseLine(eraseLine_p3, eraseLine_p4);

            statusMsg = "Line ACCEPTED - Both endpoints inside window or clipped properly";
            return;
        }
        else if (reject(code1, code2)) {
            done = true;
            showColoredLines = false;  // Ensure colored lines are not shown
            needToEraseLines = true;   // Ensure any remaining colored lines are erased

            // Ensure any final colored lines are cleared when terminating with rejection
            drawEraseLine(eraseLine_p1, eraseLine_p2);
            drawEraseLine(eraseLine_p3, eraseLine_p4);

            statusMsg = "Line REJECTED - Line completely outside window";
            return;
        }

        statusMsg = "Checking trivial accept/reject: Neither accepted nor rejected, continuing...";
        animStep++;
        return;
    }

    // Step 2: Check if we need to swap points so p1 is outside
    if (animStep == 1) {
        // Set swapInProgress to true when we're in this step
        swapInProgress = true;

        // Use inside function from header
        if (inside(code1)) {
            // Use swap functions from header
            swapPts(&curr_p1, &curr_p2);
            swapCodes(&code1, &code2);
            swapped = true;
            statusMsg = "Swapped endpoints - Ensuring P1 is outside the window";
        } else {
            statusMsg = "P1 is already outside window, no need to swap";
        }
        animStep++;
        return;
    }

    // Step 3+: Clip against window edges
    // Clear the swapInProgress flag for clipping steps
    swapInProgress = false;

    // Calculate line slope
    if (curr_p2.x != curr_p1.x)
        m = (curr_p2.y - curr_p1.y) / (curr_p2.x - curr_p1.x);
    else
        m = 1000000.0;  // Large value for nearly vertical lines

    // Reset current edge
    currentEdge = NONE;

    // Clip against the appropriate edge
    if (code1 & winLeftBitCode) {
        curr_p1.y += (winMin.x - curr_p1.x) * m;
        curr_p1.x = winMin.x;
        currentEdge = LEFT;
        statusMsg = "Clipped against LEFT edge of window";
    }
    else if (code1 & winRightBitCode) {
        curr_p1.y += (winMax.x - curr_p1.x) * m;
        curr_p1.x = winMax.x;
        currentEdge = RIGHT;
        statusMsg = "Clipped against RIGHT edge of window";
    }
    else if (code1 & winBottomBitCode) {
        if (curr_p2.x != curr_p1.x) // Avoid division by zero/undefined slope
             if (m != 0) // Avoid division by zero if line is horizontal
                curr_p1.x += (winMin.y - curr_p1.y) / m;
        curr_p1.y = winMin.y;
        currentEdge = BOTTOM;
        statusMsg = "Clipped against BOTTOM edge of window";
    }
    else if (code1 & winTopBitCode) {
         if (curr_p2.x != curr_p1.x) // Avoid division by zero/undefined slope
             if (m != 0) // Avoid division by zero if line is horizontal
                curr_p1.x += (winMax.y - curr_p1.y) / m;
        curr_p1.y = winMax.y;
        currentEdge = TOP;
        statusMsg = "Clipped against TOP edge of window";
    }

    // If we clipped against an edge, show colored lines briefly
    if (currentEdge != NONE) {
        showColoredLines = true;  // Show colored lines for this step

        // Schedule the timer to hide the colored lines after half the animation delay
        glutTimerFunc(ANIM_DELAY, coloredLinesTimer, 1);
    }

    // Recalculate code for new position using function from header
    code1 = encode(curr_p1, winMin, winMax);

    // Reset to step 0 to check for trivial conditions again
    animStep = 0;
}

// Display function
void displayFcn(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw clipping window
    drawClippingWindow();

    // Draw instructions
    drawText("Click and drag to move endpoints. Left button = P1, Right button = P2", 10, TEXT_BASE_Y);
    drawText("Press SPACE to start/reset animation", 10, TEXT_BASE_Y - 20);

    // Draw the original line
    if (animState == IDLE) {
        // Original line
        drawLine(p1, p2, 0.0, 0.0, 0.0);  // Black

        // Endpoints
        drawPoint(p1, 1.0, 0.0, 0.0);  // Red for P1
        drawPoint(p2, 0.0, 0.7, 0.0);  // Green for P2

        // Show region codes
        GLubyte c1 = encode(p1, winMin, winMax); // Use encode from header
        GLubyte c2 = encode(p2, winMin, winMax); // Use encode from header
        displayRegionCode(p1, c1, true);
        displayRegionCode(p2, c2, false);

        // Display initial status
        drawText("Set line endpoints and press SPACE to start animation", 10, 10);
    }
    else {



            // Draw color key for edge clipping
            drawText("Edge Color Key:", 10, TEXT_BASE_Y - 10);

            // Set color for LEFT edge and draw key
            setColorForEdge(LEFT);
            glBegin(GL_LINES);
            glVertex2f(100, TEXT_BASE_Y - 10);
            glVertex2f(120, TEXT_BASE_Y - 10);
            glEnd();
            drawText("LEFT", 125, TEXT_BASE_Y - 10);

            // Set color for RIGHT edge and draw key
            setColorForEdge(RIGHT);
            glBegin(GL_LINES);
            glVertex2f(135, TEXT_BASE_Y - 10);
            glVertex2f(155, TEXT_BASE_Y - 10);
            glEnd();
            drawText("RIGHT", 160, TEXT_BASE_Y - 10);

            // Set color for BOTTOM edge and draw key
            setColorForEdge(BOTTOM);
            glBegin(GL_LINES);
            glVertex2f(175, TEXT_BASE_Y - 10);
            glVertex2f(195, TEXT_BASE_Y - 10);
            glEnd();
            drawText("BOTTOM", 200, TEXT_BASE_Y - 10);

            // Set color for TOP edge and draw key
            setColorForEdge(TOP);
            glBegin(GL_LINES);
            glVertex2f(65, TEXT_BASE_Y - 10);
            glVertex2f(85, TEXT_BASE_Y - 10);
            glEnd();
            drawText("TOP", 90, TEXT_BASE_Y - 10);

            if (swapped) {
                drawText("*Points were swapped during algorithm*", 10, TEXT_BASE_Y - 40);
            }

            // First, erase previous colored lines if needed
            if (needToEraseLines && eraseEdge != NONE) {
                // Erase the previous colored lines by drawing white lines over them
                drawEraseLine(eraseLine_p1, eraseLine_p2);
                drawEraseLine(eraseLine_p3, eraseLine_p4);

                // Reset the erase flag after erasing
                needToEraseLines = false;
            }


        // If colored lines are being shown, display the colored clipping parts
        if (currentEdge != NONE && !swapInProgress && showColoredLines) {
            // First draw the complete current black line
            drawLine(curr_p1, curr_p2, 0.0, 0.0, 0.0);

            // Then overlay the colored segments to show the clipping visualization
            drawClippingLine(prev_p1, curr_p1, currentEdge);

            // Only draw second colored line if p2 actually moved during this clip
            if (prev_p2.x != curr_p2.x || prev_p2.y != curr_p2.y) {
                drawClippingLine(prev_p2, curr_p2, currentEdge);

                // Store both line segments for erasing later
                eraseLine_p3 = prev_p2;
                eraseLine_p4 = curr_p2;
            } else {
                // If p2 didn't move, set erase coordinates to invalid values
                eraseLine_p3 = eraseLine_p4 = {-1.0, -1.0};
            }

            // Store the first line segment for erasing later
            eraseLine_p1 = prev_p1;
            eraseLine_p2 = curr_p1;
            eraseEdge = currentEdge;
        } else {
            // Normal case: draw the current black line
            drawLine(curr_p1, curr_p2, 0.0, 0.0, 0.0);
        }

        // Draw current line if accepted and animation is complete
        if (done && plotLine && showFinalLine) {
            // Draw the final accepted line in black with Bresenham's algorithm
            lineBres(costume_round(curr_p1.x), costume_round(curr_p1.y),
                     costume_round(curr_p2.x), costume_round(curr_p2.y));
        }

        // Draw current endpoints
        drawPoint(curr_p1, 1.0, 0.0, 0.0);  // Red for P1
        drawPoint(curr_p2, 0.0, 0.7, 0.0);  // Green for P2

        // Show region codes
        displayRegionCode(curr_p1, code1, true);
        displayRegionCode(curr_p2, code2, false);

        // Show status information
        char stepInfo[200];
        if (currentEdge != NONE) {
            snprintf(stepInfo, sizeof(stepInfo), "Step: %d - %s",
                     animStep, statusMsg.c_str());
        } else {
            snprintf(stepInfo, sizeof(stepInfo), "Step: %d - %s", animStep, statusMsg.c_str());
        }
        drawText(stepInfo, 10, 10);

        if (done) {
            if (plotLine) {
                drawText("Line ACCEPTED - Press SPACE to reset", 10, 30);
            } else {
                drawText("Line REJECTED - Press SPACE to reset", 10, 30);
            }
        }
    }

    glutSwapBuffers(); // Changed to double buffering for smoother animation
}

// Mouse callback
void mouseFcn(int button, int state, int x, int y) {
     // Only allow mouse interaction before animation or after it's done
    if (animState == RUNNING && !done) return;

    if (state == GLUT_DOWN) {
        // Convert screen coordinates to world coordinates
        wcPt2D clickPt{};
        clickPt.x = static_cast<GLfloat>(x) * (xwcMax - xwcMin) / winWidth + xwcMin;
        clickPt.y = static_cast<GLfloat>(winHeight - y) * (ywcMax - ywcMin) / winHeight + ywcMin;

        // Reset animation if it was running and finished
        if (animState == RUNNING && done) {
             animState = IDLE;
             showFinalLine = false; // Hide final line when resetting
        }

        if (button == GLUT_LEFT_BUTTON) {
            p1 = clickPt;
        }
        else if (button == GLUT_RIGHT_BUTTON) {
            p2 = clickPt;
        }

        // If we reset, ensure the display updates to the non-animated state
        if (animState == IDLE) {
            statusMsg = "Set line endpoints and press SPACE to start animation";
        }

        glutPostRedisplay();
    }
}

// Motion callback for dragging points
void motionFcn(int x, int y) {
    // Only allow mouse interaction before animation or after it's done
    if (animState == RUNNING && !done) return;

    // Convert screen coordinates to world coordinates
    wcPt2D movePt;
    movePt.x = static_cast<GLfloat>(x) * (xwcMax - xwcMin) / winWidth + xwcMin;
    movePt.y = static_cast<GLfloat>(winHeight - y) * (ywcMax - ywcMin) / winHeight + ywcMin;

    // Determine which point is closer to the mouse position
    float d1 = std::abs(movePt.x - p1.x) + std::abs(movePt.y - p1.y);
    float d2 = std::abs(movePt.x - p2.x) + std::abs(movePt.y - p2.y);

    if (d1 < d2) {
        p1 = movePt;  // Closer to P1
    } else {
        p2 = movePt;  // Closer to P2
    }

    glutPostRedisplay();
}

// Keyboard callback
void keyboardFcn(unsigned char key, int x, int y) {
    switch (key) {
        case ' ':  // Start/reset animation
            if (animState == IDLE) {
                startAnimation();
            } else { // If animation was running or finished, reset
                animState = IDLE;
                showFinalLine = false; // Hide final line when resetting
                statusMsg = "Set line endpoints and press SPACE to start animation";
            }
            glutPostRedisplay();
            break;
        case 27: // Escape key
            exit(0);
            break;
    }
}

void winReshapeFcn(GLint newWidth, GLint newHeight) {
    glViewport(0, 0, newWidth, newHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(xwcMin, xwcMax, ywcMin, ywcMax);

    // Update global width/height for coordinate conversion
    winWidth = newWidth;
    winHeight = newHeight;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Init function
void init(void) {
    // Set color of display window to white
    glClearColor(1.0, 1.0, 1.0, 0.0);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); // Changed to double buffering
    glutInitWindowPosition(50, 50);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Cohen-Sutherland Line Clipping Animation");

    init(); // Call initialization function

    // Register callback functions
    glutDisplayFunc(displayFcn);
    glutReshapeFunc(winReshapeFcn);
    glutMouseFunc(mouseFcn);
    glutMotionFunc(motionFcn);
    glutKeyboardFunc(keyboardFcn);

    glutMainLoop();
    return 0;
}