/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Just drag'n'drop media files to the 3D objects
*/

#include <GL/gl.h>
#include <string.h>

#include "GLUtility.h"

//	Local definitions

// 	Local functions

//	Local variables
	
/*	FUNCTION:		GLCreateIdentityMatrix
	ARGUMENTS:		m			destination matrix
	RETURN:			n/a
	DESCRIPTION:	Create identity matrix
*/
void GLCreateIdentityMatrix(float *m)
{
	const float kIdentity[16] = 
	{
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1,
	};
	memcpy(m, kIdentity, 16*sizeof(float));
}

/*	FUNCTION:		GLMatrixMultiply
	ARGUMENTS:		product
					m1, m2
	RETURN:			product
	DESCRIPTION:	Perform a 4x4 matrix multiplication  (product = m1 x m2)
*/
void GLMatrixMultiply(float *product, float *m1, float *m2)
{
	product[0] = m1[0]*m2[0] + m1[4]*m2[1] + m1[8]*m2[2] + m1[12]*m2[3];
	product[1] = m1[1]*m2[0] + m1[5]*m2[1] + m1[9]*m2[2] + m1[13]*m2[3];
	product[2] = m1[2]*m2[0] + m1[6]*m2[1] + m1[10]*m2[2] + m1[14]*m2[3];
	product[3] = m1[3]*m2[0] + m1[7]*m2[1] + m1[11]*m2[2] + m1[15]*m2[3];

	product[4] = m1[0]*m2[4] + m1[4]*m2[5] + m1[8]*m2[6] + m1[12]*m2[7];
	product[5] = m1[1]*m2[4] + m1[5]*m2[5] + m1[9]*m2[6] + m1[13]*m2[7];
	product[6] = m1[2]*m2[4] + m1[6]*m2[5] + m1[10]*m2[6] + m1[14]*m2[7];
	product[7] = m1[3]*m2[4] + m1[7]*m2[5] + m1[11]*m2[6] + m1[15]*m2[7];

	product[8] = m1[0]*m2[8] + m1[4]*m2[9] + m1[8]*m2[10] + m1[12]*m2[11];
	product[9] = m1[1]*m2[8] + m1[5]*m2[9] + m1[9]*m2[10] + m1[13]*m2[11];
	product[10] = m1[2]*m2[8] + m1[6]*m2[9] + m1[10]*m2[10] + m1[14]*m2[11];
	product[11] = m1[3]*m2[8] + m1[7]*m2[9] + m1[11]*m2[10] + m1[15]*m2[11];

	product[12] = m1[0]*m2[12] + m1[4]*m2[13] + m1[8]*m2[14] + m1[12]*m2[15];
	product[13] = m1[1]*m2[12] + m1[5]*m2[13] + m1[9]*m2[14] + m1[13]*m2[15];
	product[14] = m1[2]*m2[12] + m1[6]*m2[13] + m1[10]*m2[14] + m1[14]*m2[15];
	product[15] = m1[3]*m2[12] + m1[7]*m2[13] + m1[11]*m2[14] + m1[15]*m2[15];
}

/*	FUNCTION:		GLCreatePerspectiveMatrix
	ARGUMENTS:		m			destination matrix
					fov			field of view
					aspect		aspect ratio
					znear		near plane
					zfar		far plane
	RETURN:			n/a
	DESCRIPTION:	Same as gluPerspective
*/
void GLCreatePerspectiveMatrix(float *m, float fov, float aspect, float znear, float zfar)
{
	const float h = 1.0f/tand(fov/2.0f);
	float neg_depth = znear-zfar;
	
	m[0] = h / aspect;
	m[1] = 0;
	m[2] = 0;
	m[3] = 0;

	m[4] = 0;
	m[5] = h;
	m[6] = 0;
	m[7] = 0;

	m[8] = 0;
	m[9] = 0;
	m[10] = (zfar + znear)/neg_depth;
	m[11] = -1;

	m[12] = 0;
	m[13] = 0;
	m[14] = 2.0f*(znear*zfar)/neg_depth;
	m[15] = 0;
}

/*	FUNCTION:		GLCreateModelViewMatrix
	ARGUMENTS:		m			destination matrix
					x,y,z		position
					yaw			compass direction, 0-north, 90-east, 180-south, 270-west
					pitch		azimuth, -90 down, 0 forwad, 90 up
					roll		rotation around forward axis
	RETURN:			n/a
	DESCRIPTION:	Position camera
*/
void GLCreateModelViewMatrix(float *m, float x, float y, float z, float yaw, float pitch, float roll)
{
	//	same as gluLookAt
	Quaternion q, qDir, qAzim;
	qAzim.GenerateLocalRotation(pitch + 90.0f, -1.0f, 0.0f, 0.0f);
	qDir.GenerateLocalRotation(yaw, 0.0f, 0.0f, 1.0f);
	q = qAzim * qDir;
	
	if (roll != 0)
	{
		Quaternion qTilt;
		qTilt.GenerateLocalRotation(roll, 0, 0, 1);
		q = qTilt * q;
	}
	q.CreateRotatedQuaternion(m);

	//	move camera
	float mov[16];
	GLCreateIdentityMatrix(mov);
	mov[12] = -x;
	mov[13] = -y;
	mov[14] = -z;
	GLMatrixMultiply(m, m, mov);
}
	
/**************************
	Quaternion class
***************************/

/*	FUNCTION:		Quaternion
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Constructor.  No rotation.
*/
Quaternion :: Quaternion()
{
    x = y = z = 0.0f;
    w = 1.0f;
}

/*	FUNCTION:		Quaternion :: GenerateLocalRotation
	ARGUMENTS:		angle
					x_axis, y_axis, z_axis		axis of rotation
	RETURN:			n/a
	DESCRIPTION:	Create unit quaterion corresponding to a rotation through angle <angle>
					about axis <a,b,c>.
						q = cos (angle/2) + A(a,b,c)*sin(angle/2)
					
*/
void Quaternion :: GenerateLocalRotation(float angle, float x_axis, float y_axis, float z_axis)
{
	float temp = sind(0.5f*angle);
		
	w = cosd(0.5f*angle);
	x = x_axis * temp;
	y = y_axis * temp;
	z = z_axis * temp;
}

/*	FUNCTION:		Quaternion :: GenerateFromEuler
	ARGUMENTS:		roll (degrees)
					pitch (degrees)
					yaw (degrees)
	RETURN:			n/a
	DESCRIPTION:	Creates quaternion from euler angles.  Borowed from Bullet
*/
void Quaternion :: GenerateFromEuler(float roll, float pitch, float yaw)
{
	float halfYaw = 0.5f * yaw;
	float halfPitch = 0.5f * pitch;
	float halfRoll = 0.5f * roll;
	float cosYaw = cosd(halfYaw);
	float sinYaw = sind(halfYaw);
	float cosPitch = cosd(halfPitch);
	float sinPitch = sind(halfPitch);
	float cosRoll = cosd(halfRoll);
	float sinRoll = sind(halfRoll);

	x = sinRoll*cosPitch*cosYaw - cosRoll*sinPitch*sinYaw;
	y = cosRoll*sinPitch*cosYaw + sinRoll*cosPitch*sinYaw;
	z = cosRoll*cosPitch*sinYaw - sinRoll*sinPitch*cosYaw;
	w = cosRoll*cosPitch*cosYaw + sinRoll*sinPitch*sinYaw;
}

/*	FUNCTION:		Quaternion :: CreateRotatedQuaternion
	ARGUMENTS:		matrix
	RETURN:			n/a
	DESCRIPTION:	Creates rotated quaterion. Always call GenerateLocalRotation() prior
					to calling this method
*/
void Quaternion :: CreateRotatedQuaternion(float *matrix)
{
	if (!matrix)
		return;

	// first row	
	matrix[0] = 1.0f - 2.0f*(y*y + z*z); 
    matrix[1] = 2.0f*(x*y + w*z);
    matrix[2] = 2.0f*(x*z - w*y);
	matrix[3] = 0.0f;  

	// second row	
	matrix[4] = 2.0f*(x*y - w*z);  
	matrix[5] = 1.0f - 2.0f*(x*x + z*z); 
	matrix[6] = 2.0f*(y*z + w*x);  
	matrix[7] = 0.0f;  

	// third row
	matrix[8] = 2.0f*(x*z + w*y);
	matrix[9] = 2.0f*(y*z - w*x);
	matrix[10] = 1.0f - 2.0f*(x*x + y*y);  
	matrix[11] = 0.0f;  

	// forth row
	matrix[12] = 0;  
	matrix[13] = 0;  
	matrix[14] = 0;  
	matrix[15] = 1.0f;
}

/*	FUNCTION:		operator *
	ARGUMENTS:		q
	RETURN:			multiplied quaternion
	DESCRIPTION:	Creates rotated quaternion
*/
const Quaternion Quaternion::operator *(Quaternion q)
{
	Quaternion p;

	p.x = w*q.x + x*q.w + y*q.z - z*q.y;	// (w1x2 + x1w2 + y1z2 - z1y2)i
	p.y = w*q.y - x*q.z + y*q.w + z*q.x;	// (w1y2 - x1z2 + y1w2 + z1x2)j
	p.z = w*q.z + x*q.y - y*q.x + z*q.w;	// (w1z2 + x1y2 - y1x2 + z1w2)k
	p.w = w*q.w - x*q.x - y*q.y - z*q.z;	// (w1w2 - x1x2 - y1y2 - z1z2)
	
	return(p);
}
