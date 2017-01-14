#pragma once
#include "precompiled.h"
#include "shade.h"

inline gl::Texture gpuBlurLevelDown(gl::Texture src);
inline gl::Texture gpuBlurLevelUp(gl::Texture src);
inline gl::Texture gpuBlur2(gl::Texture src, int r) {
	auto state = Shade().tex(src).expr("fetch3()").run();
	
	for(int i = 0; i < r; i++) {
		state = gpuBlurLevelDown(state);
	}
	for(int i = 0; i < r; i++) {
		state = gpuBlurLevelUp(state);
	}
	return state;
}
inline gl::Texture gpuBlurLevelDown(gl::Texture src) {
	float mouseY = AppBasic::get()->getMousePos().y / float(AppBasic::get()->getWindowHeight());

	//globaldict["gaussW"] = expRange(mouseY, .1f, 100.0f);
	// gauss(1.0) = .5
	// gauss(0.0) = 1.0
	// x=width
	// e^(-1/(x^2))=.5 => -1/(x^2)=log .5 => x^2=-1/(log .5) => x=sqrt(-1.0/log(.5))
	float gaussW = sqrt(-1.0f/log(.5f));
	globaldict["gaussW"] = gaussW;
	string shader =
		"float gauss(float f, float width) { return exp(-f*f/(width*width)); }"
		"void shade() {"
		"	vec2 offset = vec2(GB2_offsetX, GB2_offsetY);"
		"	vec3 a0 = fetch3(tex, tc + (-2.0) * offset * tsize);"
		"	vec3 a1 = fetch3(tex, tc + (-1.0) * offset * tsize);"
		"	vec3 a2 = fetch3(tex, tc + (0.0) * offset * tsize);"
		"	vec3 a3 = fetch3(tex, tc + (+1.0) * offset * tsize);"
		"	vec3 a4 = fetch3(tex, tc + (+2.0) * offset * tsize);"
		
		//"	_out = .05 * (a0 + a4) + .2 * (a1 + a3) + .5 * a2;"
		"	vec3 aM1 = fetch3(tex, tc + (-3.0) * offset * tsize);"
		"	vec3 a5 = fetch3(tex, tc + (+3.0) * offset * tsize);"
		//"	float gaussW = 1.0;"
		"   float w5=gauss(3.0, gaussW);"
		"   float w4=gauss(2.0, gaussW);"
		"   float w3=gauss(1.0, gaussW);"
		"   float w2=gauss(0.0, gaussW);"
		//"   float w5=.1, w4=.14, w3=.17, w2=.17;"
		"	_out = (w5 * (aM1 + a5) + w4 * (a0 + a4) + w3 * (a1 + a3) + w2 * a2) / (2.0*w5 + 2.0*w4 + 2.0*w3 + w2);"
		"}";

	globaldict["GB2_offsetX"] = 1.0;
	globaldict["GB2_offsetY"] = 0.0;
	auto hscaled = ::Shade().tex(src).src(shader).scale(.5f, 1.0f).run();
	globaldict["GB2_offsetX"] = 0.0;
	globaldict["GB2_offsetY"] = 1.0;
	auto vscaled = ::Shade().tex(hscaled).src(shader).scale(1.0f, .5f).run();
	return vscaled;
}

inline gl::Texture gpuBlurLevelUp(gl::Texture src) {
	//globaldict["gaussW_levelUp"] = //expRange(mouseY, .1f, 100.0f);
	//src.get
	float gaussW = sqrt(-(2.0f*2.0f)/log(.5f)); // gauss at 2.0f should be .5f
	globaldict["gaussW_levelUp"] = gaussW;
	string shader =
		"float gauss(float f, float width) { return exp(-f*f/(width*width)); }"
		"void shade() {"
		"	vec2 offset = vec2(GB2_offsetX, GB2_offsetY);"
		"	vec3 a0 = fetch3(tex, tc + (-2.0) * offset * tsize);"
		"	vec3 a1 = fetch3(tex, tc + (-1.0) * offset * tsize);"
		"	vec3 a2 = fetch3(tex, tc + (0.0) * offset * tsize);"
		"	vec3 a3 = fetch3(tex, tc + (+1.0) * offset * tsize);"
		"	vec3 a4 = fetch3(tex, tc + (+2.0) * offset * tsize);"
		
		//"	_out = .05 * (a0 + a4) + .2 * (a1 + a3) + .5 * a2;"
		"	vec3 aM1 = fetch3(tex, tc + (-3.0) * offset * tsize);"
		"	vec3 a5 = fetch3(tex, tc + (+3.0) * offset * tsize);"
		//"	float gaussW = 1.0;"
		/*"   float w5=gauss(3.0, gaussW);"
		"   float w4=gauss(2.0, gaussW);"
		"   float w3=gauss(1.0, gaussW);"
		"   float w2=gauss(0.0, gaussW);"*/
		//"   float w5=.1, w4=.14, w3=.17, w2=.17;"
		"	_out = (w5 * (aM1 + a5) + w4 * (a0 + a4) + w3 * (a1 + a3) + w2 * a2) / (2.0*w5 + 2.0*w4 + 2.0*w3 + w2);"
		"}";

	auto zeroUpscaleShader = "void shade() {"
		"ivec2 tc2i = ivec2(floor(tc * texSize));"
		"if(mod(tc2i[int(GB2_xyIndex)], 2) == 1) _out = vec3(0.0);"
		"else _out = fetch3(tex);"
		"}";
	globaldict["GB2_xyIndex"] = 0.0f;
	globaldict["GB2_offsetX"] = 1.0;
	globaldict["GB2_offsetY"] = 0.0;
	auto hzeroUpscaled = Shade().tex(src).src(zeroUpscaleShader).scale(2.0f, 1.0f).run();
	auto hfiltered = ::Shade().tex(hzeroUpscaled).src(shader).run();
	globaldict["GB2_xyIndex"] = 1.0f;
	globaldict["GB2_offsetX"] = 0.0;
	globaldict["GB2_offsetY"] = 1.0;
	auto vzeroUpscaled = Shade().tex(hfiltered).src(zeroUpscaleShader).scale(1.0f, 2.0f).run();
	auto vfiltered = ::Shade().tex(vzeroUpscaled).src(shader).run();
	return vfiltered;
}