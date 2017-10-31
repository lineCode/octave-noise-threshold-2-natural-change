#include "precompiled.h"
#include "util.h"
//#include <boost/typeof/typeof.hpp>
//#include <boost/assign.hpp>
#include "stuff.h"
#include "shade.h"
#include "gpgpu.h"
#include "gpuBlur2_4.h"
#include "cfg1.h"
#include "sw.h"
#include "my_console.h"
#include "hdrwrite.h"
#include <float.h>
#include "simplexnoise.h"
#include "easyfft.h"
#include "stefanapp.h"

typedef Array2D<float> Image;
int wsx=800, wsy = 800 * (800.0f / 1280.0f);
int scale = 4;
int sx = wsx / scale;
int sy = wsy / scale;
Image img(sx, sy);
Image whiteNoiseState(sx, sy, 0.0f);
Array2D<Vec2f> velocity(sx, sy, Vec2f::zero());
float noiseTimeDim = 0.0f;

bool pause;

void updateConfig() {
}

struct SApp : AppBasic {
	void setup()
	{
		_controlfp(_DN_FLUSH, _MCW_DN);

		createConsole();
		
		glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
		setWindowSize(wsx, wsy);

		forxy(img)
		{
			img(p) = ci::randFloat(); // NEWCODE
		}
	}
	void keyDown(KeyEvent e)
	{
		keys[e.getChar()] = true;
		if(e.isControlDown()&&e.getCode()!=KeyEvent::KEY_LCTRL)
		{
			keys2[e.getChar()] = !keys2[e.getChar()];
			return;
		}
		if(keys['r'])
		{
			std::fill(img.begin(), img.end(), 0.0f);
			std::fill(velocity.begin(), velocity.end(), Vec2f::zero());
		}
		if(keys['p'] || keys['2'])
		{
			pause = !pause;
		}
	}
	void keyUp(KeyEvent e)
	{
		keys[e.getChar()] = false;
	}
	
	void mouseDown(MouseEvent e)
	{
		mouseDown_[e.isLeft() ? 0 : e.isMiddle() ? 1 : 2] = true;
	}
	void mouseUp(MouseEvent e)
	{
		mouseDown_[e.isLeft() ? 0 : e.isMiddle() ? 1 : 2] = false;
	}
	void draw()
	{
		my_console::beginFrame();
		sw::beginFrame();

		wsx = getWindowSize().x;
		wsy = getWindowSize().y;

		mouseX = getMousePos().x / (float)wsx;
		mouseY = getMousePos().y / (float)wsy;
		
		gl::clear(Color(0, 0, 0));

		if(!pause)
		{
			updateIt();
		} // if ! pause
		
		renderIt();

		sw::endFrame();
		cfg1::print();
		my_console::endFrame();

		if(pause)
			Sleep(50);
	}
	void updateBaseNoise()
	{
		static Image whiteNoiseChange(sx, sy, 0.0f);
		static Image whiteNoiseAcc(sx, sy, 0.0f);
		forxy(whiteNoiseState)
		{
			auto& accHere = whiteNoiseAcc(p);
			auto& changeHere = whiteNoiseChange(p);
			auto& stateHere = whiteNoiseState(p);
			float lerpSpeed = .1f;
			accHere = lerp(accHere, ci::randFloat(-1.0f, 1.0f), lerpSpeed);
			changeHere = lerp(changeHere, accHere, lerpSpeed);
			stateHere = lerp(stateHere, changeHere, lerpSpeed);
			//whiteNoiseAcc(p) += ci::randFloat(-1.0f, 1.0f)*.1;
			//whiteNoiseChange(p) += whiteNoiseAcc(p);
			//whiteNoiseState(p) += whiteNoiseChange(p);
			
		}
		//whiteNoiseAcc = toRange(whiteNoiseAcc, -1.0f, 1.0f);
		//whiteNoiseChange = toRange(whiteNoiseChange, -1.0f, 1.0f);
		//whiteNoiseState = toRange(whiteNoiseState, -1.0f, 1.0f);
	}
	Image getOctaveNoiseViaFft(float spectrumExponent) // nice exponents are 0, 1, 2
	{
		Array2D<Complexf> fftOut = fft(whiteNoiseState, FFTW_MEASURE);
		Vec2f arrCenter = fftOut.Size() / 2;
		forxy(fftOut)
		{
			Vec2i cp = p + arrCenter; // p centered
			cp.x %= fftOut.w;
			cp.y %= fftOut.h;
			float dist = cp.distance(arrCenter);
			if(dist != 0.0f)
				fftOut(p) *= 1.0f / powf(dist, spectrumExponent);
			auto& val = fftOut(p);
			//val = Complexf(abs(val), 0.0f); // set phase to 0
		}
		Array2D<float> result = ifft(fftOut, FFTW_MEASURE);
		return result;
	}
	void bwStyleSmoothen() {
		if(1)for(int i = 0; i < 8; i++) {
			img = gaussianBlur(img, 3);
			forxy(img)
			{
				auto& val = img(p);
				const float fwidth = .56f;
				val = smoothstep(-fwidth, fwidth, val);
				val -= .5f;
				val *= 2.0f;
			}
		}
		if(0) forxy(img)
		{
			img(p) = img(p) < 0.0f ? -1.0f : 1.0f;
		}
	}
	void updateIt() {
		updateBaseNoise();
		img = getOctaveNoiseViaFft(/*spectrumExponent*/0.0f);
		bwStyleSmoothen();
		img = to01(img);
		noiseTimeDim += .01f * expRange(mouseY, .1, 10);
	}
	void renderIt() {
		static Array2D<Vec3f> bg = (ci::Surface8u)ci::loadImage("loud colors.png");
		static auto bgTex = gtex(bg); // static bgTex
		auto tex = gtex(img);
		if(getElapsedFrames() == 1)
		{
			bgTex = shade2(bgTex,
				"vec3 c = fetch3();"
				"c = pow(c, vec3(2.2));"
				"c *= 4.0;"
				"_out = c;",
				ShadeOpts().scale(.5f)
				);
		}
		tex = gpuBlur2_4::run(tex, 1);
		tex = shade2(tex, bgTex,
			"float f = fetch1();"
			"vec3 bgC = fetch3(tex2);"
			"float fw = fwidth(f);"
			"float thres = .4;"
			"f = smoothstep(thres - fw / 2.0, thres + fw / 2.0, f);"
			"vec3 c = bgC * (1.0 - f);"
			"_out = c;"
			, ShadeOpts().scale(::scale)
			);
		//auto texb = gpuBlur2_4::run_longtail(tex, 3, 1.0f);
		auto texb = gpuBlur2_4::run_longtail(tex, 5, 0.5f);
		tex = shade2(tex, texb,
			"vec3 c = fetch3();"
			"vec3 b = fetch3(tex2);"
			"_out = c + b * .4 * 4.0;");
		tex = shade2(tex,
			"vec3 c = fetch3();"
			/*"c *= exp(-5.0+10.0*mouse.y);"
			"c /= c + vec3(1.0);"
			"c = smoothstep(vec3(0.0), vec3(1.0), c);"*/
			"c = pow(c, vec3(1.0 / 2.2));"
			"_out = c;"
			);
		gl::draw(tex, getWindowBounds());
	}
	void renderComplexTest()
	{
		Array2D<Complexf> fftOut = fft(whiteNoiseState, FFTW_MEASURE);
		auto tex = gtex((Array2D<Vec2f>&)fftOut);
		tex = shade2(tex,
			"vec3 c = fetch3();"
			"c = c * 1.5 + .5;"
			"_out = c;");
		gl::draw(tex, getWindowBounds());
	}
	void draw(gl::Texture tex, Rectf srcRect, Rectf dstRect)
	{
		glBegin(GL_QUADS);
		Rectf tc_(srcRect.getUpperLeft() / tex.getSize(), srcRect.getLowerRight() / tex.getSize());
		glTexCoord2f(tc_.getX1(), tc_.getY1()); glVertex2f(dstRect.getX1(), dstRect.getY1());
		glTexCoord2f(tc_.getX2(), tc_.getY1()); glVertex2f(dstRect.getX2(), dstRect.getY1());
		glTexCoord2f(tc_.getX2(), tc_.getY2()); glVertex2f(dstRect.getX2(), dstRect.getY2());
		glTexCoord2f(tc_.getX1(), tc_.getY2()); glVertex2f(dstRect.getX1(), dstRect.getY2());
		glEnd();
	}

};

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) {	
	try{
		createConsole();
		cinder::app::AppBasic::prepareLaunch();														
		cinder::app::AppBasic *app = new SApp;														
		cinder::app::Renderer *ren = new RendererGl;													
		cinder::app::AppBasic::executeLaunch( app, ren, "SApp" );										
		cinder::app::AppBasic::cleanupLaunch();														
		return 0;																					
	}catch(std::exception const& e) {
		cout << "caught: " << endl << e.what() << endl;
		//cout << "(exception type: " << typeinfo
		my_console::endFrame();
		//int dummy;cin>>dummy;
		system("pause");



	}
}
