#pragma once

struct st_Header
{
	__int32		type;
	__int32		size;
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

	void Zero()
	{
		X = 0;
		Y = 0;
		Z = 0;
	}
};