#ifndef _B_FORMAT_PARAMETERS_H_
#define _B_FORMAT_PARAMETERS_H_

#include <SupportDefs.h>

enum format_alignment {
	B_ALIGN_FORMAT_LEFT,		// reuse B_ALIGN_LEFT/B_ALIGN_RIGHT?
	B_ALIGN_FORMAT_RIGHT,
};

class BFormatParameters {
	public:
		BFormatParameters(const BFormatParameters *parent = NULL);
		BFormatParameters(const BFormatParameters &other);
		~BFormatParameters();

		void SetAlignment(format_alignment alignment);
		format_alignment Alignment() const;

		void SetFormatWidth(size_t width);
		size_t FormatWidth() const;

		const BFormatParameters *ParentParameters() const;

		BFormatParameters &operator=(const BFormatParameters &other);

	protected:
		void SetParentParameters(const BFormatParameters *parent);

	private:
		const BFormatParameters	*fParent;
		format_alignment		fAlignment;
		size_t					fWidth;
		uint32					fFlags;
};

#endif	// _B_FORMAT_PARAMETERS_H_
