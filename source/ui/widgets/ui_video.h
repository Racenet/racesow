#ifndef __UI_VIDEO_H__
#define __UI_VIDEO_H__

#include "kernel/ui_common.h"
#include "kernel/ui_utils.h"
#include "widgets/ui_image.h"

namespace WSWUI
{
	class Video : public ElementImage
	{
	public:
		/// Initializes the video element
		explicit Video(const Rocket::Core::String&);
	
		virtual void OnAttributeChange(const Rocket::Core::AttributeNameList&);
	private:
	};
}

#endif // __UI_VIDEO_H__
