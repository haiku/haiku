#ifndef _ANGLE_H_
#define _ANGLE_H_

#include <GraphicsDefs.h>

class Angle
{
public:
	Angle(float angle);
	Angle(void);
	virtual ~Angle(void);

	void Normalize(void);

	float Sine(void);
	Angle InvSine(float value);

	float Cosine(void);
	Angle InvCosine(float value);

	float Tangent(int *status=NULL);
	Angle InvTangent(float value);
	
	uint8 Quadrant(void);
	Angle Constrain180(void);
	Angle Constrain90(void);
	
	void SetValue(float angle);
	float Value(void) const;
	Angle &operator=(const Angle &from);
	Angle &operator=(const float &from);
	Angle &operator=(const long &from);
	Angle &operator=(const int &from);
	bool operator==(const Angle &from);
	bool operator!=(const Angle &from);
	bool operator<(const Angle &from);
	bool operator>(const Angle &from);
	bool operator>=(const Angle &from);
	bool operator<=(const Angle &from);

protected:	
	float angle_value;
	void InitTrigTables(void);
};

#endif
