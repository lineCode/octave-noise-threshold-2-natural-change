#pragma once
#include "precompiled.h"
#include "cfg1.h"
#include "my_console.h"
#include "sw.h"

extern float mouseX, mouseY;
extern bool keys[256];
extern bool keys2[256];
extern bool mouseDown_[3];
extern int wsx, wsy; // define and initialize those in main.cpp

class StefanApp : public ci::app::AppBasic {
public:
	void beginFrame();
	void endFrame();
	/*override*/ void keyDown(KeyEvent e);
	/*override*/ void keyUp(KeyEvent e);
	/*override*/ void mouseDown(MouseEvent e);
	/*override*/ void mouseUp(MouseEvent e);
	virtual void stefanUpdate() = 0;
	virtual void stefanDraw() = 0;

	/*override*/ void draw();
};