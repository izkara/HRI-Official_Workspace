/*****************************************************************************

Copyright (c) 2004 SensAble Technologies, Inc. All rights reserved.

OpenHaptics(TM) toolkit. The material embodied in this software and use of
this software is subject to the terms and conditions of the clickthrough
Development License Agreement.

For questions, comments or bug reports, go to forums at: 
    http://dsc.sensable.com

Module Name: 

  main.cpp

Description:

  The main file that performs all haptics-relevant operation. Within a 
  asynchronous callback the graphics thread reads the position and sets
  the force. Within a synchronous callback the graphics thread gets the
  position and constructs graphics elements (e.g. force vector).

*******************************************************************************/

#include <stdlib.h>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <Windows.h>

#include <HD/hd.h>

#include "helper.h"

#include <HDU/hduError.h>
#include <HDU/hduVector.h>

// Sample code to read in test cases:
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
using namespace std;

bool buttonPushed = false;
char controlChar = ' ';
static double sphereRadius = 3.0;
static double mySphereRadius = 3;
double heightThreshold = 2;
static hduVector3Dd pos_prev(0.01, 0.01, 0.01);
static hduVector3Dd pos(0.01, 0.01, 0.01);
static double vel_target = 0.01; //0.02;
double vel = 0;
int count = 0;
static int period = 10;
hduVector3Dd forceVec;
const int n_history = 10;
hduVector3Dd force_h[10];
int force_index = 0;
static double controlK = 1400.0;
double offset = 20;

/* Charge (positive/negative) */
int charge = 1;

static HHD ghHD = HD_INVALID_HANDLE;
static HDSchedulerHandle gSchedulerCallback = HD_INVALID_HANDLE;

/* Glut callback functions used by helper.cpp */
void displayFunction(void);
void handleIdle(void);



hduVector3Dd forceField(hduVector3Dd pos, hduVector3Dd** shape, int* shape_sizes);


hduVector3Dd** Shape;
int* Shape_sizes; 
int Shape_count;

/* Haptic device record. */
struct DeviceDisplayState
{
    HHD m_hHD;
    hduVector3Dd position;
    hduVector3Dd force;
};

/*******************************************************************************
 Client callback used by the graphics main loop function.
 Use this callback synchronously.
 Gets data, in a thread safe manner, that is constantly being modified by the 
 haptics thread. 
*******************************************************************************/
HDCallbackCode HDCALLBACK DeviceStateCallback(void *pUserData)
{
    DeviceDisplayState *pDisplayState = 
        static_cast<DeviceDisplayState *>(pUserData);

    hdGetDoublev(HD_CURRENT_POSITION, pDisplayState->position);
    hdGetDoublev(HD_CURRENT_FORCE, pDisplayState->force);

    // execute this only once.
    return HD_CALLBACK_DONE;
}


/*******************************************************************************
 Graphics main loop function.
*******************************************************************************/
void displayFunction(void)
{
	// Setup model transformations.
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	setupGraphicsState();
	drawAxes(sphereRadius*2.0);

	// Draw the fixed sphere.
   // static const hduVector3Dd fixedSpherePosition(0, 0, 0);
	static const float fixedSphereColor[4] = { .2, .8, .8, .8 };
	GLUquadricObj* pQuadObj = gluNewQuadric();
	// drawSphere(pQuadObj, fixedSpherePosition, fixedSphereColor, sphereRadius);

	 // Get the current position of end effector.
	DeviceDisplayState state;
	hdScheduleSynchronous(DeviceStateCallback, &state,
		HD_MIN_SCHEDULER_PRIORITY);

	// Draw a sphere to represent the haptic cursor and the dynamic 
	// charge.
	static const float dynamicSphereColor[4] = { .8, .2, .2, .8 };
	drawSphere(pQuadObj,
		state.position,
		dynamicSphereColor,
		sphereRadius);

	for (int j = 0; j < Shape_count; j++) {
		for (int i = 0; i < Shape_sizes[j]; i++)
		{
			//glBegin(GL_POINTS); //starts drawing of points

			//glVertex3f(Shape[0][0], Shape[0][1], Shape[0][2]);//upper-right corner

			GLUquadricObj* pQuadObj1 = gluNewQuadric();
			drawSphere(pQuadObj1, Shape[j][i], fixedSphereColor, mySphereRadius);


			//glEnd();//end drawing of points
		}
	}

	// Create the force vector.
	hduVector3Dd forceVector = 40.0 * forceField(state.position, Shape, Shape_sizes);
	//if (forceVector.magnitude() > 40.0) forceVector = normalize(forceVector) * 40.0;


	drawForceVector(pQuadObj,
		state.position,
		forceVector,
		sphereRadius*.1);
	
    gluDeleteQuadric(pQuadObj);
  
    glPopMatrix();
    glutSwapBuffers();                      
}
                                
/*******************************************************************************
 Called periodically by the GLUT framework.
*******************************************************************************/
void handleIdle(void)
{
    glutPostRedisplay();

    if (!hdWaitForCompletion(gSchedulerCallback, HD_WAIT_CHECK_STATUS))
    {
        printf("The main scheduler callback has exited\n");
        printf("Press any key to quit.\n");
        getchar();
        exit(-1);
    }
}

/******************************************************************************
 Popup menu handler
******************************************************************************/
void handleMenu(int ID)
{
    switch(ID) 
    {
        case 0:
            exit(0);
            break;
        case 1:
            charge *= -1;
            break;
    }
}

int timeFrame = 0;

/*******************************************************************************
 Given the position is space, calculates the (modified) coulomb force.
*******************************************************************************/
hduVector3Dd forceField(hduVector3Dd pos, hduVector3Dd** shape, int* shape_sizes)
{
	double scaleInside = 1.0;
	double scaleFar = 6.0; // 12.0;
	double scaleClose = 3.0; //3 //scaleFar/4/sphereRadius/sphereRadius;
	double springK = 1.0;

	if (GetAsyncKeyState(0x46)) {
		if (!buttonPushed) {
			buttonPushed = true;
			cout << "ControlK: " << controlK << "\n";
			controlChar = 'F';
		}
	}
	else if (GetAsyncKeyState(0x48)) {
		if (!buttonPushed) {
			buttonPushed = true;
			cout << "Height is : " << heightThreshold << "\n";
			controlChar = 'H';
		}
	}
	else if (GetAsyncKeyState(0x53)) {
		if (!buttonPushed) {
			buttonPushed = true;
			cout << "Sphere size is: " << sphereRadius << "\n";
			controlChar = 'S';
		}
	}
	else if (GetAsyncKeyState(VK_UP)) {
		if (!buttonPushed) {
			buttonPushed = true;
			switch (controlChar) {
			case('F') :
				controlK += 10.0;
				cout << "controlK increased: " << controlK << "\n";
				break;
			case('H') :
				heightThreshold += .01;
				break;
			case('S') :
				sphereRadius += .5;
				break;
			}
		}
	}
	else if (GetAsyncKeyState(VK_DOWN)) {
		if (!buttonPushed) {
			buttonPushed = true;
			switch (controlChar) {
			case('F') :
				controlK -= 10.0;
				cout << "controlK increased: " << controlK << "\n";
				break;
			case('H') :
				heightThreshold -= .01;
				break;
			case('S') :
				sphereRadius -= .5;
				break;
			}
		}
	}
	else {
		buttonPushed = false;
	}



	hduVector3Dd forceVec(0, 0, 0);
	timeFrame++;

	pos[1] = -offset;

	//only for single cruve
	for (int j = 0; j < Shape_count; j++) {
		for (int i = 0; i < shape_sizes[j]; i++) {
			hduVector3Dd diff = pos - shape[j][i];
			double dist = diff.magnitude();
			double pointDist = (shape[j][i] - shape[j][i + 1]).magnitude();
			double heightDist = abs(shape[j][i][1] - pos[1]);
			//cout << "HeightDist: " << heightDist << "\n";
				// if two charges overlap...
			if (dist < sphereRadius*2.0 && heightDist < heightThreshold)// && i < shape_size && pointDist < 1.0)
			{
				if (i == 0 || i == Shape_sizes[j]) return -springK*diff;
				// Attract the charge to the center of the sphere.
				hduVector3Dd unitPos = normalize(diff);
				//cout << "||| x = " << unitPos[0] << " y = " << unitPos[1] << " z = " << unitPos[2] << "\n";
				//if (timeFrame > 10) {

				//	cout << "SCALE*UNITPOS: " << (-scaleClose*unitPos).magnitude() << '\n';
				//	//cout << "||| x = " << -scale*unitPos[0] << " y = " << -scale*unitPos[1] << " z = " << -scale*unitPos[2] << "\n";
				//	timeFrame = 0;
				//}
				//cout << vel - vel_target << endl;
				//return -scaleClose*unitPos;
				//unitPos[1] = 0;

				/*if (i > shape_sizes[j]) vel_target = abs(vel_target);
				else {
					if (vel_target > 0) vel_target *= -1;
				}*/
				double control_scale = controlK*(vel - vel_target);
				return control_scale*unitPos;
			}

			else
			{
				hduVector3Dd unitPos = normalize(diff);
				forceVec += -scaleFar*unitPos / (dist*dist);
				//cout << "FORCEVEC: " << forceVec.magnitude() << '\n';
			}
		}
	}

		forceVec *= charge;
		//cout << "forceVecX: " << forceVec[0] << "\nforceVecY: " << forceVec[1] << "\nforceVecZ: " << forceVec[2] << "\n";
		return forceVec;
	
}

/*******************************************************************************
 Main callback that calculates and sets the force.
*******************************************************************************/
HDCallbackCode HDCALLBACK CoulombCallback(void *data)
{
    HHD hHD = hdGetCurrentDevice();

    hdBeginFrame(hHD);
	count++;
	if (count == period) {
		pos_prev = pos; count = 0;
		vel = sqrt((pos[0] - pos_prev[0])*(pos[0] - pos_prev[0]) + (pos[2] - pos_prev[2])*(pos[2] - pos_prev[2]));
		
	}
	forceVec = forceField(pos, Shape, Shape_sizes);
	force_h[force_index] = forceVec;
	hduVector3Dd forceSmoothed(0,0,0);
	for (int i = 0; i < n_history; i++) {
		forceSmoothed += force_h[i];
	}
	forceSmoothed /= (n_history + 0.0);
	
	hdSetDoublev(HD_CURRENT_FORCE, forceSmoothed);
	hdGetDoublev(HD_CURRENT_POSITION, pos);
	
    
	//hduVector3Dd forceVec;
	//forceVec = forceField(pos, Shape, Shape_sizes);
    //hdSetDoublev(HD_CURRENT_FORCE, forceVec);
        
    hdEndFrame(hHD);

    HDErrorInfo error;
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "Error during scheduler callback");
        if (hduIsSchedulerError(&error))
        {
            return HD_CALLBACK_DONE;
        }
    }

    return HD_CALLBACK_CONTINUE;
}

/*******************************************************************************
 Schedules the coulomb force callback.
*******************************************************************************/
void CoulombForceField()
{
    gSchedulerCallback = hdScheduleAsynchronous(
        CoulombCallback, 0, HD_DEFAULT_SCHEDULER_PRIORITY);

    HDErrorInfo error;
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "Failed to initialize haptic device");
        fprintf(stderr, "\nPress any key to quit.\n");
        getchar();
        exit(-1);
    }


    glutMainLoop(); // Enter GLUT main loop.
}

/******************************************************************************
 This handler gets called when the process is exiting. Ensures that HDAPI is
 properly shutdown
******************************************************************************/
void exitHandler()
{
    hdStopScheduler();
    hdUnschedule(gSchedulerCallback);

    if (ghHD != HD_INVALID_HANDLE)
    {
        hdDisableDevice(ghHD);
        ghHD = HD_INVALID_HANDLE;
    }
}









void parse(string fileName)
{
		ifstream stream1(fileName);		
		string line;
		int shapeSize = 0;
		double coords[3];
		hduVector3Dd *tempShape;

		while (getline(stream1, line)) {
			if (line.length() > 0 && line.substr(0, 1) == "n") {
				Shape_count++;
			}
		}
		Shape = new hduVector3Dd*[Shape_count];
		Shape_sizes = new int[Shape_count];



		int currentShape = 0;
		ifstream stream2(fileName);

		//cout << "Shapecount: " << Shape_count << "\n";
		while (getline(stream2, line)) {
			//cout << "first char is : " << line.substr(0, 1) << "\n";
			if (line.length() > 0 && line.substr(0, 2) == "v ") {
				shapeSize++;
			}
			else if (line.length() > 0 && line.substr(0, 1) == "e") {
				//cout << "currentshape: " << currentShape << "\n";
				Shape_sizes[currentShape] = shapeSize;
				Shape[currentShape] = new hduVector3Dd[shapeSize];
				shapeSize = 0;
				//cout << "moving on to shape: " << currentShape << "\n";
				//cout << currentShape << "'s shape is size: " << Shape_sizes[currentShape] << "\n";

				currentShape++;
			}
		}

		/*cout << "ShapeCount0: " << Shape_sizes[0] << "\n";
		cout << "ShapeCount1: " << Shape_sizes[1] << "\n";
		cout << "ShapeCount2: " << Shape_sizes[2] << "\n";*/


		currentShape = 0;
		ifstream stream(fileName);
		int j = 0;
		while (getline(stream, line)) {
			if (line.length() > 0 && line.substr(0, 2) == "v ") {
				size_t i = 2;
				for (int coord = 0; coord < 3; coord++) {
					//for each coordinate find corresponding substring n and convert to float
					string number;

					int count = 0;
					while (i < line.length() - 1 && line[i] != ' ') {
						i++;
						count++;
					}
					i++;
					count++;

					number = line.substr(i - count, count);
					coords[coord] = stod(number) * 15;
					//cout << "coords[coord]: " << coords[coord] << "\n";
				}
				hduVector3Dd vc(coords[0], coords[2] - offset, coords[1]);//coords[1], coords[2]);//

				if (currentShape >= Shape_count)
				{
					cout << "CURRENT SHAPE IS BIGGER THAN SHAPE COUNT!\n";
					continue;
				}
				/*cout << "currentShape: " << currentShape << "\n";
				cout << "j: " << j << "\n";
*/
				Shape[currentShape][j] = vc;

				j++;
			}
			//cout << " shapeSize: " << Shape_sizes[currentShape] << "\n";
			else if (line.substr(0, 1) == "e" || j == Shape_sizes[currentShape]) {
				//cout << line << "\n";
				cout << "NEW SHAPE!\n";
				currentShape++;
				
				j = 0;
			}
		}
		return;
}

void print(hduVector3Dd** Shape) {
	cout << "ShapeCount: " << Shape_count << "\n";
	for (int j = 0; j < Shape_count; j++) {
		cout << "shape size at " << j << "th shape: " << Shape_sizes[j] << "\n";
		for (int i = 0; i < Shape_sizes[j]; i++)
		{
			cout << "x = " << Shape[j][i][0] << " y = " << Shape[j][i][1] << " z = " << Shape[j][i][2] << "\n";
		}
	}
}






/******************************************************************************
 Main function.
******************************************************************************/
int main(int argc, char* argv[])
{
	hduVector3Dd force_zero(0, 0, 0);
	for(int i = 0; i < n_history; i++) {
		force_h[i] = force_zero;
	}
	
	HDErrorInfo error;

    printf("Starting application\n");
    
    atexit(exitHandler);
	cout << "Drag OBJ here:" << endl;
	string file_name ;//= "C:/OpenHaptics/Developer/3.4.0/examples/HD/graphics/HRI-Official_Workspace\drawingObj/drawing_23_28_39.obj";
	cin >> file_name;
	string file_path = file_name;//"models/" + file_name;

	glPointSize(10.0f);
	cout << "entering parse\n";
	parse(file_path);
	cout << "exiting parse\n";
	/*cout << "entering print\n";
	print(Shape);
	cout << "exiting print\n";*/



    // Initialize the device.  This needs to be called before any other
    // actions on the device are performed.
    ghHD = hdInitDevice(HD_DEFAULT_DEVICE);
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "Failed to initialize haptic device");
        fprintf(stderr, "\nPress any key to quit.\n");
        getchar();
        exit(-1);
    }

    printf("Found device %s\n",hdGetString(HD_DEVICE_MODEL_TYPE));
    
    hdEnable(HD_FORCE_OUTPUT);
    hdEnable(HD_MAX_FORCE_CLAMPING);

    hdStartScheduler();
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "Failed to start scheduler");
        fprintf(stderr, "\nPress any key to quit.\n");
        getchar();
        exit(-1);
    }
    
    initGlut(argc, argv);

    // Get the workspace dimensions.
    HDdouble maxWorkspace[6];
    hdGetDoublev(HD_MAX_WORKSPACE_DIMENSIONS, maxWorkspace);

    // Low/left/back point of device workspace.
    hduVector3Dd LLB(maxWorkspace[0], maxWorkspace[1], maxWorkspace[2]);
    // Top/right/front point of device workspace.
    hduVector3Dd TRF(maxWorkspace[3], maxWorkspace[4], maxWorkspace[5]);
    initGraphics(LLB, TRF);

    // Application loop.
    CoulombForceField();


	for (int i = 0; i < Shape_count; i++)
	{
		delete[] Shape[i];
	}
	delete[] Shape;
	delete[] Shape_sizes;
	
    printf("Done\n");
    return 0;
}

/******************************************************************************/
