#pragma once
#include "precompiled.h"
#include "shade.h"
#include "cfg1.h"
#include "stuff.h"

extern float mouseX, mouseY;
extern bool keys[];

namespace gpuBlur2_4 {

	inline gl::Texture singleblur(gl::Texture src, float hscale, float vscale);
	inline gl::Texture upscale(gl::Texture src, float hscale, float vscale);

	inline gl::Texture run(gl::Texture src, int lvls, int postBlurs) {
		auto state = Shade().tex(src).expr("fetch3()").run();
	
		for(int i = 0; i < lvls; i++) {
			state = singleblur(state, .5, .5);
		}
		for(int i = 0; i < postBlurs; i++)
		{
			state = singleblur(state, 1.0f, 1.0f);
		}
		state = upscale(state, src.getWidth() / (float)state.getWidth(), src.getHeight() / (float)state.getHeight());
		return state;
	}
	inline gl::Texture run_longtail(gl::Texture src, int lvls, float lvlmul) {
		auto zoomstate = Shade().tex(src).expr("fetch3()").run();
		//auto accstate = maketex(src.getWidth(), src.getHeight(), GL_RGBA16F);
		auto accstate = Shade().tex(src).expr("vec3(0.0)").run();
		
		for(int i = 0; i < lvls; i++) {
			zoomstate = singleblur(zoomstate, .5, .5);
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
	inline gl::Texture run_longtail_dbg(gl::Texture src, int lvls, float lvlmul) {
		auto zoomstate = Shade().tex(src).expr("fetch3()").run();
		//auto accstate = maketex(src.getWidth(), src.getHeight(), GL_RGBA16F);
		auto accstate = Shade().tex(src).expr("vec3(0.0)").run();
		accstate = shade2(accstate, zoomstate,
			"vec3 acc = fetch3(tex);"
			"vec3 nextzoom = fetch3(tex2);"
			"vec3 c = acc + nextzoom;"
			"_out = c;"
			);
		return accstate;
	}
	float getGaussW() {
		return cfg1::getOpt("gaussW",2.0f, [&]() { return keys['/']; },
			[&]() { return exp(mouseY*10.0f); });
	}
	float gauss(float f, float width) {
			float nfactor = 1.0 / (width * sqrt(twoPi));
			return nfactor * exp(-f*f/(width*width));
	}
	inline gl::Texture upscale(gl::Texture src, float hscale, float vscale) {
		globaldict["gaussW"]=getGaussW();
		globaldict["sqrtTwoPi"] = sqrt(twoPi);
		string lib =
			"float gauss(float f, float width) {"
			"	float nfactor = 1.0 / (width * sqrtTwoPi);"
			"	return nfactor * exp(-f*f/(width*width));"
			"}";
		string shader =
			"	vec2 offset = vec2(GB2_offsetX, GB2_offsetY);"
			"	vec2 tc2 = floor(tc * texSize) / texSize;"
			"	vec2 frXY = (tc - tc2) * texSize;"
			"	float fr = (GB2_offsetX == 1.0) ? frXY.x : frXY.y;"
			"	vec3 aM2 = fetch3(tex, tc2 + (-2.0) * offset * tsize);"
			"	vec3 aM1 = fetch3(tex, tc2 + (-1.0) * offset * tsize);"
			"	vec3 a0 = fetch3(tex, tc2 + (0.0) * offset * tsize);"
			"	vec3 aP1 = fetch3(tex, tc2 + (+1.0) * offset * tsize);"
			"	vec3 aP2 = fetch3(tex, tc2 + (+2.0) * offset * tsize);"
			"	"
			"	float wM2=gauss(-2.0-fr, gaussW);"
			"	float wM1=gauss(-1.0-fr, gaussW);"
			"	float w0=gauss(-fr, gaussW);"
			"	float wP1=gauss(1.0-fr, gaussW);"
			"	float wP2=gauss(2.0-fr, gaussW);"
			"	_out = wM2*aM2 + wM1*aM1 + w0*a0 + wP1*aP1 + wP2*aP2;";
		globaldict["GB2_offsetX"] = 1.0;
		globaldict["GB2_offsetY"] = 0.0;
		setWrapBlack(src);
		auto hscaled = shade2(src, shader, ShadeOpts().scale(hscale, 1.0f), lib);
		globaldict["GB2_offsetX"] = 0.0;
		globaldict["GB2_offsetY"] = 1.0;
		setWrapBlack(hscaled);
		auto vscaled = shade2(hscaled, shader, ShadeOpts().scale(1.0f, vscale), lib);
		return vscaled;
	}
	inline gl::Texture singleblur(gl::Texture src, float hscale, float vscale) {
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
		globaldict["GB2_offsetX"] = 0.0;
		globaldict["GB2_offsetY"] = 1.0;
		setWrapBlack(hscaled);
		auto vscaled = ::Shade().tex(hscaled).src(shader).scale(1.0f, vscale).run();
		return vscaled;
	}
}