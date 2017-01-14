#pragma once
#include "precompiled.h"
#include "util.h"

// todo - virtual dtor etc? see for now the protected ctor.
template<class T> struct FFTBase
{
	typedef T Prec;
	typedef Vec2<T> Complex;
	typedef T Real;
	typedef Array2D<Complex> CArray;
	typedef Array2D<Real> RArray;

protected: FFTBase() {}
};

template<class T> struct FFT_T
{
};

template<> struct FFT_T<float> : FFTBase<float> {
	fftwf_plan p;
	static FFT_T c2c(CArray& in, CArray& out, int direction, int flags)
	{
		return FFT_T(fftwf_plan_dft_2d(in.w, in.h, get_ptr(in), get_ptr(out), direction, flags));
	}
	static FFT_T r2c(RArray& in, CArray& out, int flags)
	{
		return FFT_T(fftwf_plan_dft_r2c_2d(in.w, in.h, get_ptr(in), get_ptr(out), flags));
	}
	static FFT_T c2r(CArray& in, RArray& out, int flags)
	{
		return FFT_T(fftwf_plan_dft_c2r_2d(out.w, out.h, get_ptr(in), get_ptr(out), flags));
	}
	void execute() { fftwf_execute(p); }
	void execute(RArray& in, CArray& out) { fftwf_execute_dft_r2c(p, get_ptr(in), get_ptr(out)); }
	void execute(CArray& in, RArray& out) { fftwf_execute_dft_c2r(p, get_ptr(in), get_ptr(out)); }

private:
	FFT_T(fftwf_plan p) { this->p = p; }
	static fftwf_complex* get_ptr(CArray& a) { return (fftwf_complex*)&a.data[0]; }
	static Real* get_ptr(RArray& a) { return &a.data[0]; }
};

template<> struct FFT_T<double> : FFTBase<double> {
	fftw_plan p;
	FFT_T(Array2D<Vec2d>& in, Array2D<Vec2d>& out, int direction, int flags)
	{
		p = fftw_plan_dft_2d(in.w, in.h, (fftw_complex*)&in.data[0], (fftw_complex*)&out.data[0], direction, flags);
	}
	void execute() { fftw_execute(p); }
};

template<class T>
inline std::complex<T> const& comp(Vec2<T> const& a)
{
	return (std::complex<T> const&)a;
}

template<class T>
inline std::complex<T>& comp(Vec2<T>& a)
{
	return (std::complex<T>&)a;
}