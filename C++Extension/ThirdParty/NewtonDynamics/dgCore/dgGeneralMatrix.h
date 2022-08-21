
/* Copyright (c) <2003-2019> <Julio Jerez, Newton Game Dynamics>
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __dgGeneralMatrix__
#define __dgGeneralMatrix__

#include "dgStdafx.h"
#include "dgDebug.h"
#include "dgVector.h"
#include "dgGeneralVector.h"

#define DG_LCP_MAX_VALUE dgFloat32 (1.0e10f)


template<dgInt32 maxRows>
class dgOldSolverNetwon_1_5
{
	public:
	dgOldSolverNetwon_1_5()
	{
	}

	void SetSize(dgInt32 size)
	{
		m_size = size;
	}

	dgInt32 GetSize() const
	{
		return m_size;
	}

	dgFloat32* GetX()
	{
		return m_x;
	}

	dgFloat32* GetB()
	{
		return m_b;
	}

	dgFloat32* GetLow()
	{
		return m_low;
	}

	dgFloat32* GetHigh()
	{
		return m_high;
	}

	dgInt32* GetFrictionIndex()
	{
		return m_frictionIndex;
	}

	dgFloat32* GetInvDiag()
	{
		return m_invDiag;
	}

	dgFloat32* GetMatrixRow(dgInt32 i)
	{
		return &m_matrix[i * m_size];
	}

	dgFloat32 Solve()
	{
		dgInt32 stride = 0;
		m_x[m_size] = dgFloat32(1.0f);
		dgFloat32 accelNorm = dgFloat32(0.0f);
		for (dgInt32 i = 0; i < m_size; i++) {
			dgVector error(m_b[i]);
			dgVector x(m_x[i]);
			const dgInt32 frictionIndex = m_frictionIndex[i];
			const dgVector low(m_low[i] * m_x[frictionIndex]);
			const dgVector high(m_high[i] * m_x[frictionIndex]);
			error = error & (x < high) & (x > low);
			accelNorm += error.GetScalar() * error.GetScalar();
			stride += m_size;
		}

		const dgFloat32 tol2 = dgFloat32(1.0e-5f);
		if (accelNorm > tol2) {
			dgFloat32 accelNorm0 = accelNorm;
			for (dgInt32 i = 0; (i < 5) && (accelNorm0 > tol2); i++) {
				stride = 0;
				accelNorm0 = dgFloat32(0.0f);
				for (dgInt32 j = 0; j < m_size; j++) {
					const dgFloat32* const row = &m_matrix[stride];
					dgFloat32 r = m_b[j];
					for (dgInt32 k = 0; k < m_size; k++) {
						r = r - row[k] * m_x[k];
					}
					const dgInt32 frictionIndex = m_frictionIndex[j];
					const dgVector low(m_low[j] * m_x[frictionIndex]);
					const dgVector high(m_high[j] * m_x[frictionIndex]);
					dgVector x((r + row[j] * m_x[j]) * m_invDiag[j]);

					dgVector a(r);
					a = a & (x < high) & (x > low);
					x = x.GetMax(low).GetMin(high);
					m_x[j] = x.GetScalar();

					accelNorm0 += a.GetScalar() * a.GetScalar();
					stride += m_size;
				}
			}

			if (accelNorm0 > tol2) {
				stride = 0;
				dgFloat32 mask[maxRows];
				for (dgInt32 i = 0; i < m_size; i++) {
					dgFloat32 r = dgFloat32(0.0f);
					const dgFloat32* const row = &m_matrix[stride];
					for (dgInt32 j = 0; j < m_size; j++) {
						r += row[j] * m_x[j];
					}
					m_b[i] -= r;
					m_delta_x[i] = m_b[i];
					mask[i] = dgFloat32(1.0f);
					const dgInt32 frictionIndex = m_frictionIndex[i];
					m_low[i] *= m_x[frictionIndex];
					m_high[i] *= m_x[frictionIndex];
					stride += m_size;
				}

				dgFloat32 beta = dgFloat32(1.0f);
				for (dgInt32 k = 0; (k < 20) && (beta > tol2); k++) {
					stride = 0;
					dgFloat32 num = dgFloat32(0.0f);
					dgFloat32 den = dgFloat32(0.0f);
					for (dgInt32 i = 0; i < m_size; i++) {
						const dgFloat32* const row = &m_matrix[stride];
						dgFloat32 r = dgFloat32(0.0f);
						for (dgInt32 j = 0; j < m_size; j++) {
							r += row[j] * m_delta_x[j];
						}
						stride += m_size;
						m_delta_r[i] = r;
						den += m_delta_x[i] * r;
						num += m_b[i] * m_b[i] * mask[i];
					}

					dgInt32 index = -1;
					dgFloat32 alpha = num / den;
					dgAssert(alpha > dgFloat32(0.0f));

					for (dgInt32 i = 0; (i < m_size) && (alpha > dgFloat32(0.0f)); i++) {

						if (m_delta_x[i]) {
							dgFloat32 x = m_x[i] + alpha * m_delta_x[i];
							if (x < m_low[i]) {
								index = i;
								alpha = (m_low[i] - m_x[i]) / m_delta_x[i];
							} else if (x > m_high[i]) {
								index = i;
								alpha = (m_high[i] - m_x[i]) / m_delta_x[i];
							}
							dgAssert(alpha >= dgFloat32(-1.0e-4f));
							if (alpha < dgFloat32(1.0e-6f)) {
								alpha = dgFloat32(0.0f);
							}
						}
					}

					beta = dgFloat32(0.0f);
					for (dgInt32 i = 0; i < m_size; i++) {
						m_x[i] += alpha * m_delta_x[i];
						m_b[i] -= alpha * m_delta_r[i];
						beta += m_b[i] * m_b[i] * mask[i];
					}

					if (index >= 0) {
						beta = dgFloat32(0.0f);
						mask[index] = dgFloat32(0.0f);
						for (dgInt32 i = 0; i < m_size; i++) {
							m_delta_x[i] = m_b[i] * mask[i];
							beta += m_b[i] * m_b[i] * mask[i];
							stride += m_size;
						}
					} else {
						alpha = beta / num;
						for (dgInt32 i = 0; i < m_size; i++) {
							m_delta_x[i] = m_b[i] * mask[i] + alpha * m_delta_x[i];
						}
					}
				}
			}
		}

		return accelNorm;
	}

	private:
	dgFloat32 m_x[maxRows + 4];
	dgFloat32 m_b[maxRows];
	dgFloat32 m_low[maxRows];
	dgFloat32 m_high[maxRows];
	dgFloat32 m_invDiag[maxRows];
	dgFloat32 m_delta_x[maxRows];
	dgFloat32 m_delta_r[maxRows];
	dgFloat32 m_matrix[maxRows * maxRows];
	dgInt32 m_frictionIndex[maxRows];
	dgInt32 m_size;
};


template<class T>
class dgSymmetricConjugateGradientSolver
{
	public:
	dgSymmetricConjugateGradientSolver();
	dgSymmetricConjugateGradientSolver(T* const r0, T* const z0, T* const p0, T* const q0);
	~dgSymmetricConjugateGradientSolver();

	void SetBuffers(T* const r0, T* const z0, T* const p0, T* const q0);
	T Solve(dgInt32 size, T tolerance, T* const x, const T* const b);

	protected:
	virtual void MatrixTimeVector(T* const out, const T* const v) const = 0;
	virtual void InversePrecoditionerTimeVector(T* const out, const T* const v) const = 0;

	private:
	T SolveInternal(dgInt32 size, T tolerance, T* const x, const T* const b) const;
	//T DotProduct(dgInt32 size, const T* const b, const T* const c) const;
	//void Sub(dgInt32 size, T* const a, const T* const b, const T* const c) const;
	//void ScaleAdd(dgInt32 size, T* const a, const T* const b, T scale, const T* const c) const;

	T* m_r0;
	T* m_z0;
	T* m_p0;
	T* m_q0;
};


template<class T>
dgSymmetricConjugateGradientSolver<T>::dgSymmetricConjugateGradientSolver()
{
	SetBuffers(NULL, NULL, NULL, NULL);
}

template<class T>
dgSymmetricConjugateGradientSolver<T>::dgSymmetricConjugateGradientSolver(T* const r0, T* const z0, T* const p0, T* const q0)
{
	SetBuffers(r0, z0, p0, q0);
}

template<class T>
dgSymmetricConjugateGradientSolver<T>::~dgSymmetricConjugateGradientSolver() 
{
}

template<class T>
void dgSymmetricConjugateGradientSolver<T>::SetBuffers(T* const r0, T* const z0, T* const p0, T* const q0)
{
	m_r0 = r0;
	m_z0 = z0;
	m_p0 = p0;
	m_q0 = q0;
}

template<class T>
T dgSymmetricConjugateGradientSolver<T>::Solve(dgInt32 size, T tolerance, T* const x, const T* const b)
{
	if (m_r0) {
		return SolveInternal(size, tolerance, x, b);
	} else {
		T* const r0 = dgAlloca(T, size);
		T* const z0 = dgAlloca(T, size);
		T* const p0 = dgAlloca(T, size);
		T* const q0 = dgAlloca(T, size);
		SetBuffers(r0, z0, p0, q0);
		T error = SolveInternal(size, tolerance, x, b);
		SetBuffers(NULL, NULL, NULL, NULL);
		return error;
	}
}

template<class T>
T dgSymmetricConjugateGradientSolver<T>::SolveInternal(dgInt32 size, T tolerance, T* const x, const T* const b) const
{
	MatrixTimeVector(m_z0, x);
	dgSub(size, m_r0, b, m_z0);
	InversePrecoditionerTimeVector(m_p0, m_r0);

	dgInt32 iter = 0;
	T num = dgDotProduct(size, m_r0, m_p0);
	T error2 = num;
	for (dgInt32 j = 0; (j < size) && (error2 > tolerance); j++) {

		MatrixTimeVector(m_z0, m_p0);
		T den = dgDotProduct(size, m_p0, m_z0);

		dgAssert(fabs(den) > T(0.0f));
		T alpha = num / den;

		dgMulAdd(size, x, x, m_p0, alpha);
		if ((j % 50) != 49) {
			dgMulAdd(size, m_r0, m_r0, m_z0, -alpha);
		} else {
			MatrixTimeVector(m_z0, x);
			dgSub(size, m_r0, b, m_z0);
		}

		InversePrecoditionerTimeVector(m_q0, m_r0);

		T num1 = dgDotProduct(size, m_r0, m_q0);
		T beta = num1 / num;
		dgMulAdd(size, m_p0, m_q0, m_p0, beta);
		num = dgDotProduct(size, m_r0, m_q0);
		iter++;
		error2 = num;
		if (j > 10) {
			error2 = T(0.0f);
			for (dgInt32 i = 0; i < size; i++) {
				error2 = dgMax(error2, m_r0[i] * m_r0[i]);
			}
		}
	}
	dgAssert(iter <= size);
	return num;
}


//*************************************************************
//
// generic linear algebra functions
//
//*************************************************************
template<class T>
void dgMatrixTimeVector(dgInt32 size, const T* const matrix, const T* const v, T* const out)
{
	dgInt32 stride = 0;
	for (dgInt32 i = 0; i < size; i++) {
		const T* const row = &matrix[stride];
		out[i] = dgDotProduct(size, row, v);
		stride += size;
	}
}

template<class T>
void dgMatrixTimeMatrix(dgInt32 size, const T* const matrixA, const T* const matrixB, T* const out)
{
	for (dgInt32 i = 0; i < size; i++) {
		const T* const rowA = &matrixA[i * size];
		T* const rowOut = &out[i * size];
		for (dgInt32 j = 0; j < size; j++) {
			T acc = T(0.0f);
			for (dgInt32 k = 0; k < size; k++) {
				acc += rowA[k] * matrixB[k * size + j];
			}
			rowOut[j] = acc;
		}
	}
}

template<class T>
void dgCovarianceMatrix(dgInt32 size, T* const matrix, const T* const vectorA, const T* const vectorB)
{
	dgInt32 stride = 0;
	for (dgInt32 i = 0; i < size; i++) {
		T* const row = &matrix[stride];
		T scale (vectorA[i]);
		for (dgInt32 j = 0; j < size; j++) {
			row[j] = scale * vectorA[j];
		}
		stride += size;
	}
}

template<class T>
DG_INLINE bool dgCholeskyFactorizationAddRow(dgInt32 size, dgInt32 stride, dgInt32 n, T* const matrix, T* const invDiagonalOut)
{
	T* const rowN = &matrix[stride * n];

	dgInt32 base = 0;
	for (dgInt32 j = 0; j <= n; j++) {
		T s(0.0f);
		T* const rowJ = &matrix[base];
		for (dgInt32 k = 0; k < j; k++) {
			s += rowN[k] * rowJ[k];
		}

		if (n == j) {
			T diag = rowN[n] - s;
			if (diag < T(1.0e-6f)) {
				return false;
			}

			rowN[n] = T(sqrt(diag));
			invDiagonalOut[n] = T(1.0f) / rowN[n];
		} else {
			rowJ[n] = T(0.0f);
			//rowN[j] = (rowN[j] - s) / rowJ[j];
			rowN[j] = invDiagonalOut[j] * (rowN[j] - s);
		}

		base += stride;
	}

	return true;
}

template<class T>
bool dgCholeskyFactorization(dgInt32 size, dgInt32 stride, T* const psdMatrix)
{
	bool state = true;
	T* const invDiagonal = dgAlloca(T, size);
	for (dgInt32 i = 0; (i < size) && state; i++) {
		state = state && dgCholeskyFactorizationAddRow(size, stride, i, psdMatrix, invDiagonal);
	}
	return state;
}

template<class T>
bool dgTestPSDmatrix(dgInt32 size, dgInt32 stride, T* const matrix)
{
	T* const copy = dgAlloca(T, size * size);
	int row = 0;
	for (int i = 0; i < size; i++) {
		memcpy(&copy[i * size], &matrix[row], size * sizeof (T));
		row += stride;
	}
	return dgCholeskyFactorization(size, size, copy);
}

template<class T>
void dgCholeskyApplyRegularizer (dgInt32 size, dgInt32 stride, T* const psdMatrix, T* const regularizer)
{
	bool isPsdMatrix = false;
	dgFloat32* const lowerTriangule = dgAlloca(dgFloat32, stride * stride);
	do {
		memcpy(lowerTriangule, psdMatrix, sizeof(dgFloat32) * stride * stride);
		isPsdMatrix = dgCholeskyFactorization(size, stride, lowerTriangule);
		if (!isPsdMatrix) {
			for (dgInt32 i = 0; i < size; i++) {
				regularizer[i] *= dgFloat32(4.0f);
				psdMatrix[i * stride + i] += regularizer[i];
			}
		}
	} while (!isPsdMatrix);
}

template<class T>
DG_INLINE void dgSolveCholesky(dgInt32 size, dgInt32 stride, const T* const choleskyMatrix, T* const x, const T* const b)
{
	dgInt32 rowStart = 0;
	for (dgInt32 i = 0; i < size; i++) {
		T acc(0.0f);
		const T* const row = &choleskyMatrix[rowStart];
		for (dgInt32 j = 0; j < i; j++) {
			acc = acc + row[j] * x[j];
		}
		x[i] = (b[i] - acc) / row[i];
		rowStart += stride;
	}

	for (dgInt32 i = size - 1; i >= 0; i--) {
		T acc = 0.0f;
		for (dgInt32 j = i + 1; j < size; j++) {
			acc = acc + choleskyMatrix[stride * j + i] * x[j];
		}
		x[i] = (x[i] - acc) / choleskyMatrix[stride * i + i];
	}
}

template<class T>
void dgSolveCholesky(dgInt32 size, T* const choleskyMatrix, T* const x)
{
	dgSolveCholesky(size, size, choleskyMatrix, x);
}

template<class T>
bool dgSolveGaussian(dgInt32 size, T* const matrix, T* const b)
{
	for (dgInt32 i = 0; i < size - 1; i++) {
		const T* const rowI = &matrix[i * size];
		dgInt32 m = i;
		T maxVal (dgAbs(rowI[i]));
		for (dgInt32 j = i + 1; j < size - 1; j++) {
			T val (dgAbs(matrix[size * j + i]));
			if (val > maxVal) {
				m = j;
				maxVal = val;
			}
		}

		if (maxVal < T(1.0e-12f)) {
			return false;
		}

		if (m != i) {
			T* const rowK = &matrix[m * size];
			T* const rowJ = &matrix[i * size];
			for (dgInt32 j = 0; j < size; j++) {
				dgSwap(rowK[j], rowJ[j]);
			}
			dgSwap(b[i], b[m]);
		}

		T den = T(1.0f) / rowI[i];
		for (dgInt32 k = i + 1; k < size; k++) {
			T* const rowK = &matrix[size * k];
			T factor(-rowK[i] * den);
			for (dgInt32 j = i + 1; j < size; j++) {
				rowK[j] += rowI[j] * factor;
			}
			rowK[i] = T(0.0f);
			b[k] += b[i] * factor;
		}
	}

	for (dgInt32 i = size - 1; i >= 0; i--) {
		T acc(0);
		T* const rowI = &matrix[i * size];
		for (dgInt32 j = i + 1; j < size; j++) {
			acc = acc + rowI[j] * b[j];
		}
		b[i] = (b[i] - acc) / rowI[i];
	}
	return true;
}

template <class T>
void dgEigenValues(const dgInt32 size, const dgInt32 stride, const T* const symmetricMatrix, T* const eigenValues)
{
	T* const offDiag = dgAlloca(T, size);
	T* const matrix = dgAlloca(T, size * stride);

	memcpy(matrix, symmetricMatrix, sizeof(T) * size * stride);
	for (dgInt32 i = size - 1; i > 0; i--) {
		T h(0.0f);
		T* const rowI = &matrix[i * stride];

		if (i > 1) {
			T scale(0.0f);
			for (dgInt32 k = 0; k < i; k++) {
				scale += dgAbs(rowI[k]);
			}

			if (scale == T(0.0f)) {
				offDiag[i] = rowI[i - 1];
			} else {
				for (dgInt32 k = 0; k < i; k++) {
					rowI[k] /= scale;
					h += rowI[k] * rowI[k];
				}

				T f(rowI[i - 1]);
				T g((f >= T(0.0f) ? -T(sqrt(h)) : T(sqrt(h))));
				offDiag[i] = scale * g;
				h -= f * g;
				rowI[i - 1] = f - g;
				f = T(0.0f);

				for (dgInt32 j = 0; j < i; j++) {
					g = T(0.0f);
					const T* const rowJ = &matrix[j * stride];
					for (dgInt32 k = 0; k <= j; k++) {
						g += rowJ[k] * rowI[k];
					}
					for (dgInt32 k = j + 1; k < i; k++) {
						g += matrix[k * stride + j] * rowI[k];
					}
					offDiag[j] = g / h;
					f += offDiag[j] * rowI[j];
				}

				T hh(f / (h + h));
				for (dgInt32 j = 0; j < i; j++) {
					T f1 (rowI[j]);
					T g1(offDiag[j] - hh * f1);
					offDiag[j] = g1;
					T* const rowJ = &matrix[j * stride];
					for (dgInt32 k = 0; k <= j; k++) {
						rowJ[k] -= (f1 * offDiag[k] + g1 * rowI[k]);
					}
				}
			}
		} else {
			offDiag[i] = rowI[i - 1];
		}
		eigenValues[i] = h;
	}

	dgInt32 index = stride;
	eigenValues[0] = matrix[0];
	for (dgInt32 i = 1; i < size; i++) {
		eigenValues[i] = matrix[index + i];
		offDiag[i - 1] = offDiag[i];
		index += stride;
	}

	for (dgInt32 i = 0; i < size; i++) {
		dgInt32 j;
		dgInt32 iter = 0;
		do {
			for (j = i; j < size - 1; j++) {
  				T dd(dgAbs(eigenValues[j]) + dgAbs(eigenValues[j + 1]));
				if (dgAbs(offDiag[j]) <= (T(1.e-6f) * dd)) {
					break;
				}
			}

			if (j != i) {
				iter++;
				if (iter == 10) {
					dgAssert(0);
					return;
				}

				T g((eigenValues[i + 1] - eigenValues[i]) / (T(2.0f) * offDiag[i]));
				T r(dgPythag(g, T(1.0f)));
				g = eigenValues[j] - eigenValues[i] + offDiag[i] / (g + dgSign(r, g));
				T s(1.0f);
				T c(1.0f);
				T p(0.0f);

				dgInt32 k;
				for (k = j - 1; k >= i; k--) {
					T f(s * offDiag[k]);
					T b(c * offDiag[k]);
					T d(dgPythag(f, g));
					offDiag[k + 1] = d;
					if (d == T(0.0f)) {
						eigenValues[k + 1] -= p;
						offDiag[j] = T(0.0f);
						break;
					}
					s = f / d;
					c = g / d;
					g = eigenValues[k + 1] - p;
					d = (eigenValues[k] - g) * s + T(2.0f) * c * b;
					p = s * d;
					eigenValues[k + 1] = g + p;
					g = c * d - b;
				}

				if (r == T(0.0f) && k >= i) {
					continue;
				}
				eigenValues[i] -= p;
				offDiag[i] = g;
				offDiag[j] = T(0.0f);
			}
		} while (j != i);
	}
}

template <class T>
T dgConditionNumber(const dgInt32 size, const dgInt32 stride, const T* const choleskyMatrix)
{
	T* const eigenValues = dgAlloca(T, size);
	dgEigenValues(size, stride, choleskyMatrix, eigenValues);

	T minVal = T(1.0e20f);
	T maxVal = T(-1.0e20f);
	for (int i = 0; i < size; i++) {
		minVal = dgMin(minVal, eigenValues[i]);
		maxVal = dgMax(maxVal, eigenValues[i]);
	}
	T condition = T(dgAbs(maxVal) / dgAbs(minVal));
	return condition;
}


// solve a general Linear complementary program (LCP)
// A * x = b + r
// subjected to constraints
// x(i) = low(i),  if r(i) >= 0  
// x(i) = high(i), if r(i) <= 0  
// low(i) <= x(i) <= high(i),  if r(i) == 0  
//
// return true is the system has a solution.
// in return 
// x is the solution,
// r is return in vector b
// note: although the system is called LCP, the solver is far more general than a strict LCP
// to solve a strict LCP, set the following
// low(i) = 0
// high(i) = infinity.
// this the same as enforcing the constraint: x(i) * r(i) = 0
template <class T>
void dgGaussSeidelLcpSor(const dgInt32 size, const T* const matrix, T* const x, const T* const b, const T* const low, const T* const high, T tol2, dgInt32 maxIterCount, dgInt16* const clipped, T sor)
{
	const T* const me = matrix;
	T* const invDiag1 = dgAlloca(T, size);

	dgInt32 stride = 0;
	for (dgInt32 i = 0; i < size; i++) {
		x[i] = dgClamp(T(0.0f), low[i], high[i]);
		invDiag1[i] = T(1.0f) / me[stride + i];
		stride += size;
	}

	T tolerance(tol2 * 2.0f);
	const T* const invDiag = invDiag1;
#ifdef _DEBUG 
	dgInt32 passes = 0;
#endif
	for (dgInt32 i = 0; (i < maxIterCount) && (tolerance > tol2); i++) {
		dgInt32 base = 0;
		tolerance = T(0.0f);
#ifdef _DEBUG 
		passes++;
#endif
		for (dgInt32 j = 0; j < size; j++) {
			const T* const row = &me[base];
			T r(b[j] - dgDotProduct(size, row, x));
			T f((r + row[j] * x[j]) * invDiag[j]);
			if (f > high[j]) {
				x[j] = high[j];
				clipped[j] = 1;
			} else if (f < low[j]) {
				x[j] = low[j];
				clipped[j] = 1;
			} else {
				clipped[j] = 0;
				tolerance += r * r;
				x[j] = x[j] + (f - x[j]) * sor;
			}
			base += size;
		}
	}
}


// solve a general Linear complementary program (LCP)
// A * x = b + r
// subjected to constraints
// x(i) = low(i),  if r(i) >= 0  
// x(i) = high(i), if r(i) <= 0  
// low(i) <= x(i) <= high(i),  if r(i) == 0  
//
// return true is the system has a solution.
// in return 
// x is the solution,
// r is return in vector b
// note: although the system is called LCP, the solver is far more general than a strict LCP
// to solve a strict LCP, set the following
// low(i) = 0
// high(i) = infinity.
// this the same as enforcing the constraint: x(i) * r(i) = 0
template <class T>
void dgGaussSeidelLCP(const dgInt32 size, const T* const matrix, T* const x, const T* const b, const T* const low, const T* const high, T sor = T(1.2f))
{
	dgInt16* const clipped = dgAlloca(dgInt16, size);
	dgGaussSeidelLcpSor(size, matrix, x, b, low, high, T(1.0e-3f), size * size, clipped, sor);
}

template<class T>
void dgPermuteRows(dgInt32 size, dgInt32 i, dgInt32 j, T* const matrix, T* const choleskyMatrix, T* const x, T* const r, T* const low, T* const high, dgInt16* const permute)
{
	if (i != j) {
		T* const A = &matrix[size * i];
		T* const B = &matrix[size * j];
		T* const invA = &choleskyMatrix[size * i];
		T* const invB = &choleskyMatrix[size * j];
		for (dgInt32 k = 0; k < size; k++) {
			dgSwap(A[k], B[k]);
			dgSwap(invA[k], invB[k]);
		}

		dgInt32 stride = 0;
		for (dgInt32 k = 0; k < size; k++) {
			dgSwap(matrix[stride + i], matrix[stride + j]);
			stride += size;
		}

		dgSwap(x[i], x[j]);
		dgSwap(r[i], r[j]);
		dgSwap(low[i], low[j]);
		dgSwap(high[i], high[j]);
		dgSwap(permute[i], permute[j]);
	}
}

template<class T>
DG_INLINE void dgCalculateDelta_x(dgInt32 size, dgInt32 n, const T* const matrix, const T* const choleskyMatrix, T* const delta_x)
{
	const T* const row = &matrix[size * n];
	for (dgInt32 i = 0; i < n; i++) {
		delta_x[i] = -row[i];
	}
	dgSolveCholesky(size, n, choleskyMatrix, delta_x, delta_x);
	delta_x[n] = T(1.0f);
}

// calculate delta_r = A * delta_x
template<class T>
DG_INLINE void dgCalculateDelta_r(dgInt32 size, dgInt32 n, const T* const matrix, const T* const delta_x, T* const delta_r)
{
	dgInt32 stride = n * size;
	const dgInt32 size1 = n + 1;
	for (dgInt32 i = n; i < size; i++) {
		delta_r[i] = dgDotProduct(size1, &matrix[stride], delta_x);
		stride += size;
	}
}

template<class T>
DG_INLINE void dgHouseholderReflection(dgInt32 size, dgInt32 row, dgInt32 colum, T* const choleskyMatrix, T* const tmp, T* const reflection)
{
	dgAssert(row <= colum);
	if (row < colum) {
		for (dgInt32 i = row; i <= colum; i++) {
			T* const rowI = &choleskyMatrix[size * i];
			T mag2(0.0f);
			for (dgInt32 j = i + 1; j <= colum; j++) {
				mag2 += rowI[j] * rowI[j];
				reflection[j] = rowI[j];
			}
			if (mag2 > T(1.0e-14f)) {
				reflection[i] = rowI[i] + dgSign(rowI[i]) * T(sqrt(mag2 + rowI[i] * rowI[i]));

				const T vMag2(mag2 + reflection[i] * reflection[i]);
				const T den = T(2.0f) / vMag2;
				for (dgInt32 j = i; j < size; j++) {
					T acc(0.0f);
					T* const rowJ = &choleskyMatrix[size * j];
					for (dgInt32 k = i; k <= colum; k++) {
						acc += rowJ[k] * reflection[k];
					}
					tmp[j] = acc;
				}

				for (dgInt32 j = i + 1; j < size; j++) {
					rowI[j] = T(0.0f);
					T* const rowJ = &choleskyMatrix[size * j];
					const T a = tmp[j] * den;
					for (dgInt32 k = i; k <= colum; k++) {
						rowJ[k] -= a * reflection[k];
					}
				}
				rowI[i] -= tmp[i] * reflection[i] * den;
			}

			if (rowI[i] < T(0.0f)) {
				for (dgInt32 k = i; k < size; k++) {
					choleskyMatrix[size * k + i] = -choleskyMatrix[size * k + i];
				}
			}
		}

		for (dgInt32 i = row; i < size; i++) {
			choleskyMatrix[size * i + i] = dgMax(choleskyMatrix[size * i + i], T(1.0e-6f));
		}
	}
}

template<class T>
void dgCholeskyUpdate(dgInt32 size, dgInt32 row, dgInt32 colum, T* const choleskyMatrix, T* const tmp, T* const reflexion, const T* const psdMatrix)
{
	const dgInt32 n0 = colum - row;
	const dgInt32 n1 = n0 + 1;
	const dgInt32 choleskyCost = size * size * size / 3;
	const dgInt32 householdCost = n0 * (n0 + 1) / 2 + n1 * (n1 + 1) * (2 * (2 * n1 + 1) - 3 + 3 * (size - colum - 1)) / 6 - 1;

	if (householdCost < choleskyCost) {
		dgHouseholderReflection(size, row, colum, choleskyMatrix, tmp, reflexion);
	} else {
		memcpy (choleskyMatrix, psdMatrix, sizeof (T) * size * size);
		dgCholeskyFactorization(size, choleskyMatrix);
	}

//#if _DEBUG
#if 0
	T* const psdMatrixCopy = dgAlloca(T, size * size);
	memcpy(psdMatrixCopy, psdMatrix, sizeof(T) * size * size);
	dgCholeskyFactorization(size, psdMatrixCopy);

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			T err = psdMatrixCopy[i*size + j] - choleskyMatrix[i*size + j];
			dgAssert(dgAbs(err) < T(1.0e-4f));
		}
	}
#endif
}

// solve a general Linear complementary program (LCP)
// A * x = b + r
// subjected to constraints
// x(i) = low(i),  if r(i) >= 0  
// x(i) = high(i), if r(i) <= 0  
// low(i) <= x(i) <= high(i),  if r(i) == 0  
//
// return true is the system has a solution.
// in return 
// x is the solution,
// r is return in vector b
// note: although the system is called LCP, the solver is far more general than a strict LCP
// to solve a strict LCP, set the following
// low(i) = 0
// high(i) = infinity.
// this the same as enforcing the constraint: x(i) * r(i) = 0
template <class T>
void dgSolveDantzigLcpLow(dgInt32 size, T* const symmetricMatrixPSD, T* const x, T* const b, T* const low, T* const high)
{
	T* const x0 = dgAlloca(T, size);
	T* const r0 = dgAlloca(T, size);
	T* const tmp0 = dgAlloca(T, size);
	T* const tmp1 = dgAlloca(T, size);
	T* const delta_r = dgAlloca(T, size);
	T* const delta_x = dgAlloca(T, size);
	T* const lowerTriangularMatrix = dgAlloca(T, size * size);
	dgInt16* const permute = dgAlloca(dgInt16, size);

	for (dgInt32 i = 0; i < size; i++) {
		permute[i] = dgInt16(i);
		x0[i] = T(0.0f);
		x[i] = dgMax (b[i] * b[i], T (1.0f));
	}

	for (dgInt32 n = size - 1, i = size - 1; i >= 0; i--) {
		if (x[i] > T(1.0)) {
			dgPermuteRows(size, n, i, symmetricMatrixPSD, lowerTriangularMatrix, x, b, low, high, permute);
			n --;
		}
	}

	for (dgInt32 i = size - 1; (i >= 0) && (x[i] > T(1.0f)) ; i--) {
		dgInt32 min = i;
		for (dgInt32 j = i - 1; (j >= 0) && (x[j] > T(1.0f)); j--) {
			if (x[j] > x[min]) {
				min = j;
			}
		}
		if (min != i) {
			dgPermuteRows(size, i, min, symmetricMatrixPSD, lowerTriangularMatrix, x, b, low, high, permute);
		}
	}

	dgInt32 initialGuessCount = size;
	while (x[initialGuessCount - 1] >= T(16.0f)) {
		initialGuessCount --;
	}
	
	memcpy(lowerTriangularMatrix, symmetricMatrixPSD, sizeof(T) * size * size);
#ifdef _DEBUG
	bool valid = dgCholeskyFactorization(size, lowerTriangularMatrix);
	dgAssert(valid);
#else
	dgCholeskyFactorization(size, lowerTriangularMatrix);
#endif
	for (dgInt32 j = 0; (j != -1) && initialGuessCount;) {
		dgSolveCholesky(size, initialGuessCount, lowerTriangularMatrix, x0, b);

		j = -1;
		T alpha(1.0f);
		T value(0.0f);
		for (dgInt32 i = initialGuessCount - 1; i >= 0; i--) {
			T x1 = alpha * x0[i];
			if (x1 < low[i]) {
				j = i;
				value = low[i];
				alpha = low[i] / x0[i];
			} else if (x1 > high[i]) {
				j = i;
				value = high[i];
				alpha = high[i] / x0[i];
			}
		}

		if (j != -1) {
			x0[j] = value;
			initialGuessCount--;
			dgPermuteRows(size, j, initialGuessCount, symmetricMatrixPSD, lowerTriangularMatrix, x0, b, low, high, permute);
			dgCholeskyUpdate(size, j, initialGuessCount, lowerTriangularMatrix, tmp0, tmp1, symmetricMatrixPSD);
		}
	}

	if (initialGuessCount == size) {
		for (dgInt32 i = 0; i < size; i++) {
			dgInt32 j = permute[i];
			x[j] = x0[i];
			b[i] = T(0.0f);
		}
		return;
	}

	dgInt32 clampedIndex = size;
	dgInt32 index = initialGuessCount;
	dgInt32 count = size - initialGuessCount;
	dgInt32 stride = index * size;

	for (dgInt32 i = 0; i < size; i++) {
		r0[i] = T(0.0f);
		delta_x[i] = T(0.0f);
		delta_r[i] = T(0.0f);
	}
	
	for (dgInt32 i = index; i < size; i++) {
		r0[i] = dgDotProduct(size, &symmetricMatrixPSD[stride], x0) - b[i];
		stride += size;
	}


	while (count) {
		bool loop = true;

		while (loop) {
			loop = false;
			T clamp_x(0.0f);
			dgInt32 swapIndex = -1;

			if (dgAbs(r0[index]) > T(1.0e-12f)) {
				dgCalculateDelta_x(size, index, symmetricMatrixPSD, lowerTriangularMatrix, delta_x);
				dgCalculateDelta_r(size, index, symmetricMatrixPSD, delta_x, delta_r);

				dgAssert(delta_r[index] != T(0.0f));
				dgAssert(dgAbs(delta_x[index]) == T(1.0f));
				delta_r[index] = (delta_r[index] == T(0.0f)) ? T(1.0e-12f) : delta_r[index];

				T scale = -r0[index] / delta_r[index];
				dgAssert(dgAbs(scale) >= T(0.0f));

				for (dgInt32 i = 0; i <= index; i++) {
					T x1 = x0[i] + scale * delta_x[i];
					if (x1 > high[i]) {
						swapIndex = i;
						clamp_x = high[i];
						scale = (high[i] - x0[i]) / delta_x[i];
					} else if (x1 < low[i]) {
						swapIndex = i;
						clamp_x = low[i];
						scale = (low[i] - x0[i]) / delta_x[i];
					}
				}
				dgAssert(dgAbs(scale) >= T(0.0f));

				for (dgInt32 i = clampedIndex; (i < size) && (scale > T(1.0e-12f)); i++) {
					T r1 = r0[i] + scale * delta_r[i];
					if ((r1 * r0[i]) < T(0.0f)) {
						dgAssert(dgAbs(delta_r[i]) > T(0.0f));
						T s1 = -r0[i] / delta_r[i];
						dgAssert(dgAbs(s1) >= T(0.0f));
						dgAssert(dgAbs(s1) <= dgAbs(scale));
						if (dgAbs(s1) < dgAbs(scale)) {
							scale = s1;
							swapIndex = i;
						}
					}
				}

				if (dgAbs(scale) > T(1.0e-12f)) {
					for (dgInt32 i = 0; i < size; i++) {
						x0[i] += scale * delta_x[i];
						r0[i] += scale * delta_r[i];
					}
				}
			}

			if (swapIndex == -1) {
				r0[index] = T(0.0f);
				delta_r[index] = T(0.0f);
				index++;
				count--;
				loop = false;
			} else if (swapIndex == index) {
				count--;
				clampedIndex--;
				x0[index] = clamp_x;
				dgPermuteRows(size, index, clampedIndex, symmetricMatrixPSD, lowerTriangularMatrix, x0, r0, low, high, permute);
				dgCholeskyUpdate(size, index, clampedIndex, lowerTriangularMatrix, tmp0, tmp1, symmetricMatrixPSD);
				loop = count ? true : false;
			} else if (swapIndex > index) {
				loop = true;
				r0[swapIndex] = T(0.0f);
				dgAssert(swapIndex < size);
				dgAssert(clampedIndex <= size);
				if (swapIndex < clampedIndex) {
					count--;
					clampedIndex--;
					dgPermuteRows(size, clampedIndex, swapIndex, symmetricMatrixPSD, lowerTriangularMatrix, x0, r0, low, high, permute);
					dgCholeskyUpdate(size, swapIndex, clampedIndex, lowerTriangularMatrix, tmp0, tmp1, symmetricMatrixPSD);
					dgAssert(clampedIndex >= index);
				} else {
					count++;
					dgAssert(clampedIndex < size);
					dgPermuteRows(size, clampedIndex, swapIndex, symmetricMatrixPSD, lowerTriangularMatrix, x0, r0, low, high, permute);
					dgCholeskyUpdate(size, clampedIndex, swapIndex, lowerTriangularMatrix, tmp0, tmp1, symmetricMatrixPSD);
					clampedIndex++;
					dgAssert(clampedIndex <= size);
					dgAssert(clampedIndex >= index);
				}

			} else {
				dgAssert(index > 0);
				x0[swapIndex] = clamp_x;
				delta_x[index] = T(0.0f);

				dgAssert(swapIndex < index);
				dgPermuteRows(size, swapIndex, index - 1, symmetricMatrixPSD, lowerTriangularMatrix, x0, r0, low, high, permute);
				dgPermuteRows(size, index - 1, index, symmetricMatrixPSD, lowerTriangularMatrix, x0, r0, low, high, permute);
				dgPermuteRows(size, clampedIndex - 1, index, symmetricMatrixPSD, lowerTriangularMatrix, x0, r0, low, high, permute);
				dgCholeskyUpdate (size, swapIndex, clampedIndex - 1, lowerTriangularMatrix, tmp0, tmp1, symmetricMatrixPSD);

				clampedIndex--;
				index--;
				loop = true;
			}
		}
	}

	for (dgInt32 i = 0; i < size; i++) {
		dgInt32 j = permute[i];
		x[j] = x0[i];
		b[j] = r0[i];
	}
}

/*
// solve a general Linear complementary program (LCP)
// A * x = b + r
// subjected to constraints
// x(i) = low(i),  if r(i) >= 0  
// x(i) = high(i), if r(i) <= 0  
// low(i) <= x(i) <= high(i),  if r(i) == 0  
//
// return true is the system has a solution.
// in return 
// x is the solution,
// r is return in vector b
// note: although the system is called LCP, the solver is far more general than a strict LCP
// to solve a strict LCP, set the following
// low(i) = 0
// high(i) = infinity.
// this the same as enforcing the constraint: x(i) * r(i) = 0
template <class T>
bool dgSolveDantzigLCP(dgInt32 size, T* const symetricMatrix, T* const x, T* const b, T* const low, T* const high)
{
	T* const choleskyMatrix = dgAlloca(T, size * size);
	dgCheckAligment(choleskyMatrix);

	memcpy (choleskyMatrix, symetricMatrix, sizeof (T) * size * size);
	dgCholeskyFactorization(size, choleskyMatrix);
	for (dgInt32 i = 0; i < size; i ++) {
		T* const row = &choleskyMatrix[i * size];
		for (dgInt32 j = i + 1; j < size; j ++) {
			row[j] = T(0.0f);
		}
	}
	return dgSolveDantzigLCP(size, symetricMatrix, choleskyMatrix, x, b, low, high);
}
*/

// solve a general Linear complementary program (LCP)
// A * x = b + r
// subjected to constraints
// x(i) = low(i),  if r(i) >= 0  
// x(i) = high(i), if r(i) <= 0  
// low(i) <= x(i) <= high(i),  if r(i) == 0  
//
// return true is the system has a solution.
// in return 
// x is the solution,
// b is zero
// note: although the system is called LCP, the solver is far more general than a strict LCP
// to solve a strict LCP, set the following
// low(i) = 0
// high(i) = infinity.
// this is the same as enforcing the constraint: x(i) * r(i) = 0
template <class T>
bool dgSolvePartitionDantzigLCP(dgInt32 size, T* const symmetricMatrixPSD , T* const x, T* const b, T* const low, T* const high)
{
	dgInt16* const permute = dgAlloca(dgInt16, size);

	for (dgInt32 i = 0; i < size; i++) {
		x[i] = b[i];
		permute[i] = dgInt16(i);
	}

	dgInt32 unboundedSize = size;
	for (dgInt32 i = 0; i < unboundedSize; i++) {
		if ((low[i] <= T(-DG_LCP_MAX_VALUE)) && (high[i] >= T(DG_LCP_MAX_VALUE))) {
			dgCholeskyFactorizationAddRow(size, i, symmetricMatrixPSD );
		} else {
			dgInt32 j = unboundedSize - 1;
			if (i != j) {
				T* const A = &symmetricMatrixPSD [size * i];
				T* const B = &symmetricMatrixPSD [size * j];
				for (dgInt32 k = 0; k < size; k++) {
					dgSwap(A[k], B[k]);
				}

				dgInt32 stride = 0;
				for (dgInt32 k = 0; k < size; k++) {
					dgSwap(symmetricMatrixPSD [stride + i], symmetricMatrixPSD [stride + j]);
					stride += size;
				}
				dgSwap(x[i], x[j]);
				dgSwap(b[i], b[j]);
				dgSwap(low[i], low[j]);
				dgSwap(high[i], high[j]);
				dgSwap(permute[i], permute[j]);
			}

			i--;
			unboundedSize--;
		}
	}

	bool ret = false;
	if (unboundedSize > 0) {
		dgSolveCholesky(size, unboundedSize, symmetricMatrixPSD , x);
		dgInt32 base = unboundedSize * size;
		for (dgInt32 i = unboundedSize; i < size; i++) {
			b[i] -= dgDotProduct(unboundedSize, &symmetricMatrixPSD[base], x);
			base += size;
		}

		const dgInt32 boundedSize = size - unboundedSize;
		T* const l = dgAlloca(T, boundedSize);
		T* const h = dgAlloca(T, boundedSize);
		T* const c = dgAlloca(T, boundedSize);
		T* const u = dgAlloca(T, boundedSize);
		T* const a11 = dgAlloca(T, boundedSize * boundedSize);
		T* const a10 = dgAlloca(T, boundedSize * unboundedSize);

		for (dgInt32 i = 0; i < boundedSize; i++) {
			T* const g = &a10[i * unboundedSize];
			const T* const row = &symmetricMatrixPSD [(unboundedSize + i) * size];
			for (dgInt32 j = 0; j < unboundedSize; j++) {
				g[j] = -row[j];
			}
			dgSolveCholesky(size, unboundedSize, symmetricMatrixPSD, g);

			T* const arow = &a11[i * boundedSize];
			const T* const row2 = &symmetricMatrixPSD[(unboundedSize + i) * size];
			arow[i] = row2[unboundedSize + i] + dgDotProduct(unboundedSize, g, row2);
			for (dgInt32 j = i + 1; j < boundedSize; j++) {
				const T* const row1 = &symmetricMatrixPSD [(unboundedSize + j) * size];
				T elem = row1[unboundedSize + i] + dgDotProduct(unboundedSize, g, row1);
				arow[j] = elem;
				a11[j * boundedSize + i] = elem;
			}
			u[i] = T(0.0f);
			c[i] = b[i + unboundedSize];
			l[i] = low[i + unboundedSize];
			h[i] = high[i + unboundedSize];
		}

		if (dgSolveDantzigLCP(boundedSize, a11, u, c, l, h)) {
			for (dgInt32 i = 0; i < boundedSize; i++) {
				const T s = u[i];
				x[unboundedSize + i] = s;
				const T* const g = &a10[i * unboundedSize];
				for (dgInt32 j = 0; j < unboundedSize; j++) {
					x[j] += g[j] * s;
				}
			}
			ret = true;
		}
	} else {
		for (dgInt32 i = 0; i < size; i++) {
			x[i] = T(0.0f);
		}
		ret = dgSolveDantzigLCP(size, symmetricMatrixPSD, x, b, low, high);
	}

	for (dgInt32 i = 0; i < size; i++) {
		b[i] = x[i];
	}
	for (dgInt32 i = 0; i < size; i++) {
		dgInt32 j = permute[i];
		x[j] = b[i];
		b[i] = T(0.0f);
	}
	return ret;
}


template <class T>
void dgSolveDantzigLCP(dgInt32 size, T* const symmetricMatrixPSD, T* const x, T* const b, T* const low, T* const high)
{
	T tol2 = T(0.25f * 0.25f);
	dgInt32 passes = dgClamp(size, 12, 20);
	T* const r = dgAlloca(T, size);
	dgInt16* const clipped = dgAlloca(dgInt16, size);

	// find an approximation to the solution
	dgGaussSeidelLcpSor(size, symmetricMatrixPSD, x, b, low, high, tol2, passes, clipped, T(1.3f));

	T err2(0.0f);
	dgInt32 stride = 0;
	dgInt32 clippeCount = 0;
	for (dgInt32 i = 0; i < size; i++) {
		const T* const row = &symmetricMatrixPSD[stride];
		r[i] = b[i] - dgDotProduct(size, row, x);
		clippeCount += clipped[i];
		err2 += clipped[i] ? T(0.0f) : r[i] * r[i];
		stride += size;
	}

	if (err2 > tol2) {
		// check for small lcp
		if ((clippeCount < 16) && ((clippeCount < 32) && (err2 < T(16.0f)))) {
			// small lcp can be solved with direct method
			T* const x0 = dgAlloca(T, size);
			for (dgInt32 i = 0; i < size; i++) {
				low[i] -= x[i];
				high[i] -= x[i];
			}
			dgSolveDantzigLcpLow(size, symmetricMatrixPSD, x0, r, low, high);
			for (dgInt32 i = 0; i < size; i++) {
				x[i] += x0[i];
			}
		} else {
			// larger lcp are too hard for direct method, see if we can get better approximation
			dgGaussSeidelLcpSor(size, symmetricMatrixPSD, x, b, low, high, tol2, 20, clipped, T(1.3f));
		}
	}
}

#endif
