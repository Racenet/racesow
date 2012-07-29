#ifndef __UI_LEVELSHOT_H__
#define __UI_LEVELSHOT_H__

#include "kernel/ui_common.h"
#include "kernel/ui_utils.h"
#include "widgets/ui_image.h"

namespace WSWUI
{
	/// A decorator over an image element for displaying
	/// levelshots. The src attribute of this element only
	/// needs to contain the name of the map.
	/// If the appropriate levelshot is not found, a fallback
	/// image is displayed.
	class LevelShot : public ElementImage
	{
	public:
		/// Initializes the levelshot element
		explicit LevelShot(const Rocket::Core::String&);
	
		virtual void OnAttributeChange(const Rocket::Core::AttributeNameList&);

	private:
		/// Generates the path to the preview image from the given map name
		static Rocket::Core::String getImagePath(const Rocket::Core::String&);

		static shader_s *fallbackShader;

		bool srcProcessed;
	};
}

#endif // __UI_LEVELSHOT_H__
