#include "acx.h"


/*
  THIS IS HIGHLY UNSUPPORTED CODE THAT CHANGES ALL THE TIME
  ben fry 99.12.19
*/


// TODO add code from meta app for pop-up key list helper
//      or maybe not, just make 'u' a usage key


////////////////////////////////////////////////////////////

// CURVES

static float bezierBasis[4][4] = {
  { -1,  3, -3,  1},
  {  3, -6,  3,  0},
  { -3,  3,  0,  0},
  {  1,  0,  0,  0},
};

static float catmullRomBasis[4][4] = {
    {-0.5f,  1.5f, -1.5f,  0.5f},
    { 1   , -2.5f,  2   , -0.5f},
    {-0.5f,  0   ,  0.5f,  0   },
    { 0   ,  1   ,  0   ,  0   }
};

static float sixth = 1.0f / 6.0f;
static float bezierToCatmullRom[4][4] = {
  {  0,     1,     0,      0     },
  { -sixth, 1,     sixth,  0     },
  {  0,     sixth, 1,     -sixth },
  {  0,     0,     1,      0     }
};
acMatrix4f bezierToCatmullRomMatrix(bezierToCatmullRom);

#define PRECISION 10
float fstep = 1.0 / (float)PRECISION;
float fstep2 = fstep*fstep;
float fstep3 = fstep2*fstep;
float forwardMatrix[4][4] = {
    {        0,        0,     0, 1 },
    {   fstep3,   fstep2, fstep, 0 },
    { 6*fstep3, 2*fstep2,     0, 0 },
    { 6*fstep3,        0,     0, 0 }
};

void multCatmullRom(float m[4][4], float g[4][2], float mg[4][2]) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      mg[i][j] = 0;
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 4; k++) {
	mg[i][j] = mg[i][j] + (m[i][k] * g[k][j]);
      }
    }
  }
}

void acxCatmullRom2fv(int pointCount, float *xpoints, float *ypoints) {
  float geom[4][2];
  float plot[4][2];
  float mg[4][2];
  
  for (int i = 0; i < pointCount-3;) {
    for (int j = 0; j <4 ;j++) {
      geom[j][0] = xpoints[i+j];
      geom[j][1] = ypoints[i+j];
    }
    multCatmullRom(catmullRomBasis, geom, mg);
    i += 1;
    
    multCatmullRom(forwardMatrix, mg, plot);
    float startX = plot[0][0];
    float x = startX;
    float startY = plot[0][1];
    float y = startY;
    
    glBegin(GL_LINE_STRIP);
    glVertex3f(startX, startY, 0);
    if (aiCapture) {
      aiBeginPath();
      aiMoveTo(startX, startY);
    }
    
    // plot the curve using the forward difference method
    for (int j = 0; j < PRECISION; j++) {
      x += plot[1][0];
      plot[1][0] += plot[2][0];
      plot[2][0] += plot[3][0];
      y += plot[1][1];
      plot[1][1] += plot[2][1];
      plot[2][1] += plot[3][1];
      //g.drawLine((int)startX, (int)startY, (int)x, (int)y);
      glVertex3f(x, y, 0);
      if (aiCapture) aiLineTo(x, y);
      startX = x;
      startY = y;
    }
    glEnd();
    if (aiCapture) aiEndPath();
  }
}


void acxCatmullRomBezier2fv(int pointCount, float *xpoints, float *ypoints) {
  //printf("in there\n");
  float control[4][3];  // 4 points, 3 for xyz

  for (int i = 0; i < pointCount-4; i++) {
    float convIn[4], convOut[4];

    // transform the x points
    for (int m = 0; m < 4; m++)
      convIn[m] = xpoints[i+m];
    //transform4x4(bezierToCatmullRom, convIn, convOut);
    bezierToCatmullRomMatrix.transform4(convIn, convOut);
    for (int m = 0; m < 4; m++)
      control[m][0] = convOut[m];

    // transform the y points
    for (int m = 0; m < 4; m++)
      convIn[m] = ypoints[i+m];
    //transform4x4(bezierToCatmullRom, convIn, convOut);
    bezierToCatmullRomMatrix.transform4(convIn, convOut);
    for (int m = 0; m < 4; m++)
      control[m][1] = convOut[m];

    // zero the z points
    for (int m = 0; m < 4; m++)
      control[m][2] = 0;

    if (aiCapture) {
      aiBeginPath();
      aiMoveTo(control[0][0], control[0][1]);
      aiCurveTo(control[1][0], control[1][1],
		control[2][0], control[2][1],
		control[3][0], control[3][1]);
      aiEndPath();
    }
    glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, &control[0][0]);
    glEnable(GL_MAP1_VERTEX_3);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 20; i++) {
      glEvalCoord1f((GLfloat) i/20.0);
    }
    glEnd();
  }
}


void acxBezier2fv(int pointCount, float *xpoints, float *ypoints) {
  // hey ben, write this later when you want to take the time
  // the code is inside innocents3 -> edge.cpp -> Edge::drawBezier
}


////////////////////////////////////////////////////////////

// FRAME RATE

// can't be used with multiple apps
static float acxFPS = 10.0f;
static int acxLastTime = 0;
float acxFrameRate() {
  int currentTime = acuCurrentTimeMillis();
  if (acxLastTime != 0) {
    float elapsed = currentTime - acxLastTime;
    acxFPS = (acxFPS * 0.9f) + ((1.0f / (elapsed / 1000.0f)) * 0.1f);
  }
  acxLastTime = currentTime;
  return acxFPS;
}

static int acxLastDelayTime = 0;
void acxFrameRateDelay(int targetFPS) {
  if (acxLastDelayTime == 0) {
    acxLastDelayTime = acuCurrentTimeMillis();
    return;
  }
  int timeToLeave = acxLastDelayTime + (1000 / targetFPS);
  while (TRUE) {
    int now = acuCurrentTimeMillis();
    if (now >= timeToLeave) {
      acxLastDelayTime = now;
      return;
    }
  }
}


////////////////////////////////////////////////////////////

// FILE I/O

int acxLoadFontOrDie(char *filename) {
  int which = acuLoadFont(filename);
  if (which == ACU_ERROR) {
    sprintf(acuDebugStr, "the font '%s' could not be found", filename);
    acuDebugString(ACU_DEBUG_EMERGENCY);
  }
  return which;
}


void acxNumberedFilename(char *dest, char *tmpl, int *num) {
  int counter = 0;
  int fd = -1;
  do {
    if (fd != -1) { 
      close(fd);
      counter++;
    }
    sprintf(dest, tmpl, counter);
    //printf("trying %s\n", dest);
  } while ((fd = open(dest, O_RDONLY)) != -1);
  sprintf(dest, tmpl, counter);
  *num = counter+1;
}


void acxReadToEOL(FILE *fp) {
  char c;
  do { 
    c = getc(fp); 
  } while (c != '\n');
  do { 
    c = getc(fp); 
  } while (c == '\n');
  if (!feof(fp)) 
    ungetc(c, fp);
}


void acxReadSpace(FILE *fp) {
  char c;
  do {
    c = getc( fp );
    //if (c=='#') { c = ' '; fread_to_eol(fp); }
    if (feof(fp)) return;
  } while (isspace(c));
  ungetc(c, fp);
}


void acxReadWord(FILE *fp, char *word) {
  acxReadSpace(fp);
  int test = fscanf(fp, "%s", word);
  if (test == EOF) word[0] = 0;
  else if (test != 1) {
    acuDebug(ACU_DEBUG_PROBLEM, "scanf returned something funny");
  }
}


float acxReadFloat(FILE *fp) {
  char temp[16];
  acxReadWord(fp, temp);
  return atof(temp);
}


////////////////////////////////////////////////////////////

// SPHERICAL / CARTESIAN COORDINATES

/*
void acxSphericalToCartesian(float sx, float sy, float sz,
			     float *cx, float *cy, float *cz) {
}

void acxSphericalToCartesian2(float *sx, float *sy, float *sz,
			     float *cx, float *cy, float *cz) {

  //printf("%f %f %f  ", *sx, *sy, *sz);

  float x0 = *sx * TWO_PI;
  float y0 = *sy * PI;
  float z0 = *sz * TWO_PI;
  
  *cx = z0 * cos(x0) * sin(y0);
  *cy = z0 * sin(x0) * sin(y0);
  *cz = z0 * cos(y0);

  //printf("%f %f %f\n", *cx, *cy, *cz);
}

void acxSphericalToCartesian3(float *s, float *c) {
  //printf("%f %f %f  ", s[0], s[1], s[2]);
  register float x0 = s[0] * TWO_PI;
  register float y0 = s[1] * PI;
  register float z0 = s[2] * TWO_PI;

  c[0] = z0 * cos(x0) * sin(y0);
  c[1] = z0 * sin(x0) * sin(y0);
  c[2] = z0 * cos(y0);
  //printf("%f %f %f\n", c[0], c[1], c[2]);
}

void acxCartesianToSpherical(float cx, float cy, float cz, 
			     float *x, float *y, float *z) {
  *z = sqrt(cx*cx + cy*cy + cz*cz) / TWO_PI;

  if (cx != 0) 
    *x = atan(cy / cx) / TWO_PI;
  else 
    *x = 0.0;

  if (cz != 0) 
    *y = atan(sqrt(cx*cx + cy*cy) / cz) / PI;
  else 
    *y = 0.0;

  //printf("%12.4f %12.4f %12.4f\n", *x, *y, *z);
}
*/

////////////////////////////////////////////////////////////

// TRACKBALL (someday)

//#define ROT_INCREMENT 3.0
//#define TRANS_INCREMENT 0.1

float acxLeftRightAngle = 0;
float acxUpDownAngle = 0; 
float acxFwdBackTrans = 0;
float acxLastMouseX, acxLastMouseY;

void acxTrackballMouseDown(float x, float y) {
  acxLastMouseX = x;
  acxLastMouseY = y;
}

void acxTrackballMouseDrag(float x, float y) {
  float dX = acxLastMouseX - x;
  float dY = acxLastMouseY - y;
  acxLeftRightAngle -= dX * 0.2;
  acxLastMouseX = x;
  acxLastMouseY = y;
}

void acxTrackballTransform() {
  glRotatef(acxLeftRightAngle, 0, 1, 0);
  glRotatef(acxUpDownAngle, 1, 0, 0);
}

void acxZoomMouseDown(float x, float y) {  
  acxLastMouseX = x;
  acxLastMouseY = y;
}

void acxZoomMouseDrag(float x, float y) {
  float dY = -(acxLastMouseY - y);
  acxFwdBackTrans += dY * 0.2;
  acxLastMouseX = x;
  acxLastMouseY = y;
}

void acxZoomTransform() {
  glTranslatef(0, 0, acxFwdBackTrans);
}

/*
// not sure why you'd want this
  void screenGrabAss() {
    char path[256];
    char command[256];
    sprintf(path, "/mas/u/fry/extra/sketchc-slides/sketchc%03d.rgb", 
	    grabAssCount++);
    sprintf(command, "scrsave %s %d %d %d %d", path,
	    0, windowWidth, 0, windowHeight);
    printf("%s\n", command);
    system(command);
  }
*/



////////////////////////////////////////////////////////////

// GEOMETRY


void acxQuad2f(float x1, float y1, float x2, float y2,
	       float x3, float y3, float x4, float y4) {
  glBegin(GL_QUADS);
  glVertex3f(x1, y1, 0);
  glVertex3f(x2, y2, 0);
  glVertex3f(x3, y3, 0);
  glVertex3f(x4, y4, 0);
  glEnd();
}


void acxOval2f(float x, float y, float w, float h) {
}


// uses radius instead of thickness
void acxGolanLine2f(float x0, float y0, float x1, float y1, 
		    float r0, float r1, int endcapCount) {
    float dX = x1-x0 + 0.0001f;
    float dY = y1-y0 + 0.0001f;
    float len = sqrt(dX*dX + dY*dY);

    float rh0 = r0 / len;
    float rh1 = r1 / len;
    
    float dx0 = rh0 * dY;
    float dy0 = rh0 * dX;
    float dx1 = rh1 * dY;
    float dy1 = rh1 * dX;

    acxQuad2f(x0+dx0, y0-dy0,
	      x0-dx0, y0+dy0,
	      x1-dx1, y1+dy1,
	      x1+dx1, y1-dy1);

    /*
	final float dX = x1-x0 + EPSILON;
	final float dY = y1-y0 + EPSILON;
	final float rH = radius / (float)Math.sqrt(dX*dX + dY*dY);
	final float dx = rH*dY;
	final float dy = rH*dX;
	
	int xpts[] = { (int)(x0+dx), (int)(x0-dx), 
		       (int)(x1-dx), (int)(x1+dx) };
	int ypts[] = { (int)(y0-dy), (int)(y0+dy), 
		       (int)(y1+dy), (int)(y1-dy) };
	
	g.fillPolygon(xpts, ypts, 4);
	if (endcapCount > 0) {
	    g.fillOval((int)(x0-radius), (int)(y0-radius),
		       thickness, thickness);
		       }
	if (endcapCount == 2) {
	g.fillOval((int)(x1-radius), (int)(y1-radius),
	    thickness, thickness);
	}
    */
}
