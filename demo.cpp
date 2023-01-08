#include "GL/freeglut.h"
//#include <GL/freeglut.h>
#include <chrono>

#include "csg.hpp"

#define ARCBALL_CAMERA_IMPLEMENTATION
#include "arcball_camera.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
/* DUMP MEMORY LEAKS */
#include <crtdbg.h>
#endif

using namespace csghpp;
CSG makeThing(float x, float y, int csg_op)
{
    printf("Creating shapes...");

    //auto a = CSG::cube(vec3(x, y, .5));
    //auto b = CSG::sphere(vec3(0,0, .0f), 1.0, 8, 6);
    //auto b = CSG::cube(vec3(-.25 , -.25 , -.25));
    //auto b = CSG::cylinder(0.3f, vec3(-2.f, 0, 0), vec3(2.f, 0, 0));
    //auto b = CSG::sphere(vec3(.25, .25, .25), 1.0, 8, 6);
    auto a = CSG::cylinder(0.3f, vec3(x, -1.f, y), vec3(0.f, 1.f, 0));
    auto b = CSG::cylinder(0.3f, vec3(-1.f, 0, 0), vec3(1.f, 0, 0));
    // 
    a.setColor(1, 1, 0);
    b.setColor(0, 0.5, 1);
    printf("Working on CSG...op%d ", csg_op);
    CSG c;
    if (csg_op == 0)
        c = a.unionOp(b);
    else if (csg_op == 1)
        c = a.subOp(b);
    else
        c = a.intersectOp(b);
    printf("completed CSG.\n");
    return c;
}

CSG makeThing2()
{
    auto a = CSG::cube(vec3(-.25, -.25, -.25));
    auto b = CSG::sphere(vec3(.25, .25, .25), 1.3);

    a.setColor(1, 1, 0);
    b.setColor(0, 0.5, 1);
    return a.subOp(b);
}

CSG makeThing3()
{
    auto a = CSG::cube(vec3(-.25, -.25, -.25));
    auto b = CSG::sphere(vec3(.25, .25, .25), 1.3);
    a.setColor(1, 1, 0);
    b.setColor(0, 0.5, 1);
    return a.intersectOp(b);
}

CSG makeThing4()
{
    // Generate example from wikipedia
    auto a = CSG::cube();
    auto b = CSG::sphere(vec3(0.f), 1.35, 12);
    auto c = CSG::cylinder(0.7f, vec3(-1.f, 0, 0), vec3(1.f, 0, 0));
    auto d = CSG::cylinder(0.7f, vec3(0, -1, 0), vec3(0, 1, 0));
    auto e = CSG::cylinder(0.7f, vec3(0, 0, -1), vec3(0, 0, 1));

    a.setColor(1, 0, 0);
    b.setColor(0, 0, 1);
    c.setColor(0, 1, 0);
    d.setColor(0, 1, 0);
    e.setColor(0, 1, 0);

    return a.intersectOp(b).subOp((c.unionOp(d).unionOp(e)));
}

CSG makeThing5()
{
    // Generate example from wikipedia
    auto a = CSG::cube(vec3(0.f), 20.f);
    auto b = CSG::cube(vec3(0.f), 19.f);
    auto c = CSG::cube(vec3(0.5f), 0.5f);
    auto d = CSG::cube(vec3(-0.5f), .5f);
    auto e = CSG::cube(vec3(-0.5f, -.5, -10.5), vec3(1.f, 10.f, 1.f));

    a.setColor(1, 0, 0);
    b.setColor(0, 0, 1);
    c.setColor(0, 1, 0);
    d.setColor(0, 1, 0);
    e.setColor(0, 1, 0);

    return a.subOp(b).unionOp(c).unionOp(d).unionOp(e);
}

/* report GL errors, if any, to stderr */
void checkError(const char* functionName)
{
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "GL error 0x%X detected in %s\n", error, functionName);
    }
}
/*
 * This macro is only intended to be used on arrays, of course.
 */
#define NUMBEROF(x) ((sizeof(x))/(sizeof(x[0])))

 /*
  * These global variables control which object is drawn,
  * and how it is drawn.  No object uses all of these
  * variables.
  */
static int function_index;
static double offset[3] = { 0, 0, 0 };
static GLboolean show_info = GL_TRUE;
static float ar;
static GLboolean persProject = GL_TRUE;

static int screen_width = 0;
static int screen_height = 0;
static int mouse_x = 0;
static int mouse_y = 0;
static int old_mousex = 0;
static int old_mousey = 0;
static bool mouse_left = false;
static bool mouse_middle = false;
static bool mouse_right = false;
static float g_old_mouse_wheel = 0.f;
static float g_mouse_wheel = 0.f;
static bool need_recalc = false;
static int csg_op = 0;

static CSG csg;
static Model model;
static void recalc_csg()
{
    float mouse_centerx = mouse_x - (.5f * screen_width);
    float mouse_centery = mouse_y - (.5f * screen_height);
    float nx = mouse_centerx / (.5f * screen_width);
    float ny = mouse_centery / (.5f * screen_height);

    using clock_type = std::chrono::high_resolution_clock;
    auto start = clock_type::now();
    if (function_index == 0)
    {
        csg = makeThing(2.f * 0, 2.f * 0, csg_op);
    }
    else if (function_index == 1)
    {
        csg = makeThing2();
    }
    else if (function_index == 2)
    {
        csg = makeThing3();
    }
    else if (function_index == 3)
    {
        csg = makeThing4();
    }
    else if (function_index == 4)
    {
        csg = makeThing5();
    }
    auto endCsg = clock_type::now();
    
    model = fromPolygons(csg.polygons);
    auto endPoly = clock_type::now();
    auto nsCsg = std::chrono::duration_cast<std::chrono::nanoseconds>(endCsg - start).count();
    auto nsPoly = std::chrono::duration_cast<std::chrono::nanoseconds>(endPoly - endCsg).count();
    printf("CSG:%.2f ms, poly:%.2f ms\n", (float)nsCsg * 1e-6f, (float)nsPoly * 1e-6f);
}

static void drawCsg(CSG o, bool wire)
{
    glFrontFace(GL_CCW);
    if (wire) glCullFace(GL_NONE); else glCullFace(GL_BACK);
    //for (CSG::Polygon& p : o.polygons)
    {
        /*
        glBegin(wire ? GL_LINE_LOOP : GL_POLYGON);
        vec3 n = p.plane.normal;
        glNormal3f(n.x, n.y, n.z);
        for (CSG::Vertex& v : p.vertices)
        {
            glVertex3f(v.pos.x, v.pos.y, v.pos.z);
        }
        glEnd();
        */
        
        for(int i=0; i < model.index.size(); i+=3)
        {
            unsigned i0 = model.index[i+0];
            unsigned i1 = model.index[i+1];
            unsigned i2 = model.index[i+2];
            vec3 n0 = model.vertices[i0].normal;
            vec3 n1 = model.vertices[i1].normal;
            vec3 n2 = model.vertices[i2].normal;
            vec3 v0 = model.vertices[i0].pos;
            vec3 v1 = model.vertices[i1].pos;
            vec3 v2 = model.vertices[i2].pos;
            vec3 c0 = model.vertices[i0].color;
            vec3 c1 = model.vertices[i1].color;
            vec3 c2 = model.vertices[i2].color;
            if (wire) glColor3f(1.f, 1.f, 1.f);
            glBegin(wire ? GL_LINE_LOOP : GL_TRIANGLES);
            glColor3f(c0.x, c0.y, c0.z);
            if (!wire)glNormal3f(n0.x, n0.y, n0.z);
            glVertex3f( v0.x, v0.y, v0.z);
            
            glColor3f(c1.x, c1.y, c1.z);
            if (!wire)glNormal3f(n1.x, n1.y, n1.z);
            glVertex3f( v1.x, v1.y, v1.z);

            glColor3f(c2.x, c2.y, c2.z);
            if (!wire)glNormal3f(n2.x, n2.y, n2.z);
            glVertex3f( v2.x, v2.y, v2.z);
            glEnd();
        }
        
    }

    glFrontFace(GL_CCW);
}

/*!
    Does printf()-like work using freeglut
    glutBitmapString().  Uses a fixed font.  Prints
    at the indicated row/column position.

    Limitation: Cannot address pixels.
    Limitation: Renders in screen coords, not model coords.
*/
static void shapesPrintf(int row, int col, const char* fmt, ...)
{
    static char buf[256];
    int viewport[4];
    void* font = GLUT_BITMAP_9_BY_15;
    va_list args;

    va_start(args, fmt);
#if defined(WIN32) && !defined(__CYGWIN__)
    (void)_vsnprintf_s(buf, sizeof(buf), fmt, args);
#else
    (void)vsnprintf(buf, sizeof(buf), fmt, args);
#endif
    va_end(args);

    glGetIntegerv(GL_VIEWPORT, viewport);

    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0, viewport[2], 0, viewport[3], -1, 1);

    glRasterPos2i
    (
        glutBitmapWidth(font, ' ') * col,
        -glutBitmapHeight(font) * row + viewport[3]
    );
    glutBitmapString(font, (unsigned char*)buf);

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

/* Print info about the about the current shape and render state on the screen */
static void DrawSizeInfo(int* row)
{
    //shapesPrintf((*row)++, 1, "Height  Up  Down : %f", orad);
}

static void drawInfo()
{
    int row = 1;
    shapesPrintf(row++, 1, "Shape PgUp PgDn %d", function_index);
    shapesPrintf(row++, 1, "Left mouse to pan");
    shapesPrintf(row++, 1, "Right mouse to rotate");
    shapesPrintf(row++, 1, "Scroll wheel to zoom");
    shapesPrintf(row++, 1, "c for prev op, v for next op");
    DrawSizeInfo(&row);
    if (persProject)
        shapesPrintf(row++, 1, "Perspective projection (p)");
    else
        shapesPrintf(row++, 1, "Orthographic projection (p)");
}

/* GLUT callback Handlers */
static void
resize(int width, int height)
{
    ar = (float)width / (float)height;

    glViewport(0, 0, width, height);
    screen_width = width;
    screen_height = height;
}

static void display(void)
{
    bool mouse_move = old_mousex != mouse_x || old_mousey != mouse_y;
    if (mouse_move && mouse_middle)
    {
        need_recalc = true;
    }

    if (need_recalc)
    {
        need_recalc = false;
        recalc_csg();
    }
    static double old_time = 0.0;
    const double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    double dt = t - old_time;
    old_time = t;

    float g_wheel_delta = g_old_mouse_wheel - g_mouse_wheel;
    g_old_mouse_wheel = g_mouse_wheel;

    static float pos[] = { 0,0,-5 };
    static float target[] = {0,0,0};
    static float up[] = { 0,1,0 };
    static bool inited = false;
    if (!inited)
    {
        inited = true;
        
        // initialize "up" to be tangent to the sphere!
            // up = cross(cross(look, world_up), look)
        {
            float look[3] = { target[0] - pos[0], target[1] - pos[1], target[2] - pos[2] };
            float look_len = sqrtf(look[0] * look[0] + look[1] * look[1] + look[2] * look[2]);
            look[0] /= look_len;
            look[1] /= look_len;
            look[2] /= look_len;

            float world_up[3] = { 0.0f, 1.0f, 0.0f };

            float across[3] = {
                look[1] * world_up[2] - look[2] * world_up[1],
                look[2] * world_up[0] - look[0] * world_up[2],
                look[0] * world_up[1] - look[1] * world_up[0],
            };

            up[0] = across[1] * look[2] - across[2] * look[1];
            up[1] = across[2] * look[0] - across[0] * look[2];
            up[2] = across[0] * look[1] - across[1] * look[0];

            float up_len = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
            up[0] /= up_len;
            up[1] /= up_len;
            up[2] /= up_len;
        }
    }

    static float view[16];
    arcball_camera_update(
        pos, target, up, view,
        dt,
        0.1f, // zoom per tick
        1.0f, // pan speed
        3.0f, // rotation multiplier
        screen_width, screen_height, // screen (window) size
        old_mousex, mouse_x,
        old_mousey, mouse_y,
        mouse_left, //GetAsyncKeyState(VK_MBUTTON),
        mouse_right, //GetAsyncKeyState(VK_RBUTTON),
        g_wheel_delta,//g_wheel_delta,
        0);
    old_mousex = mouse_x;
    old_mousey = mouse_y;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (persProject)
        glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);
    else
        glOrtho(-ar * 3, ar * 3, -3.0, 3.0, 2.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view);
    glEnable(GL_LIGHTING);

    glColor3d(1, 0, 0);

    glPushMatrix();
    drawCsg(csg, 0);
    glPopMatrix();

    glDisable(GL_LIGHTING);

    glPushMatrix();
    drawCsg(csg, 1);
    glPopMatrix();

    
    glColor3d(0.1, 0.1, 0.4);
    drawInfo();

    glutSwapBuffers();
}

static void
key(unsigned char key, int x, int y)
{
    int old_csg = csg_op;
    switch (key)
    {
    case 27:
    case 'Q':
    case 'q': glutLeaveMainLoop();      break;

    case 'c': if (csg_op > 0) csg_op--; break;
    case 'v': if (csg_op < 2) csg_op++; break;

    case 'I':
    case 'i': show_info = !show_info;       break;

    case '=':
    case '+': break;

    case '-':
    case '_': break;

    case ',':
    case '<': break;

    case '.':
    case '>': break;


    case 'P':
    case 'p': persProject = !persProject;   break;


    default:
        break;
    }
    if (old_csg != csg_op)
    {
        need_recalc = true;
    }
    //glutPostRedisplay();
}

static void special(int key, int x, int y)
{
    int old_function_index = function_index;
    switch (key)
    {
    case GLUT_KEY_PAGE_UP:    ++function_index; break;
    case GLUT_KEY_PAGE_DOWN:  --function_index; break;

    default:
        break;
    }
    if (function_index != old_function_index)
    {
        need_recalc = true;
    }

    if (0 > function_index)
        function_index = 5 - 1;

    if (5 <= (unsigned)function_index)
        function_index = 0;
}


static void
idle(void)
{
    glutPostRedisplay();
}

const GLfloat light_ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void mouseButton(int button, int state, int x, int y)
{
    mouse_x = x;
    mouse_y = y;
    // No delta
    old_mousex = mouse_x;
    old_mousey = mouse_y;
    if (state == GLUT_DOWN) {
        switch (button) {
        case GLUT_LEFT_BUTTON:
            mouse_left = true;
            break;
        case GLUT_RIGHT_BUTTON:
            mouse_right = true;
            break;
        case GLUT_MIDDLE_BUTTON:
            mouse_middle = true;
            break;
        default:
            break;
        }
    }
    else if (state == GLUT_UP)
    {
        switch (button) {
        case GLUT_LEFT_BUTTON:
            mouse_left = false;
            break;
        case GLUT_RIGHT_BUTTON:
            mouse_right = false;
            break;
        case GLUT_MIDDLE_BUTTON:
            mouse_middle = false;
            break;
        default:
            break;
        }
    }

    
}

void mouseMove(int x, int y) 
{
    mouse_x = x;
    mouse_y = y;
}

void mouseWheel(int button, int dir, int x, int y)
{
    if (dir > 0)
    {
        // Zoom in
        g_mouse_wheel+=5.0f;
    }
    else
    {
        // Zoom out
        g_mouse_wheel-=5.0f;
    }
}
/* Program entry point */

int
main(int argc, char* argv[])
{
    recalc_csg();

    glutInitWindowSize(800, 600);
    glutInitWindowPosition(40, 40);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);

    glutCreateWindow("CSG with BSP");

    glutReshapeFunc(resize);
    glutDisplayFunc(display);
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMove);
    glutMouseWheelFunc(mouseWheel);
    glutIdleFunc(idle);

    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    glClearColor(1, 1, 1, 1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);

    glutMainLoop();

#ifdef _MSC_VER
    /* DUMP MEMORY LEAK INFORMATION */
    _CrtDumpMemoryLeaks();
#endif

    return EXIT_SUCCESS;
}
