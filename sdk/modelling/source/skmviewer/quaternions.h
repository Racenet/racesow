
//====================================================
//QUATERNIONS

typedef float quat_t[4];

void Quat_Identity( quat_t q );
void Quat_Copy( const quat_t q1, quat_t q2 );
void Quat_Conjugate( const quat_t q1, quat_t q2 );
float Quat_Normalize( quat_t q );
float Quat_Inverse( const quat_t q1, quat_t q2 );
void Matrix_Quat( float m[3][3], quat_t q );
void Quat_Multiply( const quat_t q1, const quat_t q2, quat_t out );
void Quat_Lerp( const quat_t q1, const quat_t q2, float t, quat_t out );
void Quat_Matrix( const quat_t q, float m[3][3] );
void Quat_TransformVector( const quat_t q, const float v[3], float out[3] );
void Quat_ConcatTransforms( const quat_t q1, const float v1[3], const quat_t q2, const float v2[3], quat_t q, float v[3] );
