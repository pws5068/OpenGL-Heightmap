/* Programmer:	Ryan S Mullins
 * Class:		CMPSC 458
 * Section:		001
 * Date:		9/20/2010
 * Project:		HeightMap and SkyBox
 * Description:	
 */

#ifdef WIN32
#include <windows.h>			// For VC++ you need to include this file as glut.h and gl.h refer to it
#pragma warning(disable:4996)	// disable the warning for the use of strdup and friends
#endif
#include <stdio.h>				// Standard Header For Most Programs
#include <stdlib.h>				// Additional standard Functions (exit() for example)
#include <iostream>
#include <GL/gl.h>				// The GL Header File
#include <GL/glu.h>				// The GL Utilities (GLU) header
#include <GL/glut.h>			// The GL Utility Toolkit (Glut) Header
#include <pic.h>				// Interface to libpicio, provides functions to load/save jpeg files

using namespace std;

int g_iMenuId;
int currentFrame = 0;
const float world_width = 1.0;
const float world_yAspect = 1.0;
const float world_zAspect = 0.75;
float z_scaling = 0.05;

/* State of the mouse */
int mousePos[2] = {0, 0};
int leftMouseButtonState = 0;    /* 1 if pressed, 0 if not */
int middleMouseButtonState = 0;
int rightMouseButtonState = 0;

/* Current control mode the program is in (what will happen when the mouse is
* moved */
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
CONTROLSTATE currentControlState = ROTATE;

/* some global variable transformations to apply to the heightmap */
/* we will change these values with keyboard and mouse interaction, and the display function will use them to transform the heightmap */
GLfloat currentRotation[3] = {0.0, 0.0, 0.0};
GLfloat currentTranslation[3] = {0.0, 0.0, 0.0};
GLfloat currentScaling[3] = {1, 1, 1};

/* Heightmap source image (pic.h uses the Pic* data structure) */
Pic * sourceImage;

/******** Texture ids ********/
/* Each texture has to have its own id (you will need 6 for the skybox),
 * they are assigned using glGenTextures, which provides a unique integer identifying that texture which you can load using glLoadTexture
 * before drawing shapes with texturing 
 */
GLuint frontTextureId;
GLuint rearTextureId;
GLuint leftTextureId;
GLuint rightTextureId;
GLuint deckTextureId;
GLuint ceilingTextureId;
GLuint heightMapTextureId;

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

/*This function takes the name of a jpg file and a texture ID (by reference)*/
/*It creates a texture from the image specified and sets the ID specified to the value OpenGL generates for that texture*/
void loadTexture (char *filename, GLuint &textureID)
{
	Pic *pBitMap = pic_read(filename, NULL);

	if(pBitMap == NULL)
	{
		printf("Could not load the texture file\n");
		exit(1);
	}

	glEnable(GL_TEXTURE_2D); // Enable texturing
	glGenTextures(1, &textureID); // Obtain an id for the texture

	glBindTexture(GL_TEXTURE_2D, textureID); // Set as the current texture

	/* set some texture parameters */
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

	/* Load the texture into OpenGL as a mipmap. !!! This is a very important step !!! */
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, pBitMap->nx, pBitMap->ny, GL_RGB, GL_UNSIGNED_BYTE, pBitMap->pix);
	glDisable(GL_TEXTURE_2D);
	pic_free(pBitMap); // now that the texture has been copied by OpenGL we can free our copy
}

/* initGL will perform the one time initialization by
 * setting some state variables that are not going to
 * be changed
 */
void InitGL ( GLvoid )
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 1.333, 0, world_width + 1);
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);

	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background	

	//call our function to load a texture (you will have to do this for each texture you will load)
	loadTexture("texture/front.jpg",frontTextureId);
	loadTexture("texture/right.jpg",leftTextureId);
	loadTexture("texture/left.jpg",rightTextureId);
	loadTexture("texture/back.jpg",rearTextureId);
	loadTexture("texture/down.jpg",deckTextureId);
	loadTexture("texture/up.jpg",ceilingTextureId);
	/*if you plan to use Display lists this is a good place to create them */
}

/* Main GL display loop.
 * This function is called by GL whenever it wants to update the display.
 * All of the drawing you do should be done within this function, or functions
 * that this callsq
 */
void display ( void )   // Create The Display Function
{
	// World Dimension Variables
	float xMax = world_width;
	float yMax = world_width * world_yAspect;
	float zMax = world_width * world_zAspect;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();				// Load Identity Matrix to perform transformation on Camera
	gluLookAt(0, 0, 0, 0, yMax, 0, 0, 0, zMax);

	glRotatef(currentRotation[0], 1.0, 0.0, 0.0);										// Rotate Matrix about X-Axis
	glRotatef(currentRotation[1], 0.0, 1.0, 0.0);										// Rotate Matrix about Y-Axis
	glRotatef(currentRotation[2], 0.0, 0.0, 1.0);										// Rotate Matrix about Z-Axis

	/* Clear buffers */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/******** Draw SkyBox ********/
	glEnable(GL_TEXTURE_2D); // Enable texturing from now on
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

	// Front Wall
	glBindTexture(GL_TEXTURE_2D, frontTextureId); // select which texture to use 
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0); 	//set color

	glTexCoord2f(0.0 ,0.0);		// assign the upper left corner of the texture to the following vertex
	glVertex3f(-xMax, yMax, zMax);
	glTexCoord2f(0.0 ,1.0);
	glVertex3f(-xMax, yMax, -zMax);
	glTexCoord2f(1.0 ,1.0);
	glVertex3f(xMax, yMax, -zMax);
	glTexCoord2f(1.0 ,0.0);
	glVertex3f(xMax, yMax, zMax);

	glEnd();

	// Left Wall
	glBindTexture(GL_TEXTURE_2D, leftTextureId);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0); 	

	glTexCoord2f(0.0 ,0.0);		
	glVertex3f(-xMax, -yMax, zMax);
	glTexCoord2f(0.0 ,1.0);
	glVertex3f(-xMax, -yMax, -zMax);
	glTexCoord2f(1.0 ,1.0);
	glVertex3f(-xMax, yMax, -zMax);
	glTexCoord2f(1.0 ,0.0);
	glVertex3f(-xMax, yMax, zMax);

	glEnd();

	// Right Wall
	glBindTexture(GL_TEXTURE_2D, rightTextureId);  
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0); 	

	glTexCoord2f(0.0 ,0.0);		
	glVertex3f(xMax, yMax, zMax);
	glTexCoord2f(0.0 ,1.0);
	glVertex3f(xMax, yMax, -zMax);
	glTexCoord2f(1.0 ,1.0);
	glVertex3f(xMax, -yMax, -zMax);
	glTexCoord2f(1.0 ,0.0);
	glVertex3f(xMax, -yMax, zMax);

	glEnd();

	// Rear Wall
	glBindTexture(GL_TEXTURE_2D, rearTextureId);  
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0); 	

	glTexCoord2f(0.0 ,0.0);		
	glVertex3f(xMax, -yMax, zMax);
	glTexCoord2f(0.0 ,1.0);
	glVertex3f(xMax, -yMax, -zMax);
	glTexCoord2f(1.0 ,1.0);
	glVertex3f(-xMax, -yMax, -zMax);
	glTexCoord2f(1.0 ,0.0);
	glVertex3f(-xMax, -yMax, zMax);

	glEnd();

	// Deck
	glBindTexture(GL_TEXTURE_2D, deckTextureId);  
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0); 	

	glTexCoord2f(0.0 ,0.0);		
	glVertex3f(-xMax, yMax, -zMax);
	glTexCoord2f(0.0 ,1.0);
	glVertex3f(-xMax, -yMax, -zMax);
	glTexCoord2f(1.0 ,1.0);
	glVertex3f(xMax, -yMax, -zMax);
	glTexCoord2f(1.0 ,0.0);
	glVertex3f(xMax, yMax, -zMax);

	glEnd();

	// Ceiling
	glBindTexture(GL_TEXTURE_2D, ceilingTextureId);  
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0); 	

	glTexCoord2f(0.0 ,0.0);		
	glVertex3f(xMax, yMax, zMax);
	glTexCoord2f(0.0 ,1.0);
	glVertex3f(xMax, -yMax, zMax);
	glTexCoord2f(1.0 ,1.0);
	glVertex3f(-xMax, -yMax, zMax);
	glTexCoord2f(1.0 ,0.0);
	glVertex3f(-xMax, yMax, zMax);

	glEnd();

	glDisable(GL_TEXTURE_2D); //disable texturing so that the following objects do not use it
	/******** End Draw SkyBox ********/

	glTranslatef(currentTranslation[0], currentTranslation[1], currentTranslation[2]);	// Translate Matrix
	glScalef(currentScaling[0], currentScaling[1], currentScaling[2]);					// Scale Matrix

	/******** Draw HeightMap ********/
	glBindTexture(GL_TEXTURE_2D, heightMapTextureId);
	for(int i = 0; i < sourceImage->ny - 1; i++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(int j = 0; j < sourceImage->nx; j++)
		{
			// Define Color for the Vertex
			int red = PIC_PIXEL(sourceImage , j, i, 0);		// Find RGB value for each pixel
			int green = PIC_PIXEL(sourceImage , j, i, 1);
			int blue = PIC_PIXEL(sourceImage , j, i, 2);
			float color = (float)(red + green + blue) / (3.0 * 255.0);
			glColor3f(color, color, color);  

			// Define Position for the Vertex
			float xPos = (-1.0 + 2.0*(float)j/(float)sourceImage->nx) * xMax;
			float yPos = (-1.0 + 2.0*(float)i/(float)sourceImage->ny) * yMax;
			float height = (-zMax + ((float)(red + green + blue) / (3.0 * 255.0))) * z_scaling;
			glVertex3f(xPos, yPos, (-zMax + height));

			// Define Color for the next Vertex 
			int red1 = PIC_PIXEL(sourceImage , j, i+1, 0);	// Find RGB value for each pixel
			int green1 = PIC_PIXEL(sourceImage , j, i+1, 1);
			int blue1 = PIC_PIXEL(sourceImage , j, i+1, 2);
			float color1 = (float)(red1 + green1 + blue1) / (3.0 * 255.0);
			glColor3f(color1, color1, color1);

			// Define Position for the Vertex
			float yPos1 = (-1.0 + 2.0*(float)(i + 1)/(float)sourceImage->ny) * yMax;
			float height1 = (-zMax + ((float)(red1 + green1 + blue1) / (3.0 * 255.0))) * z_scaling;
			glVertex3f(xPos, yPos1, (-zMax + height1));
		}
		glEnd();
	}
	/******** End Draw HeightMap ********/

	glPopMatrix();
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);

	/* Swap buffers, so one we just drew is displayed */
	glutSwapBuffers();
}

/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
	int i;
	Pic *in = NULL;

	if (filename == NULL)
		return;

	/* Allocate a picture buffer */
	in = pic_alloc(WINDOW_WIDTH, WINDOW_HEIGHT, 3, NULL);

	printf("File to save to: %s\n", filename);

	/* Loop over each row of the image and copy into the image */
	for (i=WINDOW_HEIGHT-1; i>=0; i--) {
		glReadPixels(0, WINDOW_HEIGHT-1-i, WINDOW_WIDTH, 1, GL_RGB,
			GL_UNSIGNED_BYTE, &in->pix[i*in->nx*in->bpp]);
	}

	/* Output the file */
	if (jpeg_write(filename, in)) {
		printf("File saved Successfully\n");
	} else {
		printf("Error in Saving\n");
	}

	/* Free memory used by image */
	pic_free(in);
}

/* converts mouse drags into information about rotation/translation/scaling
 * This is run by GL whenever the mouse is moved and a mouse button is being
 * held down.
 */
void mousedrag(int x, int y)
{
	int mousePosChange[2] = {x-mousePos[0], y-mousePos[1]};

	/* Check which state we are in. */
	switch (currentControlState)
	{
	case TRANSLATE:
		if (leftMouseButtonState)
		{
			currentTranslation[0] += mousePosChange[0]*0.01;
			currentTranslation[1] -= mousePosChange[1]*0.01;
		}
		if (middleMouseButtonState)
		{
			currentTranslation[2] += mousePosChange[1]*0.01;
		}
		break;
	case ROTATE:
		if (leftMouseButtonState)
		{
			currentRotation[0] += mousePosChange[1];
			currentRotation[1] += mousePosChange[0];
		}
		if (middleMouseButtonState)
		{
			currentRotation[2] += mousePosChange[1];
		}
		break;
	case SCALE:
		if (leftMouseButtonState)
		{
			currentScaling[0] *= 1.0+mousePosChange[0]*0.01;
			currentScaling[1] *= 1.0-mousePosChange[1]*0.01;
		}
		if (middleMouseButtonState)
		{
			currentScaling[2] *= 1.0-mousePosChange[1]*0.01;
		}
		break;
	}

	/* Update stored mouse position */
	mousePos[0] = x;
	mousePos[1] = y;
}

/* This function is called by GL when the mouse moves passively.
 * "Passively" means that no mouse button is being held down 
 */
void mouseidle(int x, int y)
{
	/* Update mouse position */
	mousePos[0] = x;
	mousePos[1] = y;
}

/* This is called by GL whenever a mouse button is pressed. */
void mousebutton(int button, int state, int x, int y)
{

	/* Check which button was pressed and update stored mouse state */
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		leftMouseButtonState = (state==GLUT_DOWN);
		break;
	case GLUT_MIDDLE_BUTTON:
		middleMouseButtonState = (state==GLUT_DOWN);
		break;
	case GLUT_RIGHT_BUTTON:
		rightMouseButtonState = (state==GLUT_DOWN);
		break;
	}

	/* Check what modifier keys (shift, ctrl, etc) are pressed and update the
	* control mode based off of which keys are pressed */
	switch(glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		currentControlState = TRANSLATE;
		break;
	case GLUT_ACTIVE_SHIFT:
		currentControlState = SCALE;
		break;
	default:
		currentControlState = ROTATE;
		break;
	}

	/* Update stored mouse position */
	mousePos[0] = x;
	mousePos[1] = y;
}

/* This function will be called by GL whenever a keyboard key is pressed.
 * It recieves the ASCII value of the key that was pressed, along with the x
 * and y coordinates of the mouse at the time the key was pressed.
 */
void keyboard (unsigned char key, int x, int y) {

	/* User pressed quit key */
	if(key == 'q' || key == 'Q' || key == 27) {
		exit(0);
	}
}

/* Function that GL runs once a menu selection has been made.
 * This receives the number of the menu item that was selected
 */
void menufunc(int value)
{
	switch (value)
	{
	case 0:
		exit(0);
		break;
	}
}

/* This function is called by GL whenever it is idle, usually after calling
 * display.
 */
void doIdle()
{
	glutPostRedisplay();
	char * fName = new char[99];
	itoa(currentFrame, fName, 10);
	fName = strcat(fName, ".jpg");
	saveScreenshot(fName);
	free(fName);
	currentFrame++;
}

// The ubiquituous main function.
int main ( int argc, char** argv )   // Create Main Function For Bringing It All Together
{
	if (argc<2)// if not specified, prompt for filename
	{
		char inputFile[999];
		printf("Input height file:");
		cin>>inputFile;
		sourceImage = jpeg_read(inputFile, NULL);
		loadTexture(inputFile, heightMapTextureId);
	}
	else //otherwise, use the name provided
	{
		/* Open jpeg (reads into memory) */
		sourceImage = jpeg_read(argv[1], NULL);
		loadTexture(argv[1], heightMapTextureId);
	}

	printf("Set Z-Scaling Value <%f>: ", z_scaling);
	cin >> z_scaling;

	//this is an example of reading image data, you will use the pixel values to determine the height of the map at each node
	int red = PIC_PIXEL(sourceImage , 0, 0, 0);
	int green = PIC_PIXEL(sourceImage , 0, 0, 1);
	int blue = PIC_PIXEL(sourceImage , 0, 0, 2);
	cout<<"\nThe (r,g,b) value at the upper left corner is ("<<red<<","<<green<<","<<blue<<")\n";


	/*** The following are commands for setting up GL      ***/
	/*** No OpenGl call should be before this sequence!!!! ***/

	/* Initialize glut */
	glutInit(&argc,argv);

	/* Set up window modes */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	/* Set window position (where it will appear when it opens) */
	glutInitWindowPosition(50,50);

	/* Set size of window */
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	/* Create the window */
	glutCreateWindow    ( "CMPSC 458: Heightmap" ); // Window Title (argv[0] for current directory as title)

	/**** Call to our initialization routine****/
	InitGL ();

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);

	/** allow the user to quit using the right mouse button menu **/
	/* Set menu function callback */
	g_iMenuId = glutCreateMenu(menufunc);

	/* Set identifier for menu */
	glutSetMenu(g_iMenuId);

	/* Add quit option to menu */
	glutAddMenuEntry("Quit",0);

	/* Add any other menu functions you may want here.  The format is:
	* glutAddMenuEntry(MenuEntry, EntryIndex)
	* The index is used in the menufunc to determine what option was selected
	*/

	/* Attach menu to right button clicks */
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/* Set idle function.  You can change this to call code for your animation,
	* or place your animation code in doIdle */
	glutIdleFunc(doIdle);

	/* callback for keyboard input */
	glutKeyboardFunc(keyboard);

	/* callback for mouse drags */
	glutMotionFunc(mousedrag);

	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);

	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);

	glutMainLoop        ( );          // Initialize The Main Loop

	return 0;
}

