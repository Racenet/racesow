/*
Copyright (C) 2007 Benjamin Litzelmann

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#ifndef _UICORE_FACTORY_H_
#define _UICORE_FACTORY_H_

#include "uicore_Types.h"

namespace UICore
{
	class Factory
	{
	private:
		static const Panel *panelTemplate;
		static const Panel *frameTemplate;
		static const Label *labelTemplate;
		static const Button *buttonTemplate;
		static const SwitchButton *switchButtonTemplate;
		static const TextBox *textBoxTemplate;
		static const CheckBox *checkBoxTemplate;
		static const Slider *sliderTemplate;
		static const ListItem *listItemTemplate;
		static const ListBox *listBoxTemplate;
		static const DropDownBox *dropDownBoxTemplate;
		static const TitleBar *titleBarTemplate;

		static void setBaseObjectProperties( BaseObject *baseObject, const BaseObject *templateBaseObject );
		static void setLabelProperties( Label *label, const Label *templateLabel );
		static void setSwitchButtonProperties( SwitchButton *switchButton, const SwitchButton *templateSwitchButton );
		static void setButtonProperties( Button *button, const Button *templateButton );
		static void setTextBoxProperties( TextBox *textBox, const TextBox *templateTextBox );
		static void setCheckBoxProperties( CheckBox *checkBox, const CheckBox *templateCheckBox );
		static void setSliderProperties( Slider *slider, const Slider *templateSlider );
		static void setListBoxProperties( ListBox *listBox, const ListBox *templateListBox );
		static void setDropDownBoxProperties( DropDownBox *dropDownBox, const DropDownBox *templateDropDownBox );
		static void setTitleBarProperties( TitleBar *titleBar, const TitleBar *templateTitleBar );

	public:

		static void setPanelTemplate( const Panel *panel );
		static void setFrameTemplate( const Panel *panel );
		static void setLabelTemplate( const Label *label );
		static void setButtonTemplate( const Button *button );
		static void setSwitchButtonTemplate( const SwitchButton *switchButton );
		static void setTextBoxTemplate( const TextBox *textBox );
		static void setCheckBoxTemplate( const CheckBox *checkBox );
		static void setSliderTemplate( const Slider *slider );
		static void setListItemTemplate( const ListItem *listItem );
		static void setListBoxTemplate( const ListBox *listBox );
		static void setDropDownBoxTemplate( const DropDownBox *dropDownBox );
		static void setTitleBarTemplate( const TitleBar *titleBar );

		/** Create a new panel at given position, with the given parent object (can be NULL) */
		static Panel * newPanel( BaseObject *parent, float x, float y, float w, float h );
		/** Create a new frame at given position, with the given parent object (can be NULL) */
		static Panel * newFrame( BaseObject *parent, float x, float y, float w, float h );
		/** Create a new label at given position, with the given parent object (can be NULL) and given caption */
		static Label * newLabel( BaseObject *parent, float x, float y, float w, float h, std::string caption = "" );
		/** Create a new button at given position, with the given parent object (can be NULL) and given caption  */
		static Button * newButton( BaseObject *parent, float x, float y, float w, float h, std::string caption = "" );
		/** Create a new switchbutton at given position, with the given parent object (can be NULL) and given caption */
		static SwitchButton * newSwitchButton( BaseObject *parent, float x, float y, float w, float h, std::string caption = "" );
		/** Create a new textbox at given position, with the given parent object (can be NULL) and given buffer size */
		static TextBox * newTextBox( BaseObject *parent, float x, float y, float w, float h, int bufferSize = 16 );
		/** Create a new checkbox at given position, with the given parent object (can be NULL) and given caption */
		static CheckBox * newCheckBox( BaseObject *parent, float x, float y, std::string caption = "", float w = 0, float h = 0 );
		/** Create a new slider at given position, with the given parent object (can be NULL) and given orientation */
		static Slider * newSlider( BaseObject *parent, float x, float y, float w, float h, bool horizontal );
		/** Create a new item in the given list. It instanciates a new ListItem and add it to the list at given position.
			@param parent the listbox to add the item on.
			@param caption caption of the new item.
			@param position of the item in the list. 0 is first. Items in following positions
			are shifted by 1. Use -1 to add the item at the end of the list. */
		static ListItem * newListItem( ListBox *parent, std::string caption = "", int position = -1 );
		/** Create a new panel at given position, with the given parent object (can be NULL) and given orientation */
		static ListBox * newListBox( BaseObject *parent, float x, float y, float w, float h, bool horizontal );
		/** Create a new dropdown box at the given position, with the given parent object (can be NULL) and a given list height and title */
		static DropDownBox * newDropDownBox( BaseObject *parent, float x, float y, float w, float h, float listh = 150, std::string title = "" );
		/** Create a new title bar at the given position, with the given parent object (can be NULL), a title and optionally width and height */
		static TitleBar * newTitleBar( BaseObject *parent, float x, float y, std::string caption, float w = 0, float h = 0 );
		/** Create a new message box with the given caption and buttons, with an optional image */
		static MessageBox * newMessageBox( std::string caption, int buttons, MessageBoxImage image = IMAGE_NONE );
		/** Create a new message box without any properties */
		static MessageBox * newMessageBox( void );
	};
}

#endif
