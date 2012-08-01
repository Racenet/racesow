/**
 * Vec3
 */
class Vec3
{
public:
	/* object properties */
	float x;
	float y;
	float z;

	/* object behaviors */
	void _beh_0_();&s
	void _beh_0_(float, float, float);&s
	void _beh_0_(float);&s
	void _beh_0_(const Vec3&in);&s

	/* object methods */
	Vec3& opAssign(Vec3&in);
	Vec3& opAssign(int);
	Vec3& opAssign(float);
	Vec3& opAddAssign(Vec3&in);
	Vec3& opSubAssign(Vec3&in);
	Vec3& opMulAssign(Vec3&in);
	Vec3& opXorAssign(Vec3&in);
	Vec3& opMulAssign(int);
	Vec3& opMulAssign(float);
	Vec3 opAdd(Vec3&in) const;
	Vec3 opSub(Vec3&in) const;
	float opMul(Vec3&in) const;
	Vec3 opMul(float) const;
	Vec3 opMul_r(float) const;
	Vec3 opMul(int) const;
	Vec3 opMul_r(int) const;
	Vec3 opXor(const Vec3&in) const;
	bool opEquals(const Vec3&in) const;
	void set(float, float, float);
	float length() const;
	float normalize() const;
	float distance(const Vec3&in) const;
	void angleVectors(Vec3&out, Vec3&out, Vec3&out) const;
	Vec3 toAngles() const;
	Vec3 perpendicular() const;
	void makeNormalVectors(Vec3&out, Vec3&out) const;
};

