/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */


#include "SVGViewView.h"

#include <strings.h>


named_color colors[] = {
	{ "aliceblue",			{ 240, 248, 255, 255 } },
	{ "antiquewhite",		{ 250, 235, 215, 255 } },
	{ "aqua",				{ 0,   255, 255, 255 } },
	{ "aquamarine",			{ 127, 255, 212, 255 } },
	{ "azure",				{ 240, 255, 255, 255 } },
	{ "beige",				{ 245, 245, 220, 255 } },
	{ "bisque",				{ 255, 228, 196, 255 } },
	{ "black",				{ 0,   0,   0,   255 } },
	{ "blanchedalmond",		{ 255, 235, 205, 255 } },
	{ "blue",				{ 0,   0,   255, 255 } },
	{ "blueviolet",			{ 138, 43,  226, 255 } },
	{ "brown",				{ 165, 42,  42,  255 } },
	{ "burlywood",			{ 222, 184, 135, 255 } },
	{ "cadetblue",			{ 95,  158, 160, 255 } },
	{ "chartreuse",			{ 127, 255, 0,   255 } },
	{ "chocolate",			{ 210, 105, 30,  255 } },
	{ "coral",				{ 255, 127, 80,  255 } },
	{ "cornflowerblue",		{ 100, 149, 237, 255 } },
	{ "cornsilk",			{ 255, 248, 220, 255 } },
	{ "crimson",			{ 220, 20,  60,  255 } },
	{ "cyan",				{ 0,   255, 255, 255 } },
	{ "darkblue",			{ 0,   0,   139, 255 } },
	{ "darkcyan",			{ 0,   139, 139, 255 } },
	{ "darkgoldenrod",		{ 184, 134, 11,  255 } },
	{ "darkgray",			{ 169, 169, 169, 255 } },
	{ "darkgreen",			{ 0,   100, 0,   255 } },
	{ "darkgrey",			{ 169, 169, 169, 255 } },
	{ "darkkhaki",			{ 189, 183, 107, 255 } },
	{ "darkmagenta",		{ 139, 0,   139, 255 } },
	{ "darkolivegreen",		{ 85,  107, 47,  255 } },
	{ "darkorange",			{ 255, 140, 0,   255 } },
	{ "darkorchid",			{ 153, 50,  204, 255 } },
	{ "darkred",			{ 139, 0,   0,   255 } },
	{ "darksalmon",			{ 233, 150, 122, 255 } },
	{ "darkseagreen",		{ 143, 188, 143, 255 } },
	{ "darkslateblue",		{ 72,  61,  139, 255 } },
	{ "darkslategray",		{ 47,  79,  79,  255 } },
	{ "darkslategrey",		{ 47,  79,  79,  255 } },
	{ "darkturquoise",		{ 0,   206, 209, 255 } },
	{ "darkviolet",			{ 148, 0,   211, 255 } },
	{ "deeppink",			{ 255, 20,  147, 255 } },
	{ "deepskyblue",		{ 0,   191, 255, 255 } },
	{ "dimgray",			{ 105, 105, 105, 255 } },
	{ "dimgrey",			{ 105, 105, 105, 255 } },
	{ "dodgerblue",			{ 30,  144, 255, 255 } },
	{ "firebrick",			{ 178, 34,  34,  255 } },
	{ "floralwhite",		{ 255, 250, 240, 255 } },
	{ "forestgreen",		{ 34,  139, 34,  255 } },
	{ "fuchsia",			{ 255, 0,   255, 255 } },
	{ "gainsboro",			{ 220, 220, 220, 255 } },
	{ "ghostwhite",			{ 248, 248, 255, 255 } },
	{ "gold",				{ 255, 215, 0,   255 } },
	{ "goldenrod",			{ 218, 165, 32,  255 } },
	{ "gray",				{ 128, 128, 128, 255 } },
	{ "grey",				{ 128, 128, 128, 255 } },
	{ "green",				{ 0,   128, 0,   255 } },
	{ "greenyellow",		{ 173, 255, 47,  255 } },
	{ "honeydew",			{ 240, 255, 240, 255 } },
	{ "hotpink",			{ 255, 105, 180, 255 } },
	{ "indianred",			{ 205, 92,  92,  255 } },
	{ "indigo",				{ 75,  0,   130, 255 } },
	{ "ivory",				{ 255, 255, 240, 255 } },
	{ "khaki",				{ 240, 230, 140, 255 } },
	{ "lavender",			{ 230, 230, 250, 255 } },
	{ "lavenderblush",		{ 255, 240, 245, 255 } },
	{ "lawngreen",			{ 124, 252, 0,   255 } },
	{ "lemonchiffon",		{ 255, 250, 205, 255 } },
	{ "lightblue",			{ 173, 216, 230, 255 } },
	{ "lightcoral",			{ 240, 128, 128, 255 } },
	{ "lightcyan",			{ 224, 255, 255, 255 } },
	{ "lightgoldenrodyellow",{ 250, 250, 210, 255 } },
	{ "lightgray",			{ 211, 211, 211, 255 } },
	{ "lightgreen",			{ 144, 238, 144, 255 } },
	{ "lightgrey",			{ 211, 211, 211, 255 } },
	{ "lightpink",			{ 255, 182, 193, 255 } },
	{ "lightsalmon",		{ 255, 160, 122, 255 } },
	{ "lightseagreen",		{ 32, 178, 170, 255 } },
	{ "lightskyblue",		{ 135, 206, 250, 255 } },
	{ "lightslategray",		{ 119, 136, 153, 255 } },
	{ "lightslategrey",		{ 119, 136, 153, 255 } },
	{ "lightsteelblue",		{ 176, 196, 222, 255 } },
	{ "lightyellow",		{ 255, 255, 224, 255 } },
	{ "lime",				{ 0,   255, 0,   255 } },
	{ "limegreen",			{ 50,  205, 50,  255 } },
	{ "linen",				{ 250, 240, 230, 255 } },
	{ "magenta",			{ 255, 0,   255, 255 } },
	{ "maroon",				{ 128, 0,   0,   255 } },
	{ "mediumaquamarine",	{ 102, 205, 170, 255 } },
	{ "mediumblue",			{ 0,   0,   205, 255 } },
	{ "mediumorchid",		{ 186, 85,  211, 255 } },
	{ "mediumpurple",		{ 147, 112, 219, 255 } },
	{ "mediumseagreen",		{ 60,  179, 113, 255 } },
	{ "mediumslateblue",	{ 123, 104, 238, 255 } },
	{ "mediumspringgreen",	{ 0,   250, 154, 255 } },
	{ "mediumturquoise",	{ 72,  209, 204, 255 } },
	{ "mediumvioletred",	{ 199, 21,  133, 255 } },
	{ "midnightblue",		{ 25,  25,  112, 255 } },
	{ "mintcream",			{ 245, 255, 250, 255 } },
	{ "mistyrose",			{ 255, 228, 225, 255 } },
	{ "moccasin",			{ 255, 228, 181, 255 } },
	{ "navajowhite",		{ 255, 222, 173, 255 } },
	{ "navy",				{ 0,   0,   128, 255 } },
	{ "oldlace",			{ 253, 245, 230, 255 } },
	{ "olive",				{ 128, 128, 0,   255 } },
	{ "olivedrab",			{ 107, 142, 35,  255 } },
	{ "orange",				{ 255, 165, 0,   255 } },
	{ "orangered",			{ 255, 69,  0,   255 } },
	{ "orchid",				{ 218, 112, 214, 255 } },
	{ "palegoldenrod",		{ 238, 232, 170, 255 } },
	{ "palegreen",			{ 152, 251, 152, 255 } },
	{ "paleturquoise",		{ 175, 238, 238, 255 } },
	{ "palevioletred",		{ 219, 112, 147, 255 } },
	{ "papayawhip",			{ 255, 239, 213, 255 } },
	{ "peachpuff",			{ 255, 218, 185, 255 } },
	{ "peru",				{ 205, 133, 63,  255 } },
	{ "pink",				{ 255, 192, 203, 255 } },
	{ "plum",				{ 221, 160, 221, 255 } },
	{ "powderblue",			{ 176, 224, 230, 255 } },
	{ "purple",				{ 128, 0,   128, 255 } },
	{ "red",				{ 255, 0,   0,   255 } },
	{ "rosybrown",			{ 188, 143, 143, 255 } },
	{ "royalblue",			{ 65,  105, 225, 255 } },
	{ "saddlebrown",		{ 139, 69,  19,  255 } },
	{ "salmon",				{ 250, 128, 114, 255 } },
	{ "sandybrown",			{ 244, 164, 96,  255 } },
	{ "seagreen",			{ 46,  139, 87,  255 } },
	{ "seashell",			{ 255, 245, 238, 255 } },
	{ "sienna",				{ 160, 82,  45,  255 } },
	{ "silver",				{ 192, 192, 192, 255 } },
	{ "skyblue",			{ 135, 206, 235, 255 } },
	{ "slateblue",			{ 106, 90,  205, 255 } },
	{ "slategray",			{ 112, 128, 144, 255 } },
	{ "slategrey",			{ 112, 128, 144, 255 } },
	{ "snow",				{ 255, 250, 250, 255 } },
	{ "springgreen",		{ 0,   255, 127, 255 } },
	{ "steelblue",			{ 70,  130, 180, 255 } },
	{ "tan",				{ 210, 180, 140, 255 } },
	{ "teal",				{ 0,   128, 128, 255 } },
	{ "thistle",			{ 216, 191, 216, 255 } },
	{ "tomato",				{ 255, 99,  71,  255 } },
	{ "turquoise",			{ 64,  224, 208, 255 } },
	{ "violet",				{ 238, 130, 238, 255 } },
	{ "wheat",				{ 245, 222, 179, 255 } },
	{ "white",				{ 255, 255, 255, 255 } },
	{ "whitesmoke",			{ 245, 245, 245, 255 } },
	{ "yellow",				{ 255, 255, 0,   255 } },
	{ "yellowgreen",		{ 154, 205, 50,  255 } },
	{ NULL		,			{ 0,   0,	  0, 255 } },
};

// Globals ---------------------------------------------------------------------

// Svg2PictureView class -------------------------------------------------------
Svg2PictureView::Svg2PictureView(BRect frame, const char *filename)
    :   BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
	fFileName(filename),
	fPicture(NULL)
{
	fPicture = new BPicture();
}


Svg2PictureView::~Svg2PictureView()
{
	delete fPicture;
}


void
Svg2PictureView::AttachedToWindow()
{
	BeginPicture(fPicture);

	bool done = false;
	FILE *file = fopen(fFileName.String(), "rb");
	if (file) {
		XML_Parser parser = XML_ParserCreate("UTF-8");
		XML_SetUserData(parser, this);
		XML_SetElementHandler(parser, (XML_StartElementHandler)_StartElement, (XML_EndElementHandler)_EndElement);
		XML_SetCharacterDataHandler(parser, (XML_CharacterDataHandler)_CharacterDataHandler);

		while (!done) {
			char buf[256];
			size_t len = fread(buf, 1, sizeof(buf), file);
			done = len < sizeof(buf);
            		if (!XML_Parse(parser, buf, len, done))
                		break;
        	}

        	XML_ParserFree(parser);
        	fclose(file);
	}
	fPicture = EndPicture();
}


void
Svg2PictureView::Draw(BRect updateRect)
{
	if (fPicture)
		DrawPicture(fPicture);
}


//------------------------------------------------------------------------------
bool Svg2PictureView::HasAttribute(const XML_Char **attributes, const char *name) {
    while (*attributes && strcasecmp(*attributes, name) != 0)
        attributes += 2;

    return (*attributes);
}
//------------------------------------------------------------------------------
float Svg2PictureView::GetFloatAttribute(const XML_Char **attributes, const char *name) {
    while (*attributes && strcasecmp(*attributes, name) != 0)
        attributes += 2;

	if (*attributes)
		return atof(*(attributes + 1));
	else
		return 0;
}
//------------------------------------------------------------------------------
const char *Svg2PictureView::GetStringAttribute(const XML_Char **attributes, const char *name) {
    while (*attributes && strcasecmp(*attributes, name) != 0)
        attributes += 2;

	if (*attributes)
		return *(attributes + 1);
	else
		return NULL;
}
//------------------------------------------------------------------------------
rgb_color Svg2PictureView::GetColorAttribute(const XML_Char **attributes, const char *name, uint8 alpha) {
    const char *attr = GetStringAttribute(attributes, name);

	if (!attr)
		return colors[0].color;

	int32 red, green, blue;

	if (attr[0] == '#') {
		if (strlen(attr) == 4) {
			sscanf(attr, "#%1X%1X%1X", &red, &green, &blue);
			red = (red << 4) + red;
			green = (green << 4) + green;
			blue = (blue << 4) + blue;
		}
		else
			sscanf(attr, "#%2X%2X%2X", &red, &green, &blue);

		rgb_color color;

		color.red = red;
		color.green = green;
		color.blue = blue;
		color.alpha = alpha;

		return color;
	}

	if (sscanf(attr, "rgb(%d, %d, %d)", &red, &green, &blue) == 3) {
		rgb_color color;

		color.red = red;
		color.green = green;
		color.blue = blue;
		color.alpha = alpha;

		return color;
	}

	float redf, greenf, bluef;

	if (sscanf(attr, "rgb(%f%%, %f%%, %f%%)", &redf, &greenf, &bluef) == 3) {
		rgb_color color;

		color.red = (int32)(redf * 2.55f);
		color.green = (int32)(greenf * 2.55f);
		color.blue = (int32)(bluef * 2.55f);
		color.alpha = alpha;

		return color;
	}

	if (strcasecmp(attr, "url")) {
		const char *grad = strchr(attr, '#');

		if (grad) {
			for (int32 i = 0; i < fGradients.CountItems(); i++) {
				named_color *item = (named_color*)fGradients.ItemAt(i);

				if (strstr(grad, item->name)) {
					rgb_color color = item->color;
					color.alpha = alpha;
					return color;
				}
			}
		}
	}

	for (int32 i = 0; colors[i].name != NULL; i++)
		if (strcasecmp(colors[i].name, attr) == 0) {
			rgb_color color = colors[i].color;
			color.alpha = alpha;
			return color;
		}

	rgb_color color = colors[0].color;
	color.alpha = alpha;
	return color;
}
//------------------------------------------------------------------------------
void Svg2PictureView::GetPolygonAttribute(const XML_Char **attributes, const char *name, BShape &shape) {
	const char *attr = NULL;

	while (*attributes && strcasecmp(*attributes, name) != 0)
        attributes += 2;

    if (*attributes)
		attr = *(attributes + 1);

	if (!attr)
		return;

	char *ptr = const_cast<char*>(attr);
	BPoint point;
	bool first = true;

	while (*ptr) {
		// Skip white space and ','
		while (*ptr && (*ptr == ' ') || (*ptr == ','))
			ptr++;

		sscanf(ptr, "%f", &point.x);

		// Skip x
		while (*ptr && *ptr != ',')
			ptr++;
		if (!*ptr || !*(ptr + 1))
			break;
		ptr++;

		sscanf(ptr, "%f", &point.y);

		if (first)
		{
			shape.MoveTo(point);
			first = false;
		}
		else
			shape.LineTo(point);

		// Skip y
		while (*ptr && (*ptr != ' ') && (*ptr != ','))
			ptr++;
	}
}
//------------------------------------------------------------------------------
void Svg2PictureView::GetMatrixAttribute(const XML_Char **attributes, const char *name, BMatrix *matrix) {
	const char *attr = NULL;

	while (*attributes && strcasecmp(*attributes, name) != 0)
        attributes += 2;

    if (*attributes)
		attr = *(attributes + 1);

	if (!attr)
		return;

	char *ptr = (char*)attr;

	while (*ptr) {
		while (*ptr == ' ')
			ptr++;

		char *transform_name = ptr;

		while (*ptr != '(')
			ptr++;

		if (strncmp(transform_name, "translate", 9) == 0) {
			float x, y;

			if (sscanf(ptr, "(%f %f)", &x, &y) != 2)
				sscanf(ptr, "(%f,%f)", &x, &y);

			matrix->Translate(x, y);
		}
		else if (strncmp(transform_name, "rotate", 6) == 0) {
			float angle;

			sscanf(ptr, "(%f)", &angle);

			matrix->Rotate(angle);
		}
		else if (strncmp(transform_name, "scale", 5) == 0) {
			float sx, sy;

			if (sscanf(ptr, "(%f,%f)", &sx, &sy) == 2)
				matrix->Scale(sx, sy);
			else
			{
				sscanf(ptr, "(%f)", &sx);
				matrix->Scale(sx, sx);
			}
		}
		else if (strncmp(transform_name, "skewX", 5) == 0) {
			float angle;

			sscanf(ptr, "(%f)", &angle);

			matrix->SkewX(angle);
		}
		else if (strncmp(transform_name, "skewY", 5) == 0) {
			float angle;

			sscanf(ptr, "(%f)", &angle);

			matrix->SkewY(angle);
		}

		while (*ptr != ')')
			ptr++;

		ptr++;
	}
}
//------------------------------------------------------------------------------
double CalcVectorAngle(double ux, double uy, double vx, double vy) {
	double ta = atan2(uy, ux);
	double tb = atan2(vy, vx);

	if (tb >= ta)
		return tb - ta;

	return 6.28318530718 - (ta - tb);
}
//------------------------------------------------------------------------------
char *SkipFloat(char *string) {
	if (*string == '-')
		string++;

	int32 len = strspn(string, "1234567890.");

	return string + len;
}
//------------------------------------------------------------------------------
char *FindFloat(char *string) {
	return strpbrk(string, "1234567890-.");
}
//------------------------------------------------------------------------------
float GetFloat(char **string) {
	*string = FindFloat(*string);
	float f = atof(*string);
	*string = SkipFloat(*string);
	return f;
}
//------------------------------------------------------------------------------
void Svg2PictureView::GetShapeAttribute(const XML_Char **attributes, const char *name, BShape &shape) {
	const char *attr = GetStringAttribute(attributes, name);

	if (!attr)
		return;

	char *ptr = const_cast<char*>(attr);
	float x, y, x1, y1, x2, y2, rx, ry, angle;
	bool largeArcFlag, sweepFlag;
	char command, prevCommand = 0;
	BPoint prevCtlPt;
	BPoint pos, startPos;
	bool canMove = true;

	while (*ptr) {
		ptr = strpbrk(ptr, "ZzMmLlCcQqAaHhVvSsTt");

		if (ptr == NULL)
			break;

		command = *ptr;

		switch (command) {
			case 'Z':
			case 'z':
			{
				pos.Set(startPos.x, startPos.y);
				canMove = true;
				shape.Close();
				ptr++;
				break;
			}
			case 'M':
			{
				x = GetFloat(&ptr);
				y = GetFloat(&ptr);

				pos.Set(x, y);
				if (canMove)
					startPos = pos;
				shape.MoveTo(pos);
				break;
			}
			case 'm':
			{
				x = GetFloat(&ptr);
				y = GetFloat(&ptr);

				pos.x += x;
				pos.y += y;
				if (canMove)
					startPos = pos;
				shape.MoveTo(pos);
				break;
			}
			case 'L':
			{
				x = GetFloat(&ptr);
				y = GetFloat(&ptr);

				pos.Set(x, y);
				canMove = false;
				shape.LineTo(pos);
				break;
			}
			case 'l':
			{
				x = GetFloat(&ptr);
				y = GetFloat(&ptr);

				pos.x += x;
				pos.y += y;
				canMove = false;
				shape.LineTo(pos);
				break;
			}
			case 'C':
			case 'c':
			{
				if (command == 'C') {
					x1 = GetFloat(&ptr);
					y1 = GetFloat(&ptr);
					x2 = GetFloat(&ptr);
					y2 = GetFloat(&ptr);
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);
				}
				else {
					x1 = GetFloat(&ptr);
					y1 = GetFloat(&ptr);
					x2 = GetFloat(&ptr);
					y2 = GetFloat(&ptr);
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);

					x1 += pos.x;
					y1 += pos.y;
					x2 += pos.x;
					y2 += pos.y;
					x += pos.x;
					y += pos.y;
				}

				BPoint controlPoints[3];

				controlPoints[0].Set(x1, y1);
				controlPoints[1].Set(x2, y2);
				controlPoints[2].Set(x, y);

				pos.Set(x, y);
				prevCtlPt = controlPoints[1];
				canMove = false;
				shape.BezierTo(controlPoints);
				break;
			}
			case 'Q':
			case 'q':
			{
				if (command == 'Q') {
				    x1 = GetFloat(&ptr);
					y1 = GetFloat(&ptr);
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);
				}
				else {
					x1 = GetFloat(&ptr);
					y1 = GetFloat(&ptr);
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);

					x1 += pos.x;
					y1 += pos.y;
					x += pos.x;
					y += pos.y;
				}

				BPoint controlPoints[3];

				controlPoints[0].Set(pos.x + 2.0f / 3.0f * (x1 - pos.x),
					pos.y + 2.0f / 3.0f * (y1 - pos.y));
				controlPoints[1].Set(x1 + 1.0f / 3.0f * (x - x1),
					y1  + 1.0f / 3.0f * (y - y1));
				controlPoints[2].Set(x, y);

				pos.Set(x, y);
				prevCtlPt.Set(x1, y1);
				canMove = false;
				shape.BezierTo(controlPoints);
				break;
			}
			case 'A':
			case 'a':
			{
				x1 = pos.x;
				y1 = pos.y;

				if (command == 'A') {
				    rx = GetFloat(&ptr);
					ry = GetFloat(&ptr);
					angle = GetFloat(&ptr);
					largeArcFlag = GetFloat(&ptr);
					sweepFlag = GetFloat(&ptr);
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);

					x2 = x;
					y2 = y;
				}
				else {
					rx = GetFloat(&ptr);
					ry = GetFloat(&ptr);
					angle = GetFloat(&ptr);
					largeArcFlag = GetFloat(&ptr);
					sweepFlag = GetFloat(&ptr);
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);

					x2 = x + pos.x;
					y2 = y + pos.y;
				}

				const double pi = 3.14159265359;
				const double radPerDeg = pi / 180.0;

				if (x1 == x2 && y1 == y2)
					break;

				if (rx == 0.0f || ry == 0.0f) {
					shape.LineTo(BPoint((float)x2, (float)y2));
					break;
				}

				if (rx < 0.0)
					rx = -rx;

				if (ry < 0.0)
					ry = -ry;

				double sinPhi = sin(angle * radPerDeg);
				double cosPhi = cos(angle * radPerDeg);

				double x1dash =  cosPhi * (x1 - x2) / 2.0 +
					sinPhi * (y1 - y2) / 2.0;
				double y1dash = -sinPhi * (x1 - x2) / 2.0 +
					cosPhi * (y1 - y2) / 2.0;

				double root, numerator = rx * rx * ry * ry - rx * rx * y1dash * y1dash -
					ry * ry * x1dash * x1dash;

				if (numerator < 0.0) {
					double s = (float)sqrt(1.0 - numerator / (rx * rx * ry * ry));

					rx *= s;
					ry *= s;
					root = 0.0;
				}
				else {
					root = (largeArcFlag == sweepFlag ? -1.0 : 1.0) *
						sqrt(numerator /
						(rx * rx * y1dash * y1dash + ry * ry * x1dash * x1dash));
				}

				double cxdash = root * rx * y1dash / ry, cydash = -root * ry * x1dash / rx;

				double cx = cosPhi * cxdash - sinPhi * cydash + (x1 + x2) / 2.0;
				double cy = sinPhi * cxdash + cosPhi * cydash + (y1 + y2) / 2.0;

				double theta1 = CalcVectorAngle(1.0, 0.0, (x1dash - cxdash) / rx,
					(y1dash - cydash) / ry ),
					dtheta = CalcVectorAngle((x1dash - cxdash) / rx,
						(y1dash - cydash) / ry, (-x1dash - cxdash) / rx,
						(-y1dash - cydash) / ry);

				if (!sweepFlag && dtheta > 0)
					dtheta -= 2.0 * pi;
				else if (sweepFlag && dtheta < 0)
					dtheta += 2.0 * pi;

				int segments = (int)ceil (fabs(dtheta / (pi / 2.0)));
				double delta = dtheta / segments;
				double t = 8.0/3.0 * sin(delta / 4.0) * sin( delta / 4.0) /
					sin(delta / 2.0);

				BPoint controlPoints[3];

				for (int n = 0; n < segments; ++n) {
					double cosTheta1 = cos(theta1);
					double sinTheta1 = sin(theta1);
					double theta2 = theta1 + delta;
					double cosTheta2 = cos(theta2);
					double sinTheta2 = sin(theta2);

					double xe = cosPhi * rx * cosTheta2 - sinPhi * ry * sinTheta2 + cx;
					double ye = sinPhi * rx * cosTheta2 + cosPhi * ry * sinTheta2 + cy;

					double dx1 = t * (-cosPhi * rx * sinTheta1 - sinPhi * ry * cosTheta1);
					double dy1 = t * (-sinPhi * rx * sinTheta1 + cosPhi * ry * cosTheta1);

					double dxe = t * (cosPhi * rx * sinTheta2 + sinPhi * ry * cosTheta2);
					double dye = t * (sinPhi * rx * sinTheta2 - cosPhi * ry * cosTheta2);

					controlPoints[0].Set((float)(x1 + dx1), (float)(y1 + dy1));
					controlPoints[1].Set((float)(xe + dxe), (float)(ye + dye));
					controlPoints[2].Set((float)xe, (float)ye );

					shape.BezierTo(controlPoints);

					theta1 = theta2;
					x1 = (float)xe;
					y1 = (float)ye;
				}

				pos.Set(x2, y2);
				break;
			}
			case 'H':
			{
				x = GetFloat(&ptr);

				pos.x = x;
				canMove = false;
				shape.LineTo(pos);
				break;
			}
			case 'h':
			{
				x = GetFloat(&ptr);

				pos.x += x;
				canMove = false;
				shape.LineTo(pos);
				break;
			}
			case 'V':
			{
				y = GetFloat(&ptr);

				pos.y = y;
				canMove = false;
				shape.LineTo(pos);
				break;
			}
			case 'v':
			{
				y = GetFloat(&ptr);

				pos.y += y;
				canMove = false;
				shape.LineTo(pos);
				break;
			}
			case 'S':
			case 's':
			{
				if (command == 'S') {
					x2 = GetFloat(&ptr);
					y2 = GetFloat(&ptr);
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);
				}
				else {
					x2 = GetFloat(&ptr);
					y2 = GetFloat(&ptr);
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);

					x2 += pos.x;
					y2 += pos.y;
					x += pos.x;
					y += pos.y;
				}

				if (prevCommand == 'C' || prevCommand == 'c' ||
					prevCommand == 'S' || prevCommand == 's') {
					x1 = prevCtlPt.x  + 2 * (pos.x - prevCtlPt.x);
					y1 = prevCtlPt.y  + 2 * (pos.y - prevCtlPt.y);
				}
				else {
					x1 = pos.x;
					y1 = pos.y;
				}

				BPoint controlPoints[3];

				controlPoints[0].Set(x1, y1);
				controlPoints[1].Set(x2, y2);
				controlPoints[2].Set(x, y);

				pos.Set(x, y);
				prevCtlPt.Set(x2, y2);
				canMove = false;
				shape.BezierTo(controlPoints);
				break;
			}
			case 'T':
			case 't':
			{
				if (command == 'T') {
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);
				}
				else {
					x = GetFloat(&ptr);
					y = GetFloat(&ptr);

					x += pos.x;
					y += pos.y;
				}

				if (prevCommand == 'Q' || prevCommand == 'q' ||
					prevCommand == 'T' || prevCommand == 't') {
					x1 = prevCtlPt.x  + 2 * (pos.x - prevCtlPt.x);
					y1 = prevCtlPt.y  + 2 * (pos.y - prevCtlPt.y);
				}
				else {
					x1 = pos.x;
					y1 = pos.y;
				}

				BPoint controlPoints[3];

				controlPoints[0].Set(pos.x + 2.0f / 3.0f * (x1 - pos.x),
					pos.y + 2.0f / 3.0f * (y1 - pos.y));
				controlPoints[1].Set(x1 + 1.0f / 3.0f * (x - x1),
					y1  + 1.0f / 3.0f * (y - y1));
				controlPoints[2].Set(x, y);

				pos.Set(x, y);
				prevCtlPt.Set(x1, y1);
				canMove = false;
				shape.BezierTo(controlPoints);
				break;
			}
		}

		prevCommand = command;
	}
}
//------------------------------------------------------------------------------
void Svg2PictureView::CheckAttributes(const XML_Char **attributes) {
	uint8 alpha = fState.fStrokeColor.alpha;

	if (HasAttribute(attributes, "opacity")) {
	    float opacity = GetFloatAttribute(attributes, "opacity");
		fState.fStrokeColor.alpha = (uint8)(opacity * alpha);
		fState.fFlags |= STROKE_FLAG;
		fState.fFillColor.alpha = (uint8)(opacity * alpha);
		fState.fFlags |= FILL_FLAG;
	}

	if (HasAttribute(attributes, "color")) {
        fState.fCurrentColor = GetColorAttribute(attributes, "color", fState.fCurrentColor.alpha);
	}

	if (HasAttribute(attributes, "stroke")) {
	    const char *stroke = GetStringAttribute(attributes, "stroke");
		if (strcasecmp(stroke, "none") == 0)
			fState.fStroke = false;
        else if (strcasecmp(stroke, "currentColor") == 0) {
            fState.fStrokeColor = fState.fCurrentColor;
			fState.fStroke = true;
        }
		else {
			fState.fStrokeColor = GetColorAttribute(attributes, "stroke", fState.fFillColor.alpha);
			fState.fStroke = true;
			SetHighColor(fState.fStrokeColor);
		}
		fState.fFlags |= STROKE_FLAG;
	}

	if (HasAttribute(attributes, "stroke-opacity")) {
		fState.fStrokeColor.alpha = (uint8)(GetFloatAttribute(attributes, "stroke-opacity") * alpha);
		fState.fFlags |= STROKE_FLAG;
	}

	if (HasAttribute(attributes, "fill")) {
	    const char *fill = GetStringAttribute(attributes, "fill");
		if (strcasecmp(fill, "none") == 0)
			fState.fFill = false;
		else if (strcasecmp(fill, "currentColor") == 0) {
		    fState.fFillColor = fState.fCurrentColor;
			fState.fFill = true;
		}
		else {
			fState.fFillColor = GetColorAttribute(attributes, "fill", fState.fFillColor.alpha);
			fState.fFill = true;
		}
		fState.fFlags |= FILL_FLAG;
	}

	if (HasAttribute(attributes, "fill-opacity")) {
		fState.fFillColor.alpha = (uint8)(GetFloatAttribute(attributes, "fill-opacity") * alpha);
		fState.fFlags |= FILL_FLAG;
	}

	if (HasAttribute(attributes, "stroke-width")) {
		fState.fStrokeWidth = GetFloatAttribute(attributes, "stroke-width");
		SetPenSize(fState.fStrokeWidth);
		fState.fFlags |= STROKE_WIDTH_FLAG;
	}

	if (HasAttribute(attributes, "stroke-linecap")) {
	    const char *stroke_linecap = GetStringAttribute(attributes, "stroke-linecap");

		if (strcasecmp(stroke_linecap, "but") == 0)
			fState.fLineCap = B_BUTT_CAP;
		else if (strcasecmp(stroke_linecap, "round") == 0)
			fState.fLineCap = B_ROUND_CAP;
		else if (strcasecmp(stroke_linecap, "square") == 0)
			fState.fLineCap = B_SQUARE_CAP;

		SetLineMode(fState.fLineCap, LineJoinMode(), LineMiterLimit());
		fState.fFlags |= LINE_MODE_FLAG;
	}

	if (HasAttribute(attributes, "stroke-linejoin")) {
	    const char *stroke_linejoin = GetStringAttribute(attributes, "stroke-linejoin");

		if (strcasecmp(stroke_linejoin, "miter") == 0)
			fState.fLineJoin = B_MITER_JOIN;
		else if (strcasecmp(stroke_linejoin, "round") == 0)
			fState.fLineJoin = B_ROUND_JOIN;
		else if (strcasecmp(stroke_linejoin, "bevel") == 0)
			fState.fLineJoin = B_BEVEL_JOIN;

		SetLineMode(LineCapMode(), fState.fLineJoin, LineMiterLimit());
		fState.fFlags |= LINE_MODE_FLAG;
	}

	if (HasAttribute(attributes, "stroke-miterlimit")) {
		fState.fLineMiterLimit = GetFloatAttribute(attributes, "stroke-miterlimit");
		SetLineMode(LineCapMode(), LineJoinMode(), fState.fLineMiterLimit);
		fState.fFlags |= LINE_MODE_FLAG;
	}

	if (HasAttribute(attributes, "font-size")) {
		fState.fFontSize = GetFloatAttribute(attributes, "font-size");
		SetFontSize(fState.fFontSize);
		fState.fFlags |= FONT_SIZE_FLAG;
	}

	if (HasAttribute(attributes, "transform")) {
		BMatrix matrix;
		GetMatrixAttribute(attributes, "transform", &matrix);
		fState.fMatrix *= matrix;
		fState.fFlags |= MATRIX_FLAG;
	}
}
//------------------------------------------------------------------------------
void Svg2PictureView::StartElement(const XML_Char *name, const XML_Char **attributes) {
    Push();
    CheckAttributes(attributes);

    if (strcasecmp(name, "circle") == 0) {
        BPoint c(GetFloatAttribute(attributes, "cx"), GetFloatAttribute(attributes, "cy"));
        float r = GetFloatAttribute(attributes, "r");

        if (fState.fFill) {
            SetHighColor(fState.fFillColor);
            FillEllipse(c, r, r);
            SetHighColor(fState.fStrokeColor);
        }
        if (fState.fStroke)
            StrokeEllipse(c, r, r);
    }
    else if (strcasecmp(name, "ellipse") == 0) {
        BPoint c(GetFloatAttribute(attributes, "cx"), GetFloatAttribute(attributes, "cy"));
        float rx = GetFloatAttribute(attributes, "rx");
        float ry = GetFloatAttribute(attributes, "ry");

        if (fState.fFill) {
            SetHighColor(fState.fFillColor);
            FillEllipse(c, rx, ry);
            SetHighColor(fState.fStrokeColor);
        }
        if (fState.fStroke)
            StrokeEllipse(c, rx, ry);
    }
    else if (strcasecmp(name, "image") == 0) {
        BPoint topLeft(GetFloatAttribute(attributes, "x"), GetFloatAttribute(attributes, "y"));
        BPoint bottomRight(topLeft.x + GetFloatAttribute(attributes, "width"),
            topLeft.y + GetFloatAttribute(attributes, "height"));

        fState.fMatrix.Transform(&topLeft);
        fState.fMatrix.Transform(&bottomRight);

        const char *href = GetStringAttribute(attributes, "xlink:href");

        if (href) {
            BBitmap *bitmap = BTranslationUtils::GetBitmap(href);

            if (bitmap) {
                DrawBitmap(bitmap, BRect(topLeft, bottomRight));
                delete bitmap;
            }
        }
    }
    else if (strcasecmp(name, "line") == 0){
        BPoint from(GetFloatAttribute(attributes, "x1"), GetFloatAttribute(attributes, "y1"));
        BPoint to(GetFloatAttribute(attributes, "x2"), GetFloatAttribute(attributes, "y2"));

        fState.fMatrix.Transform(&from);
        fState.fMatrix.Transform(&to);

        StrokeLine(from, to);
    }
    else if (strcasecmp(name, "linearGradient") == 0) {
        fGradient = new named_gradient;

        fGradient->name = strdup(GetStringAttribute(attributes, "id"));
        fGradient->color.red = 0;
        fGradient->color.green = 0;
        fGradient->color.blue = 0;
        fGradient->color.alpha = 255;
        fGradient->started = false;
    }
    else if (strcasecmp(name, "path") == 0) {
        BShape shape;
        GetShapeAttribute(attributes, "d", shape);
        fState.fMatrix.Transform(shape);

        if (fState.fFill) {
            SetHighColor(fState.fFillColor);
            FillShape(&shape);
            SetHighColor(fState.fStrokeColor);
        }
        if (fState.fStroke)
            StrokeShape(&shape);
    }
    else if (strcasecmp(name, "polygon") == 0) {
        BShape shape;
        GetPolygonAttribute(attributes, "points", shape);
        shape.Close();
        fState.fMatrix.Transform(shape);

        if (fState.fFill) {
            SetHighColor(fState.fFillColor);
            FillShape(&shape);
            SetHighColor(fState.fStrokeColor);
        }
        if (fState.fStroke)
            StrokeShape(&shape);
    }
    else if (strcasecmp(name, "polyline") == 0) {
        BShape shape;
        GetPolygonAttribute(attributes, "points", shape);
        fState.fMatrix.Transform(shape);

        if (fState.fFill) {
            SetHighColor(fState.fFillColor);
            FillShape(&shape);
            SetHighColor(fState.fStrokeColor);
        }
        if (fState.fStroke)
            StrokeShape(&shape);
    }
    else if (strcasecmp(name, "radialGradient") == 0) {
        fGradient = new named_gradient;

        fGradient->name = strdup(GetStringAttribute(attributes, "id"));
        fGradient->color.red = 0;
        fGradient->color.green = 0;
        fGradient->color.blue = 0;
        fGradient->color.alpha = 255;
        fGradient->started = false;
    }
    else if (strcasecmp(name, "stop") == 0) {
        rgb_color color = GetColorAttribute(attributes, "stop-color", 255);

        if (fGradient) {
            if (fGradient->started) {
                fGradient->color.red = (int8)(((int32)fGradient->color.red + (int32)color.red) / 2);
                fGradient->color.green = (int8)(((int32)fGradient->color.green + (int32)color.green) / 2);
                fGradient->color.blue = (int8)(((int32)fGradient->color.blue + (int32)color.blue) / 2);
            }
            else {
                fGradient->color = color;
                fGradient->started = true;
            }
        }
    }
    else if (strcasecmp(name, "rect") == 0) {
        BPoint points[4];

        points[0].x = points[3].x = GetFloatAttribute(attributes, "x");
        points[0].y= points[1].y = GetFloatAttribute(attributes, "y");
        points[1].x = points[2].x = points[0].x + GetFloatAttribute(attributes, "width");
        points[2].y = points[3].y = points[0].y + GetFloatAttribute(attributes, "height");

        /*const char *_rx = element->Attribute("rx");
        const char *_ry = element->Attribute("ry");

        if (_rx || _ry)
        {
            float rx, ry;

            if (_rx)
            {
                rx = atof(_rx);

                if (_ry)
                    ry = atof(_ry);
                else
                    ry = rx;
            }
            else
                rx = ry = atof(_ry);

            if (fState.fFill)
            {
                SetHighColor(fState.fFillColor);
                FillRoundRect(rect, rx, ry);
                SetHighColor(fState.fStrokeColor);
            }
            if (fState.fStroke)
                StrokeRoundRect(rect, rx, ry);
        }
        else
        {
            if (fState.fFill)
            {
                SetHighColor(fState.fFillColor);
                FillRect(rect);
                SetHighColor(fState.fStrokeColor);
            }
            if (fState.fStroke)
                StrokeRect(rect);
        }*/

        BShape shape;

        shape.MoveTo(points[0]);
        shape.LineTo(points[1]);
        shape.LineTo(points[2]);
        shape.LineTo(points[3]);
        shape.Close();

        fState.fMatrix.Transform(shape);

        if (fState.fFill)
        {
            SetHighColor(fState.fFillColor);
            FillShape(&shape);
            SetHighColor(fState.fStrokeColor);
        }
        if (fState.fStroke)
            StrokeShape(&shape);
    }
    else if (strcasecmp(name, "text") == 0) {
        fTextPosition.Set(GetFloatAttribute(attributes, "x"), GetFloatAttribute(attributes, "y"));
        fState.fMatrix.Transform(&fTextPosition);
    }
}
//------------------------------------------------------------------------------
void Svg2PictureView::EndElement(const XML_Char *name) {
    if (strcasecmp(name, "linearGradient") == 0) {
        if (fGradient)
            fGradients.AddItem(fGradient);
        fGradient = NULL;
    }
    else if (strcasecmp(name, "radialGradient") == 0) {
        if (fGradient)
            fGradients.AddItem(fGradient);
        fGradient = NULL;
    }
    else if (strcasecmp(name, "text") == 0) {
        if (fState.fFill)
        {
            SetHighColor(fState.fFillColor);
            DrawString(fText.String(), fTextPosition);
            SetHighColor(fState.fStrokeColor);
        }
        if (fState.fStroke)
            DrawString(fText.String(), fTextPosition);
        printf("%f, %f\n", fTextPosition.x, fTextPosition.y);
    }

    Pop();
}
//------------------------------------------------------------------------------
void Svg2PictureView::CharacterDataHandler(const XML_Char *s, int len) {
    fText.SetTo(s, len);
}
//------------------------------------------------------------------------------
void Svg2PictureView::Push() {
	_state_ *state = new _state_(fState);

	fStack.AddItem(state);
}
//------------------------------------------------------------------------------
void Svg2PictureView::Pop() {
	if (fStack.CountItems() == 0)
		printf("Unbalanced Push/Pop\n");

	_state_ *state = (_state_*)fStack.LastItem();

	if (fState.fFlags & STROKE_FLAG)
	{
		if (state->fStroke)
			SetHighColor(state->fStrokeColor);
	}

	if (fState.fFlags & FILL_FLAG)
	{
		if (state->fFill)
			SetHighColor(state->fFillColor);
	}

	if (fState.fFlags & STROKE_WIDTH_FLAG)
		SetPenSize(state->fStrokeWidth);

	if (fState.fFlags & LINE_MODE_FLAG)
		SetLineMode(state->fLineCap, state->fLineJoin, state->fLineMiterLimit);

	if (fState.fFlags & FONT_SIZE_FLAG)
		SetFontSize(state->fFontSize);

	fState = *state;

	fStack.RemoveItem(state);
	delete state;
}
//------------------------------------------------------------------------------
void Svg2PictureView::_StartElement(Svg2PictureView *view, const XML_Char *name, const XML_Char **attributes) {
    view->StartElement(name, attributes);
}
//------------------------------------------------------------------------------
void Svg2PictureView::_EndElement(Svg2PictureView *view, const XML_Char *name) {
    view->EndElement(name);
}
//------------------------------------------------------------------------------
void Svg2PictureView::_CharacterDataHandler(Svg2PictureView *view, const XML_Char *s, int len) {
    view->CharacterDataHandler(s, len);
}
//------------------------------------------------------------------------------
