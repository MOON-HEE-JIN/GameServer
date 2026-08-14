#pragma once
#include <math.h>

struct st_Header
{
	int type;
	int size;
};

struct st_Vector3F
{
	float X;
	float Y;
	float Z;

	st_Vector3F() : X(0), Y(0), Z(0) {}
	st_Vector3F(float x, float y, float z) : X(x), Y(y), Z(z) {}
	st_Vector3F operator+(const st_Vector3F& other) const
	{
		return st_Vector3F(X + other.X, Y + other.Y, Z + other.Z);
	}

	st_Vector3F& operator+=(const st_Vector3F& other)
	{
		X += other.X;
		Y += other.Y;
		Z += other.Z;
		return *this;
	}

	st_Vector3F operator-(const st_Vector3F& other) const
	{
		return st_Vector3F(X - other.X, Y - other.Y, Z - other.Z);
	}
	
	st_Vector3F& operator-=(const st_Vector3F& other)
	{
		X -= other.X;
		Y -= other.Y;
		Z -= other.Z;
		return *this;
	}

	st_Vector3F operator*(float scalar) const
	{
		return st_Vector3F(X * scalar, Y * scalar, Z * scalar);
	}

	float Length() const
	{
		return static_cast<float>(sqrt(X * X + Y * Y + Z * Z));
	}

	float DistanceToSquared(const st_Vector3F& other) const
	{
		return (*this - other).Length();
	}

	float DistanceToNSquared(const st_Vector3F& other) const
	{
		st_Vector3F diff = *this - other;
		return diff.X * diff.X + diff.Y * diff.Y + diff.Z * diff.Z;
	}

	st_Vector3F Direction(const st_Vector3F& target) const
	{
		st_Vector3F dir = target - *this;
		float len = dir.Length();
		if (len > 0)
		{
			return st_Vector3F(dir.X / len, dir.Y / len, dir.Z / len);
		}
		return st_Vector3F(0, 0, 0);
	}

	st_Vector3F& Normalization()
	{
		float len = Length();
		if (len > 0)
		{
			X /= len;
			Y /= len;
			Z /= len;
		}
		return *this;
	}

	bool operator==(const st_Vector3F& other) const
	{
		return X == other.X && Y == other.Y && Z == other.Z;
	}

	bool operator!=(const st_Vector3F& other) const
	{
		return !(*this == other);
	}

	void Zero()
	{
		X = 0; Y = 0; Z = 0;
	}
};

struct st_Vector3L
{
	double X;
	double Y;
	double Z;
};