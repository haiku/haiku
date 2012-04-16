/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <DataIO.h>
#include <File.h>
#include <Locale.h>
#include <String.h>

#include "support.h"

#include "Icon.h"
#include "GradientTransformable.h"
#include "Shape.h"
#include "StrokeTransformer.h"
#include "Style.h"
#include "VectorPath.h"

#include "SVGExporter.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-SVGExport"


// write_line
static status_t
write_line(BPositionIO* stream, BString& string)
{
	ssize_t written = stream->Write(string.String(), string.Length());
	if (written > B_OK && written < string.Length())
		written = B_ERROR;
	string.SetTo("");
	return written;
}

// #pragma mark -

// constructor
SVGExporter::SVGExporter()
	: Exporter(),
	  fGradientCount(0),
	  fOriginalEntry(NULL)
{
}

// destructor
SVGExporter::~SVGExporter()
{
	delete fOriginalEntry;
}

// Export
status_t
SVGExporter::Export(const Icon* icon, BPositionIO* stream)
{
	if (!icon || !stream)
		return B_BAD_VALUE;

//	if (fOriginalEntry && *fOriginalEntry == *refToFinalFile) {
//		if (!_DisplayWarning()) {
//			return B_CANCELED;
//		} else {
//			delete fOriginalEntry;
//			fOriginalEntry = NULL;
//		}
//	}

	BString helper;

	fGradientCount = 0;

	// header
	helper << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
	status_t ret = write_line(stream, helper);

	// image size
	if (ret >= B_OK) {
		helper << "<svg version=\"1.1\" width=\"" << 64 << "\""
			   << " height=\"" << 64 << "\""
			   << " color-interpolation=\"linearRGB\"\n"
			   << "     xmlns:svg=\"http://www.w3.org/2000/svg\""
// Should be needed when exporting inline images:
//			   << " xmlns:xlink=\"http://www.w3.org/1999/xlink\""
			   << " xmlns=\"http://www.w3.org/2000/svg\">\n";
		ret = write_line(stream, helper);
	}

	if (ret >= B_OK) {
		// start (the only) layer
		helper << " <g>\n";
		ret = write_line(stream, helper);

		// export all shapes
		if (ret >= B_OK) {
			int32 count = icon->Shapes()->CountShapes();
			for (int32 i = 0; i < count; i++) {
				Shape* shape = icon->Shapes()->ShapeAtFast(i);
				ret = _ExportShape(shape, stream);
				if (ret < B_OK)
					break;
			}
		}

		// finish (the only) layer
		if (ret >= B_OK) {
			helper << " </g>\n";
			ret = write_line(stream, helper);
		}
	}

	// footer
	if (ret >= B_OK) {
		helper << "</svg>\n";
		ret = write_line(stream, helper);
	}
	return ret;
}

// MIMEType
const char*
SVGExporter::MIMEType()
{
	return "image/svg+xml";
}

// Extension
const char*
SVGExporter::Extension()
{
	return "svg";
}

// #pragma mark -

// SetOriginalEntry
void
SVGExporter::SetOriginalEntry(const entry_ref* ref)
{
	if (ref) {
		delete fOriginalEntry;
		fOriginalEntry = new entry_ref(*ref);
	}
}

// DisplayWarning
bool
SVGExporter::_DisplayWarning() const
{
	BAlert* alert = new BAlert(B_TRANSLATE("warning"),
							   B_TRANSLATE("Icon-O-Matic might not have "
							   "interpreted all data from the SVG "
							   "when it was loaded. "
							   "By overwriting the original "
							   "file, this information would now "
							   "be lost."),
							   B_TRANSLATE("Cancel"), 
							   B_TRANSLATE("Overwrite"));
	return alert->Go() == 1;
}

// #pragma mark -

// convert_join_mode_svg
const char*
convert_join_mode_svg(agg::line_join_e mode)
{
	const char* svgMode = "miter";
	switch (mode) {
		case agg::round_join:
			svgMode = "round";
			break;
		case agg::bevel_join:
			svgMode = "bevel";
			break;
		case agg::miter_join:
		default:
			break;
	}
	return svgMode;
}

// convert_cap_mode_svg
const char*
convert_cap_mode_svg(agg::line_cap_e mode)
{
	const char* svgMode = "butt";
	switch (mode) {
		case agg::square_cap:
			svgMode = "square";
			break;
		case agg::round_cap:
			svgMode = "round";
			break;
		case agg::butt_cap:
		default:
			break;
	}
	return svgMode;
}

// #pragma mark -

// _ExportShape
status_t
SVGExporter::_ExportShape(const Shape* shape, BPositionIO* stream)
{
	if (shape->MaxVisibilityScale() < 1.0
		|| shape->MinVisibilityScale() > 1.0) {
		// don't export shapes which are not visible at the
		// default scale
		return B_OK;
	}

	const Style* style = shape->Style();

	char color[64];
	status_t ret = _GetFill(style, color, stream);

	if (ret < B_OK)
		return ret;

	// The transformation matrix is extracted again in order to
	// maintain the effect of a distorted outline. There is of
	// course a difference when applying the transformation to
	// the points of the path, then stroking the transformed
	// path with an outline of a certain width, opposed to applying
	// the transformation to the points of the generated stroke
	// as well. Adobe SVG Viewer is supporting this fairly well,
	// though I have come across SVG software that doesn't (InkScape).

	// start new shape and write transform matrix
	BString helper;
	helper << "  <path ";

	// hack to see if this is an outline shape
	StrokeTransformer* stroke
		= dynamic_cast<StrokeTransformer*>(shape->TransformerAt(0));
	if (stroke) {
		helper << "style=\"fill:none; stroke:" << color;
		if (!style->Gradient() && style->Color().alpha < 255) {
			helper << "; stroke-opacity:";
			append_float(helper, style->Color().alpha / 255.0);
		}
		helper << "; stroke-width:";
		append_float(helper, stroke->width());

		if (stroke->line_cap() != agg::butt_cap) {
			helper << "; stroke-linecap:";
			helper << convert_cap_mode_svg(stroke->line_cap());
		}

		if (stroke->line_join() != agg::miter_join) {
			helper << "; stroke-linejoin:";
			helper << convert_join_mode_svg(stroke->line_join());
		}

		helper << "\"\n";
	} else {
		helper << "style=\"fill:" << color;

		if (!style->Gradient() && style->Color().alpha < 255) {
			helper << "; fill-opacity:";
			append_float(helper, style->Color().alpha / 255.0);
		}

//		if (shape->FillingRule() == FILL_MODE_EVEN_ODD &&
//			shape->Paths()->CountPaths() > 1)
//			helper << "; fill-rule:evenodd";

		helper << "\"\n";
	}

	helper << "        d=\"";
	ret = write_line(stream, helper);

	if (ret < B_OK)
		return ret;

	int32 count = shape->Paths()->CountPaths();
	for (int32 i = 0; i < count; i++) {
		VectorPath* path = shape->Paths()->PathAtFast(i);

		if (i > 0) {
			helper << "\n           ";
		}

		BPoint a, aIn, aOut;
		if (path->GetPointAt(0, a)) {
			helper << "M";
			append_float(helper, a.x, 2);
			helper << " ";
			append_float(helper, a.y, 2);
		}

		int32 pointCount = path->CountPoints();
		for (int32 j = 0; j < pointCount; j++) {

			if (!path->GetPointsAt(j, a, aIn, aOut))
				break;

			BPoint b, bIn, bOut;
			if ((j + 1 < pointCount && path->GetPointsAt(j + 1, b, bIn, bOut))
				|| (path->IsClosed() && path->GetPointsAt(0, b, bIn, bOut))) {

				if (aOut == a && bIn == b) {
					if (a.x == b.x) {
						// vertical line
						helper << "V";
						append_float(helper, b.y, 2);
					} else if (a.y == b.y) {
						// horizontal line
						helper << "H";
						append_float(helper, b.x, 2);
					} else {
						// line
						helper << "L";
						append_float(helper, b.x, 2);
						helper << " ";
						append_float(helper, b.y, 2);
					}
				} else {
					// cubic curve
					helper << "C";
					append_float(helper, aOut.x, 2);
					helper << " ";
					append_float(helper, aOut.y, 2);
					helper << " ";
					append_float(helper, bIn.x, 2);
					helper << " ";
					append_float(helper, bIn.y, 2);
					helper << " ";
					append_float(helper, b.x, 2);
					helper << " ";
					append_float(helper, b.y, 2);
				}
			}
		}
		if (path->IsClosed())
			helper << "z";

		ret = write_line(stream, helper);
		if (ret < B_OK)
			break;
	}
	helper << "\"\n";

	if (!shape->IsIdentity()) {
		helper << "        transform=\"";
		_AppendMatrix(shape, helper);
		helper << "\n";
	}

	if (ret >= B_OK) {
		helper << "  />\n";

		ret = write_line(stream, helper);
	}

	return ret;
}

// _ExportGradient
status_t
SVGExporter::_ExportGradient(const Gradient* gradient, BPositionIO* stream)
{
	BString helper;

	// start new gradient tag
	if (gradient->Type() == GRADIENT_CIRCULAR) {
		helper << "  <radialGradient ";
	} else {
		helper << "  <linearGradient ";
	}

	// id
	BString gradientName("gradient");
	gradientName << fGradientCount;

	helper << "id=\"" << gradientName << "\" gradientUnits=\"userSpaceOnUse\"";

	// write gradient transformation
	if (gradient->Type() == GRADIENT_CIRCULAR) {
		helper << " cx=\"0\" cy=\"0\" r=\"64\"";
		if (!gradient->IsIdentity()) {
			helper << " gradientTransform=\"";
			_AppendMatrix(gradient, helper);
		}
	} else {
		double x1 = -64.0;
		double y1 = -64.0;
		double x2 = 64.0;
		double y2 = -64.0;
		gradient->Transform(&x1, &y1);
		gradient->Transform(&x2, &y2);

		helper << " x1=\"";
		append_float(helper, x1, 2);
		helper << "\"";
		helper << " y1=\"";
		append_float(helper, y1, 2);
		helper << "\"";
		helper << " x2=\"";
		append_float(helper, x2, 2);
		helper << "\"";
		helper << " y2=\"";
		append_float(helper, y2, 2);
		helper << "\"";
	}

	helper << ">\n";

	// write stop tags
	char color[16];
	for (int32 i = 0; BGradient::ColorStop* stop = gradient->ColorAt(i); i++) {

		sprintf(color, "%.2x%.2x%.2x", stop->color.red,
									   stop->color.green,
									   stop->color.blue);
		color[6] = 0;

		helper << "   <stop offset=\"";
		append_float(helper, stop->offset);
		helper << "\" stop-color=\"#" << color << "\"";

		if (stop->color.alpha < 255) {
			helper << " stop-opacity=\"";
			append_float(helper, (float)stop->color.alpha / 255.0);
			helper << "\"";
		}
		helper << "/>\n";
	}

	// finish gradient tag
	if (gradient->Type() == GRADIENT_CIRCULAR) {
		helper << "  </radialGradient>\n";
	} else {
		helper << "  </linearGradient>\n";
	}

	return write_line(stream, helper);
}

// _AppendMatrix
void
SVGExporter::_AppendMatrix(const Transformable* object, BString& string) const
{
	string << "matrix(";
//	append_float(string, object->sx);
//	string << ",";
//	append_float(string, object->shy);
//	string << ",";
//	append_float(string, object->shx);
//	string << ",";
//	append_float(string, object->sy);
//	string << ",";
//	append_float(string, object->tx);
//	string << ",";
//	append_float(string, object->ty);
double matrix[Transformable::matrix_size];
object->StoreTo(matrix);
append_float(string, matrix[0]);
string << ",";
append_float(string, matrix[1]);
string << ",";
append_float(string, matrix[2]);
string << ",";
append_float(string, matrix[3]);
string << ",";
append_float(string, matrix[4]);
string << ",";
append_float(string, matrix[5]);
	string << ")\"";
}

// _GetFill
status_t
SVGExporter::_GetFill(const Style* style, char* string,
					  BPositionIO* stream)
{
	status_t ret = B_OK;
	if (Gradient* gradient = style->Gradient()) {
		ret = _ExportGradient(gradient, stream);
		sprintf(string, "url(#gradient%ld)", fGradientCount++);
	} else {
		sprintf(string, "#%.2x%.2x%.2x", style->Color().red,
										 style->Color().green,
										 style->Color().blue);
	}
	return ret;
}
