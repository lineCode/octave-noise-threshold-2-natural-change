#include "precompiled.h"
#include "stefanapp.h"

float mouseX, mouseY;
bool keys[256];
bool keys2[256];
bool mouseDown_[3];

void StefanApp::beginFrame() {
	::mouseX = getMousePos().x / (float)getWindowWidth();
	::mouseY = getMousePos().y / (float)getWindowHeight();

	my_console::beginFrame();
		
	sw::beginFrame();
	wsx = getWindowSize().x;
	wsy = getWindowSize().y;
}

void StefanApp::endFrame() {
	sw::endFrame();
	cfg1::print();
	my_console::endFrame();
}

void StefanApp::keyDown(KeyEvent e) {
	keys[e.getChar()] = true;
	if(e.isControlDown()&&e.getCode()!=KeyEvent::KEY_LCTRL)
	{
		keys2[e.getChar()] = !keys2[e.getChar()];
		return;
	}
}

void StefanApp::keyUp(KeyEvent e) {
		keys[e.getChar()] = false;
}

void StefanApp::mouseDown(MouseEvent e)
{
	mouseDown_[e.isLeft() ? 0 : e.isMiddle() ? 1 : 2] = true;
}
void StefanApp::mouseUp(MouseEvent e)
{
	mouseDown_[e.isLeft() ? 0 : e.isMiddle() ? 1 : 2] = false;
}

void StefanApp::draw()
{
	beginFrame();
	stefanUpdate();
	stefanDraw();
	endFrame();
}