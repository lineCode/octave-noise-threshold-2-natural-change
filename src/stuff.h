#pragma once
#include "precompiled.h"
#include "util.h"

const GLenum hdrFormat = GL_RGBA16F;
inline void gotoxy(int x, int y) { 
    COORD pos = {x, y};
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(output, pos);
}
inline void clearconsole() {
    COORD topLeft  = { 0, 0 };
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(
        console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    FillConsoleOutputAttribute(
        console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    SetConsoleCursorPosition(console, topLeft);
}
////////////////
#if 0
template<class Func>
struct ToBind
{
	/*function<Func> f;
	ToBind(Func f) : f(f){
	}*/
};

template<class Func>
ToBind<Func> toBind(Func func) {
	throw 0;
	//return ToBind<Func>(func);
}

void test()
{
	toBind(powf);
}
#endif

template<class F>
struct Transformed {
	F f;
	Transformed(F f) : f(f)
	{
	}
};

template<class Range, class F>
auto operator|(Array2D<Range> r, Transformed<F> transformed_) -> Array2D<decltype(transformed_.f(r(0, 0)))>
{
	Array2D<decltype(transformed_.f(r(0, 0)))> r2(r.w, r.h);
	forxy(r2)
	{
		r2(p) = transformed_.f(r(p));
	}
	return r2;
}

template<class F>
Transformed<F> transformed(F f)
{
	return Transformed<F>(f);
}

template<class T>
Vec2i wrapPoint(Array2D<T> const& src, Vec2i p)
{
	Vec2i wp = p;
	wp.x %= src.w; if(wp.x < 0) wp.x += src.w;
	wp.y %= src.h; if(wp.y < 0) wp.y += src.h;
	return wp;
}
template<class T>
T const& getWrapped(Array2D<T> const& src, Vec2i p)
{
	return src(wrapPoint(src, p));
}
template<class T>
T const& getWrapped(Array2D<T> const& src, int x, int y)
{
	return getWrapped(src, Vec2i(x, y));
}
template<class T>
T& getWrapped(Array2D<T>& src, Vec2i p)
{
	return src(wrapPoint(src, p));
}
template<class T>
T& getWrapped(Array2D<T>& src, int x, int y)
{
	return getWrapped(src, Vec2i(x, y));
}
struct WrapModes {
	struct GetWrapped {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			return ::getWrapped(src, x, y);
		}
	};
	struct Get_WrapZeros {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			return ::get_wrapZeros(src, x, y);
		}
	};
	struct NoWrap {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			return src(x, y);
		}
	};
	typedef GetWrapped DefaultImpl;
};
template<class T>
Array2D<T> blur(Array2D<T> const& src, int r, T zero = T())
{
	Array2D<T> newImg(sx, sy);
	float divide = pow(2.0f * r + 1.0f, 2.0f);
	for(int x = 0; x < sx; x++)
	{
		T sum = zero;
		for(int y = -r; y <= r; y++)
		{
			sum += get_clamped(src, x, y);
		}
		for(int y = 0; y < sy; y++)
		{
			newImg(x, y) = sum;
			sum -= get_clamped(src, x, y - r);
			sum += get_clamped(src, x, y + r + 1);
		}
	}
	Array2D<T> newImg2(sx, sy);
	for(int y = 0; y < sy; y++)
	{
		T sum = zero;
		for(int x = -r; x <= r; x++)
		{
			sum += get_clamped(newImg, x, y);
		}
		for(int x = 0; x < sx; x++)
		{
			newImg2(x, y) = sum;
			sum -= get_clamped(newImg, x - r, y);
			sum += get_clamped(newImg, x + r + 1, y);
		}
	}
	forxy(img)
		newImg2(p) /= divide;
	return newImg2;
}
template<class T>
Array2D<T> blur2(Array2D<T> const& src, int r)
{
	T zero = ::zero<T>();
	Array2D<T> newImg(sx, sy);
	float mul = 1.0f/pow(2.0f * r + 1.0f, 2.0f);
	auto blur1d = [&](int imax, T* p0, T* pEnd, int step) {
		vector<T> buffer;
		int windowSize = 2*r+1;
		buffer.resize(windowSize, zero);
		T* back = &buffer[0];
		T* front = &buffer[buffer.size()-1];
		T* dst=p0;
		for(int i = 0; i <= r; i++)
		{
			*(back+i+r)=*dst;
			dst += step;
		}
		dst=p0;
		T sum=zero;
		T* rStep=r*step;
		for(dst=p0; dst!=pEnd; dst+=step)
		{
			*dst=sum;
			sum-=*back;
			front++;if(front==&*buffer.end())front=&buffer[0];
			back++;if(back==&*buffer.end())back=&buffer[0];
			*front=*(dst+rStep);
			sum+=*front;
		}
	};

	for(int x = 0; x < sx; x++)
	{
		T sum = zero;
		for(int y = 0; y <= r; y++)
		{
			sum += src(x, y);
		}
		for(int y = 0; y < sy; y++)
		{
			newImg(x, y) = sum;
			sum -= get_wrapZeros(src, x, y - r);
			sum += get_wrapZeros(src, x, y + r + 1);
		}
	}
	Array2D<T> newImg2(sx, sy);
	for(int y = 0; y < sy; y++)
	{
		T sum = zero;
		for(int x = 0; x <= r; x++)
		{
			sum += newImg(x, y);
		}
		for(int x = 0; x < sx; x++)
		{
			newImg2(x, y) = sum;
			sum -= get_wrapZeros(newImg, x - r, y);
			sum += get_wrapZeros(newImg, x + r + 1, y);
		}
	}
	forxy(img)
		newImg2(p) *= mul;
	return newImg2;
}
template<class T>
void aaPoint_i2(Array2D<T>& dst, Vec2i p, T value)
{
	if(dst.contains(p))
		dst(p) += value;
}
template<class T>
void aaPoint_i2(Array2D<T>& dst, int x, int y, T value)
{
	if(dst.contains(x, y))
		dst(p) += value;
}
template<class T>
void aaPoint2(Array2D<T>& dst, Vec2f p, T value)
{
	aaPoint2(dst, p.x, p.y, value);
}
template<class T>
void aaPoint2(Array2D<T>& dst, float x, float y, T value)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
	if(x < 0.0f && fx != x) { fx--; ix--; }
	if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	float fractx1 = 1.0 - fractx;
	float fracty1 = 1.0 - fracty;
	get2(dst, ix, iy) += (fractx1 * fracty1) * value;
	get2(dst, ix, iy+1) += (fractx1 * fracty) * value;
	get2(dst, ix+1, iy) += (fractx * fracty1) * value;
	get2(dst, ix+1, iy+1) += (fractx * fracty) * value;
}
template<class T>
void aaPoint2_fast(Array2D<T>& dst, Vec2f p, T const& value)
{
	aaPoint2_fast(dst, p.x, p.y, value);
}
inline void my_assert_func(bool isTrue, string desc) {
	if(!isTrue) {
		cout << "assert failure: " << desc << endl;
		system("pause");
		throw std::runtime_error(desc.c_str());
	}
}
#define my_assert(isTrue) my_assert_func(isTrue, #isTrue);
//#define AAPOINT_DEBUG
template<class T>
void aaPoint2_fast(Array2D<T>& dst, float x, float y, T const& value)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
#ifdef AAPOINT_DEBUG
	my_assert(x>=0.0f);
	my_assert(y>=0.0f);
	my_assert(ix+1<=dst.w-1);
	my_assert(iy+1<=dst.h-1);
#endif
	//if(x < 0.0f && fx != x) { fx--; ix--; }
	//if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	//float fractx1 = 1.0 - fractx;
	//float fracty1 = 1.0 - fracty;
	
	T partB = fracty*value;
	T partT = value - partB;
	T partBR = fractx * partB;
	T partBL = partB - partBR;
	T partTR = fractx * partT;
	T partTL = partT - partTR;
	auto basePtr = &dst(ix, iy);
	basePtr[0] += partTL;
	basePtr[1] += partTR;
	basePtr[dst.w] += partBL;
	basePtr[dst.w+1] += partBR;
}
template<class T>
T& get2(Array2D<T>& src, int x, int y)
{
	static T t;
	if(src.contains(x, y))
	{
		return src(x, y);
	}
	else
	{
		return t;
	}
}
template<class T>
void aaPoint_i(Array2D<T>& dst, Vec2i p, T value)
{
	dst.wr(p) += value;
}
template<class T>
void aaPoint_i(Array2D<T>& dst, int x, int y, T value)
{
	dst.wr(x, y) += value;
}
template<class T>
void aaPoint_wrapZeros(Array2D<T>& dst, Vec2f p, T value)
{
	aaPoint_wrapZeros(dst, p.x, p.y, value);
}
template<class T>
void aaPoint_wrapZeros(Array2D<T>& dst, float x, float y, T value)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
	if(x < 0.0f && fx != x) { fx--; ix--; }
	if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	float fractx1 = 1.0 - fractx;
	float fracty1 = 1.0 - fracty;
	get_wrapZeros(dst, ix, iy) += (fractx1 * fracty1) * value;
	get_wrapZeros(dst, ix, iy+1) += (fractx1 * fracty) * value;
	get_wrapZeros(dst, ix+1, iy) += (fractx * fracty1) * value;
	get_wrapZeros(dst, ix+1, iy+1) += (fractx * fracty) * value;
}
template<class T>
void aaPoint(Array2D<T>& dst, Vec2f p, T value)
{
	aaPoint(dst, p.x, p.y, value);
}
template<class T>
void aaPoint(Array2D<T>& dst, float x, float y, T value)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
	if(x < 0.0f && fx != x) { fx--; ix--; }
	if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	float fractx1 = 1.0 - fractx;
	float fracty1 = 1.0 - fracty;
	dst.wr(ix, iy) += (fractx1 * fracty1) * value;
	dst.wr(ix, iy+1) += (fractx1 * fracty) * value;
	dst.wr(ix+1, iy) += (fractx * fracty1) * value;
	dst.wr(ix+1, iy+1) += (fractx * fracty) * value;
}
template<class T>
T getBilinear(Array2D<T> src, Vec2f p)
{
	return getBilinear(src, p.x, p.y);
}
template<class T>
T getBilinear(Array2D<T> src, float x, float y)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
	if(x < 0.0f && fx != x) { fx--; ix--; }
	if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	return lerp(
		lerp(getWrapped(src, ix, iy), getWrapped(src, ix + 1, iy), fractx),
		lerp(getWrapped(src, ix, iy + 1), getWrapped(src, ix + 1, iy + 1), fractx),
		fracty);
}

inline gl::Texture gtex(Array2D<float> a)
{
	gl::Texture::Format fmt;
	fmt.setInternalFormat(hdrFormat);
	gl::Texture tex(a.w, a.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_LUMINANCE, GL_FLOAT, a.data);
	return tex;
}
inline gl::Texture gtex(Array2D<Vec2f> a)
{
	gl::Texture::Format fmt;
	fmt.setInternalFormat(hdrFormat);
	gl::Texture tex(a.w, a.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RG, GL_FLOAT, a.data);
	return tex;
}
inline gl::Texture gtex(Array2D<Vec3f> a)
{
	gl::Texture::Format fmt;
	fmt.setInternalFormat(hdrFormat);
	gl::Texture tex(a.w, a.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGB, GL_FLOAT, a.data);
	return tex;
}
inline Array2D<float> to01(Array2D<float> a) {
	auto minn = *std::min_element(a.begin(), a.end());
	auto maxx = *std::max_element(a.begin(), a.end());
	auto b = a.clone();
	forxy(b) {
		b(p) -= minn;
		b(p) /= (maxx - minn);
	}
	return b;
}
inline Array2D<float> toRange(Array2D<float> inArr, float outMin, float outMax) {
	auto inMin = *std::min_element(inArr.begin(), inArr.end());
	auto inMax = *std::max_element(inArr.begin(), inArr.end());
	auto result = inArr.clone();
	forxy(result) {
		auto& val = result(p);
		val -= inMin;
		val /= (inMax - inMin);
		// now val is in [0, 1], can use it to lerp.
		val = lerp(outMin, outMax, val);
	}
	return result;
}
template<class T>
T& zero() {
	static T val=T()*0.0f;
	val = T()*0.0f;
	return val;
}

inline Vec2i clampPoint(Vec2i p, int w, int h)
{
	Vec2i wp = p;
	if(wp.x < 0) wp.x = 0;
	if(wp.x > w-1) wp.x = w-1;
	if(wp.y < 0) wp.y = 0;
	if(wp.y > h-1) wp.y = h-1;
	return wp;
}
template<class T>
T& get_clamped(Array2D<T>& src, int x, int y)
{
	return src(clampPoint(Vec2i(x, y), src.w, src.h));
}
template<class T>
T const& get_clamped(Array2D<T> const& src, int x, int y)
{
	return src(clampPoint(Vec2i(x, y), src.w, src.h));
}

template<class T>
Array2D<T> gauss3(Array2D<T> src) {
	T zero=::zero<T>();
	Array2D<T> dst1(sx, sy);
	Array2D<T> dst2(sx, sy);
	forxy(dst1)
		dst1(p) = .25f * (2 * get_clamped(src, p.x, p.y) + get_clamped(src, p.x-1, p.y) + get_clamped(src, p.x+1, p.y));
	forxy(dst2)
		dst2(p) = .25f * (2 * get_clamped(dst1, p.x, p.y) + get_clamped(dst1, p.x, p.y-1) + get_clamped(dst1, p.x, p.y+1));
	return dst2;
}

inline int sign(float f)
{
	if(f < 0)
		return -1;
	if(f > 0)
		return 1;
	return 0;
}
inline float expRange(float x, float min, float max) {
	return exp(lerp(log(min),log(max), x));
}

inline float niceExpRangeX(float mouseX, float min, float max) {
	float x2=sign(mouseX)*std::max(0.0f,abs(mouseX)-40.0f/(float)App::get()->getWindowWidth());
	return sign(x2)*expRange(abs(x2), min, max);
}

inline float niceExpRangeY(float mouseY, float min, float max) {
	float y2=sign(mouseY)*std::max(0.0f,abs(mouseY)-40.0f/(float)App::get()->getWindowHeight());
	return sign(y2)*expRange(abs(y2), min, max);
}

template<class Func>
class MapHelper {
private:
	static Func* func;
public:
	typedef typename decltype((*func)(Vec2i(0, 0))) result_dtype;
};

template<class TSrc, class Func>
auto map(Array2D<TSrc> a, Func func) -> Array2D<typename MapHelper<Func>::result_dtype> {
	auto result = Array2D<typename MapHelper<Func>::result_dtype>(a.w, a.h);
	forxy(a) {
		result(p) = func(p);
	}
	return result;
}

template<class T>
Vec2f gradient_i2(Array2D<T> src, Vec2i p)
{
	T nbs[3][3];
	for(int x = -1; x <= 1; x++) {
		for(int y = -1; y <= 1; y++) {
			nbs[x+1][y+1] = get_clamped(src, p.x + x, p.y + y);
		}
	}
	Vec2f gradient;
	T aTL = nbs[0][0];
	T aTC = nbs[1][0];
	T aTR = nbs[2][0];
	T aML = nbs[0][1];
	T aMR = nbs[2][1];
	T aBL = nbs[0][2];
	T aBC = nbs[1][2];
	T aBR = nbs[2][2];
	// removed '2' distance-denominator for backward compat for now
	float norm = 1.0f/16.f; //1.0f / 32.0f;
	gradient.x = ((3.0f * aTR + 10.0f * aMR + 3.0f * aBR) - (3.0f * aTL + 10.0f * aML + 3.0f * aBL)) * norm;
	gradient.y = ((3.0f * aBL + 10.0f * aBC + 3.0f * aBR) - (3.0f * aTL + 10.0f * aTC + 3.0f * aTR)) * norm;
	//gradient.x = get_clamped(src, p.x + 1, p.y) - get_clamped(src, p.x - 1, p.y);
	//gradient.y = get_clamped(src, p.x, p.y + 1) - get_clamped(src, p.x, p.y - 1);
	return gradient;
}

inline void mm(Array2D<float> arr, string desc="") {
	if(desc!="") {
		cout << "[" << desc << "] ";
	}
	cout<<"min: " << *std::min_element(arr.begin(),arr.end()) << ", ";
	cout<<"max: " << *std::max_element(arr.begin(),arr.end()) << endl;
}
//get_wrapZeros
template<class T>
T& get_wrapZeros(Array2D<T>& src, int x, int y)
{
	if(x < 0 || y < 0 || x >= src.w || y >= src.h)
	{
		return zero<T>();
	}
	return src(x, y);
}
template<class T>
T const& get_wrapZeros(Array2D<T> const& src, int x, int y)
{
	if(x < 0 || y < 0 || x >= src.w || y >= src.h)
	{
		return zero<T>();
	}
	return src(x, y);
}
template<class T>
Vec2f gradient_i(Array2D<T> src, Vec2i p)
{
	//if(p.x<1||p.y<1||p.x>=src.w-1||p.y>=src.h-1)
	//	return Vec2f::zero();
	Vec2f gradient;
	gradient.x = (get_wrapZeros(src,p.x + 1, p.y) - get_wrapZeros(src, p.x - 1, p.y)) / 2.0f;
	gradient.y = (get_wrapZeros(src,p.x, p.y + 1) - get_wrapZeros(src, p.x, p.y - 1)) / 2.0f;
	return gradient;
}
template<class T>
Array2D<Vec2f> get_gradients(Array2D<T> src)
{
	Array2D<Vec2f> gradients(sx, sy);
	forxy(gradients)
	{
		gradients(p) = gradient_i(img, p);
		//gradients(p) = Vec2f(-gradients(p).y, gradients(p).x);
	}
	return gradients;
}

inline gl::Texture maketex(int w, int h, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat); return gl::Texture(NULL, GL_RGBA, w, h, fmt);
}
inline gl::Texture maketex(Array2D<Vec3f> arr, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat);
	auto tex = gl::Texture(arr.w, arr.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, arr.w, arr.h, GL_RGB, GL_FLOAT, arr.data);
	return tex;
}
inline gl::Texture maketex(Array2D<Vec2f> arr, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat);
	auto tex = gl::Texture(arr.w, arr.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, arr.w, arr.h, GL_RG, GL_FLOAT, arr.data);
	return tex;
}
inline gl::Texture maketex(Array2D<float> arr, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat);
	auto tex = gl::Texture(arr.w, arr.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, arr.w, arr.h, GL_LUMINANCE, GL_FLOAT, arr.data);
	return tex;
}

template<class T>
Array2D<T> gettexdata(gl::Texture tex, GLenum format, GLenum type) {
	return gettexdata<T>(tex, format, type, tex.getBounds());
}

template<class T>
Array2D<T> gettexdata(gl::Texture tex, GLenum format, GLenum type, ci::Area area) {
	Array2D<T> data(area.getWidth(), area.getHeight());
	tex.bind();
	//glGetTexImage(GL_TEXTURE_2D, 0, format, type, data.data);
	beginRTT(tex);
	glReadPixels(area.x1, area.y1, area.getWidth(), area.getHeight(), format, type, data.data);
	endRTT();
	//glGetTexS
	GLenum errCode;
	if ((errCode = glGetError()) != GL_NO_ERROR) 
	{
		cout<<"ERROR"<<errCode<<endl;
	}
	return data;
}
inline float sq(float f) {
	return f * f;
}
inline vector<float> getGaussianKernel(int ksize) { // ksize must be odd
	float sigma = 0.3*((ksize-1)*0.5 - 1) + 0.8;
	vector<float> result;
	int r=ksize/2;
	float sum=0.0f;
	for(int i=-r;i<=r;i++) {
		float exponent = -(i*i/sq(2*sigma));
		float val = exp(exponent);
		sum += val;
		result.push_back(val);
	}
	for(int i=0; i<result.size(); i++) {
		result[i] /= sum;
	}
	return result;
}
template<class T,class FetchFunc>
Array2D<T> gaussianBlur(Array2D<T> src, int ksize) { // ksize must be odd. fastpath is for r%3==0.
	int r = ksize / 2;
	if(r % 3 == 0)
	{
		auto blurred = src;
		for(int i = 0; i < 3; i++)
		{
			blurred = blur(blurred, r / 3, ::zero<T>());
		}
		return blurred;
	}

	auto kernel = getGaussianKernel(ksize);
	T zero=::zero<T>();
	Array2D<T> dst1(src.w, src.h);
	Array2D<T> dst2(src.w, src.h);
	
	int w = src.w, h = src.h;
	
	// vertical

	auto runtime_fetch = FetchFunc::fetch<T>;
	for(int y = 0; y < h; y++)
	{
		auto blurVert = [&](int x0, int x1) {
			// guard against w<r
			x0 = max(x0, 0);
			x1 = min(x1, w);

			for(int x = x0; x < x1; x++)
			{
				T sum = zero;
				for(int xadd = -r; xadd <= r; xadd++)
				{
					sum += kernel[xadd + r] * (runtime_fetch(src, x + xadd, y));
				}
				dst1(x, y) = sum;
			}
		};

		
		blurVert(0, r);
		blurVert(w-r, w);
		for(int x = r; x < w-r; x++)
		{
			T sum = zero;
			for(int xadd = -r; xadd <= r; xadd++)
			{
				sum += kernel[xadd + r] * src(x + xadd, y);
			}
			dst1(x, y) = sum;
		}
	}
	
	// horizontal
	for(int x = 0; x < w; x++)
	{
		auto blurHorz = [&](int y0, int y1) {
			// guard against h<r
			y0 = max(y0, 0);
			y1 = min(y1, h);
			for(int y = y0; y < y1; y++)
			{
				T sum = zero;
				for(int yadd = -r; yadd <= r; yadd++)
				{
					sum += kernel[yadd + r] * runtime_fetch(dst1, x, y + yadd);
				}
				dst2(x, y) = sum;
			}
		};

		blurHorz(0, r);
		blurHorz(h-r, h);
		for(int y = r; y < h-r-1; y++)
		{
			T sum = zero;
			for(int yadd = -r; yadd <= r; yadd++)
			{
				sum += kernel[yadd + r] * dst1(x, y + yadd);
			}
			dst2(x, y) = sum;
		}
	}
	return dst2;
}
template<class T>
Array2D<T> gaussianBlur(Array2D<T> src, int ksize) {
	return gaussianBlur<T, WrapModes::DefaultImpl>(src, ksize);
}
inline void setWrapBlack(gl::Texture tex) {
	tex.bind();
	float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, black);
	tex.setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
}

namespace glsl_lib {
	inline string all() {
		return
			"		float getL(vec3 c) {"
			"			vec3 w = vec3(.22, .71, .07);"
			"			return dot(w, c);"
			"		}"
			"		float pi=3.14159265;"
			"		vec3 hsv_to_rgb(float h, float s, float v)"
			"		{"
			"			float c = v * s;"
			"			h = mod((h * 6.0), 6.0);"
			"			float x = c * (1.0 - abs(mod(h, 2.0) - 1.0));"
			"			vec3 color;"
			" "
			"			if (0.0 <= h && h < 1.0) {"
			"				color = vec3(c, x, 0.0);"
			"			} else if (1.0 <= h && h < 2.0) {"
			"				color = vec3(x, c, 0.0);"
			"			} else if (2.0 <= h && h < 3.0) {"
			"				color = vec3(0.0, c, x);"
			"			} else if (3.0 <= h && h < 4.0) {"
			"				color = vec3(0.0, x, c);"
			"			} else if (4.0 <= h && h < 5.0) {"
			"				color = vec3(x, 0.0, c);"
			"			} else if (5.0 <= h && h < 6.0) {"
			"				color = vec3(c, 0.0, x);"
			"			} else {"
			"				color = vec3(0.0, 0.0, 0.0);"
			"			}"
			" "
			"			color += v - c;"
			" "
			"			return color;"
			"		}";
	}
}

// http://stackoverflow.com/a/994647/122687
static unsigned int mylog2 (unsigned int val) {
	if (val == 0) return UINT_MAX;
	if (val == 1) return 0;
	unsigned int ret = 0;
	while (val > 1) {
		val >>= 1;
		ret++;
	}
	return ret;
}

// nullTerminatedLoadFile
inline string ntLoadFile(string filename)
{
	vector<unsigned char> vec;
	loadFile(vec, filename);
	return string(vec.begin(), vec.end());
}
// http://www.asawicki.info/news_1301_reflect_and_refract_functions.html
inline Vec3f refract(const Vec3f &I, const Vec3f &N, float eta)
{
	float N_dot_I = ci::dot(N, I);
	float k = 1.f - eta * eta * (1.f - N_dot_I * N_dot_I);
	if (k < 0.f)
		return Vec3f(0.f, 0.f, 0.f);
	else
		return eta * I - (eta * N_dot_I + sqrtf(k)) * N;
}

template<class T>
inline Vec2f gradient_f(Array2D<T> src, Vec2f p)
{
	Vec2f gradient;
	gradient.x = getBilinear(src, p.x + 1, p.y) - getBilinear(src, p.x - 1, p.y);
	gradient.y = getBilinear(src, p.x, p.y + 1) - getBilinear(src, p.x, p.y - 1);
	return gradient;
}