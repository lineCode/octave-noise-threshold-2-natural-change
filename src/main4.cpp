#include "precompiled.h"
#if 1
#include "ciextra.h"
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

typedef Array2D<float> Image;
int wsx=800, wsy = 800 * (800.0f / 1280.0f);
int scale = 4;
int sx = wsx / scale;
int sy = wsy / scale;
Image img(sx, sy);
Image whiteNoiseState(sx, sy, 0.0f);
Array2D<Vec2f> velocity(sx, sy, Vec2f::zero());
bool mouseDown_[3];
bool keys[256];
gl::Texture::Format gtexfmt;
float noiseTimeDim = 0.0f;

float mouseX, mouseY;
bool pause;
bool keys2[256];

void updateConfig() {
}

struct SApp : AppBasic {
	Rectf area;
		
	void setup()
	{
		//keys2['0']=keys2['1']=keys2['2']=keys2['3']=true;
		//_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

		_controlfp(_DN_FLUSH, _MCW_DN);

		area = Rectf(0, 0, (float)sx-1, (float)sy-1).inflated(Vec2f::zero());

		createConsole();
		
		glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
		gtexfmt.setInternalFormat(hdrFormat);
		setWindowSize(wsx, wsy);

		forxy(img)
		{
			img(p) = ci::randFloat(); // NEWCODE
		}

		tmpEnergy = Array2D<Vec2f>(sx, sy);
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
	Vec2f direction;
	Vec2f lastm;
	void mouseDrag(MouseEvent e)
	{
		mm();
	}
	void mouseMove(MouseEvent e)
	{
		mm();
	}
	void mm()
	{
		direction = getMousePos() - lastm;
		lastm = getMousePos();
	}
	Vec2f reflect(Vec2f const & I, Vec2f const & N)
	{
		return I - N * N.dot(I) * 2.0f;
	}
	Array2D<float> img3;
	Array2D<Vec2f> tmpEnergy3;
	Array2D<Vec2f> tmpEnergy;

	void toTmpEnergy()
	{
		tmpEnergy = Array2D<Vec2f>(sx, sy);
		forxy(tmpEnergy)
		{
			tmpEnergy(p) = velocity(p) * img(p);
		}
	}

	void fromTmpEnergy()
	{
		forxy(tmpEnergy)
		{
			if(img(p) != 0.0f)
				velocity(p) = tmpEnergy(p) / img(p);
		}
	}

	void moveMatter(Vec2i p, Vec2f p2, float movedMatter, Vec2f transferredEnergy, Vec2f vec2)
	{
		aaPoint_i2(img3, p, -movedMatter);
		aaPoint2(img3, p2, movedMatter);
		
		aaPoint_i2(tmpEnergy3, p, -transferredEnergy);
		if(area.contains(Vec2f(p) + vec2))
			aaPoint2(tmpEnergy3, p2, transferredEnergy);
	}

	/*string tmsg;
	void tnext(string msg)
	{
		tmsg = msg;
	}*/
	void draw()
	{
		my_console::beginFrame();
		sw::beginFrame();
		static bool first = true;
		first = false;

		wsx = getWindowSize().x;
		wsy = getWindowSize().y;

		mouseX = getMousePos().x / (float)wsx;
		mouseY = getMousePos().y / (float)wsy;
		//if(keys['x']) {
		//	globaldict["ltm"] = sgn(mouseX) * exp(lmap(abs(mouseX), 0.0f, .3f, log(.001f), log(10.0f)));
		//}
		/*if(keys['y']) {
			globaldict["mul"] = mouseY;
		}
		if(keys['u']) {
			globaldict["mul2"] = mouseY;
		}*/
		auto surfTensionThres=cfg1::getOpt("surfTensionThres", .5f,
			[&]() { return keys['6']; },
			[&]() { return expRange(mouseY, 0.1f, 50000.0f); });
		auto surfTension=cfg1::getOpt("surfTension", 1.0f,
			[&]() { return keys['7']; },
			[&]() { return expRange(mouseY, .0001f, 40000.0f); });
		auto gravity=cfg1::getOpt("gravity", .1f,//0.0f,//.1f,
			[&]() { return keys['8']; },
			[&]() { return expRange(mouseY, .0001f, 40000.0f); });
		mouseY = getMousePos().y / (float)wsy;
		auto incompressibilityCoef=cfg1::getOpt("incompressibilityCoef", 1.0f,
			[&]() { return keys['\\']; },
			[&]() { return expRange(mouseY, .0001f, 40000.0f); });

		gl::clear(Color(0, 0, 0));

		if(!pause)
		{
			updateIt();
		} // if ! pause
		
		renderIt();

		/*Sleep(50);*/my_console::clr();
		sw::endFrame();
		cfg1::print();
		my_console::endFrame();

		if(pause)
			Sleep(50);
		//Sleep(5);
#if 0
		if(getElapsedFrames()==1){
			if(!SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS/*BELOW_NORMAL_PRIORITY_CLASS*/))
			{
				throw 0;
			}
		}
#endif
	}
	Array2D<float> getBandlimitedNoise(int blurDiam)
	{
		Array2D<float> result = whiteNoiseState.clone();
		result = gaussianBlur(result, blurDiam);
		/*for(int i = 0; i < 3; i++)
		{
			result = ::blur(result, blurDiam/3, ::zero<float>());
		}*/
		result = toRange(result, -1.0f, 1.0f);
		return result;
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
	Image getOctaveNoiseViaSummation()
	{
		Image result(sx, sy, 0.0f);
		float maxOctave = mylog2(sx)-1;
		cout << "maxOctave = " << maxOctave << endl;
		for(int octave = 0; octave <= maxOctave; octave++)
		{
			auto bln = getBandlimitedNoise(powf(2, maxOctave-octave));
			float weight = powf(.3f, octave); // should be .5f
			forxy(result) {
				result(p) += weight * bln(p);
			}
		}
		return result;
	}
	template<class T>
	static void normalizeFft(Array2D<T>& arr)
	{
		float norm = 1.0 / sqrt((float)arr.area);
		for(int i=0; i < arr.area; i++) arr(i) *= norm;
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
				const float fwidth = .36f;
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
	inline gl::Texture tmp_singleblurHorz(gl::Texture src, float hscale, float vscale) {
		using namespace gpuBlur2_4;
		// gauss(0.0) = 1.0
		// gauss(2.0) = 0.1
		//globaldict["gaussW0"]
		/*int ksizeR = 3;
		float gaussThres=.1f; // weight at last sample
		float e = exp(1.0f);
		float gaussW = 1.0f / (gaussThres * sqrt(twoPi * e));*/
		float gaussW=getGaussW();
		
		float w0=gauss(0.0, gaussW);
		float w1=gauss(1.0, gaussW);
		float w2=gauss(2.0, gaussW);
		//float w3=gauss(3.0, gaussW);
		float sum=/*2.0f*w3+*/2.0f*w2+2.0f*w1+w0;
		//w3/=sum;
		w2/=sum;
		w1/=sum;
		w0/=sum;
		stringstream weights;
		weights << fixed << "float w0="<<w0 << ", w1=" << w1 << ", w2=" << w2 << /*",w3=" << w3 <<*/ ";"<<endl;
		//system("pause");
		cout<<"weights: "<<weights.str()<<endl;

		string shader =
			"void shade() {"
			"	vec2 offset = vec2(GB2_offsetX, GB2_offsetY);"
			//"	vec3 aM3 = fetch3(tex, tc + (-3.0) * offset * tsize);"
			"	vec3 aM2 = fetch3(tex, tc + (-2.0) * offset * tsize);"
			"	vec3 aM1 = fetch3(tex, tc + (-1.0) * offset * tsize);"
			"	vec3 a0 = fetch3(tex, tc + (0.0) * offset * tsize);"
			"	vec3 aP1 = fetch3(tex, tc + (+1.0) * offset * tsize);"
			"	vec3 aP2 = fetch3(tex, tc + (+2.0) * offset * tsize);"
			//"	vec3 aP3 = fetch3(tex, tc + (+3.0) * offset * tsize);"
			""
			//"	float w2=0.0294118, w1=0.235294, w0=0.470588;"
			//"float w0=0.164872, w1=0.151377, w2=0.117165,w3=0.0764467;"
			+ weights.str() +
			//"	_out = .05 * (a0 + a4) + .2 * (a1 + a3) + .5 * a2;"
			"	_out = w2 * (aM2 + aP2) + w1 * (aM1 + aP1) + w0 * a0;"
			//"	_out += w3 * (aM3 + aP3);"
			"}";

		globaldict["GB2_offsetX"] = 1.0;
		globaldict["GB2_offsetY"] = 0.0;
		setWrapBlack(src);
		auto hscaled = ::Shade().tex(src).src(shader).scale(hscale, 1.0f).run();
		/*globaldict["GB2_offsetX"] = 0.0;
		globaldict["GB2_offsetY"] = 1.0;
		setWrapBlack(hscaled);
		auto vscaled = ::Shade().tex(hscaled).src(shader).scale(1.0f, vscale).run();
		return vscaled;*/
		return hscaled;
	}
	inline gl::Texture longtailBlurHorz(gl::Texture src, int lvls, float lvlmul) {
		auto zoomstate = Shade().tex(src).expr("fetch3()").run();
		//auto accstate = maketex(src.getWidth(), src.getHeight(), GL_RGBA16F);
		auto accstate = Shade().tex(src).expr("vec3(0.0)").run();
		
		for(int i = 0; i < lvls; i++) {
			//zoomstate = gpuBlur2_4::singleblur(zoomstate, .5, 1.0);
			zoomstate = tmp_singleblurHorz(zoomstate, .5, 1.0);
			if(zoomstate.getWidth() < 1 || zoomstate.getHeight() < 1) throw runtime_error("too many blur levels");
			globaldict["_mul"] = pow(lvlmul, float(i));
			accstate = shade2(accstate, zoomstate,
				"vec3 acc = fetch3(tex);"
				"vec3 nextzoom = fetch3(tex2);"
				"vec3 c = acc + nextzoom * _mul;"
				"_out = c;"
				);
		}
		//zoomstate = upscale(zoomstate, src.getWidth() / (float)zoomstate.getWidth(), src.getHeight() / (float)zoomstate.getHeight());
		return accstate;
	}
	void renderIt() {
		/*tex = shade2(tex,
			"vec3 c = fetch3();"
			"c *= .5;"
			"_out = c;");
		//auto texb = longtailBlurHorz(tex, 5, .5f); // .5f so it's not so broad
		auto texb = gpuBlur2_4::run_longtail(tex, 5, mouseY); // .5f so it's not so broad
		texb = shade2(tex, texb,
			"vec3 c = fetch3();"
			"vec3 blurred = fetch3(tex2) * 2.0;"
			"c = mix(blurred, vec3(1.0), c);"
			"_out = c;");
		auto grads = get_gradients_tex(texb);
		auto lighted = shade2(tex, grads,
			"float grad = fetch2(tex2).x;"
			"vec3 orange = vec3(1.0, .5, 0.0);"
			"vec3 teal = vec3(0.0, .5, 1.0);"
			"vec3 c;"
			"if(grad > 0.0)"
			"	c = orange * abs(grad);"
			"else"
			"	c = teal * abs(grad);"
			"c *= 4.0;"
			"c = mix(c, vec3(0.5), fetch3(tex));"
			//"c += fetch3(tex) * 2.0;"
			"_out = c;"
			);
		gl::draw(lighted, getWindowBounds());*/
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
		tex = shade2(tex, bgTex,
			"float f = fetch1();"
			"vec3 bgC = fetch3(tex2);"
			"vec3 c = bgC * (1.0 - f);"
			"_out = c;"
			);
		auto texb = gpuBlur2_4::run_longtail(tex, 3, 1.0f);
		tex = shade2(tex, texb,
			"vec3 c = fetch3();"
			"vec3 b = fetch3(tex2);"
			"_out = c + b * .2;");
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

//#include <lib/define_winmain.h>


//CINDER_APP_BASIC(SApp, RendererGl)
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

#endif