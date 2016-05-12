#pragma once

inline float Normalize(short v, short d)
{
	if (v > d)
		return (float)(v - d) / (SHRT_MAX - d);
	if (v < -d)
		return (float)(v + d) / (SHRT_MAX - d);
	else
		return 0.0f;
}

inline float Power(float Base, int Power)
{
	float Return = 1;
	if (Power > 0)
		for (int n = 0; n < Power; n++)
			Return *= Base;
	else
		for (int n = 0; n > Power; n--)
			Return /= Base;
	return Return;
}

// Lowest Bit Set
inline int LBS(int v)
{
	int c = 0;
	if (v != 0)
	{
		v = (v ^ (v - 1)) >> 1;  // Set v's trailing 0s to 1s and zero rest
		for (c = 0; v; c++)
		{
			v >>= 1;
		}
	}
	return c;
}

inline const wchar_t* _(bool b)
{
	return b ? L"true" : L"false";
}

template <typename T, size_t M, typename U, size_t N>
inline void Copy(T(&a)[M], const U(&b)[N])
{
	for (int i = 0; i < N; ++i)
		a[i] = b[i];
}

template <typename T>
inline bool Equal(const T& a, const T& b)
{
	return memcmp(&a, &b, sizeof(T)) == 0;
}
