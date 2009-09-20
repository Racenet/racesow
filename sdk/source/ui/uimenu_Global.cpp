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


#include "uimenu_Global.h"
#include "uiwsw_Utils.h"

namespace UIMenu
{
	MenuBase *activeMenu;

	UICore::Color black( 0.0f, 0.0f, 0.0f, 0.6f);
	UICore::Color darkgray( 0.25f, 0.25f, 0.25f, 0.6f );
	UICore::Color midgray( 0.5f, 0.5f, 0.5f, 0.6f );

	UICore::Color blue1( 0.6f, 0.45f, 0.8f, 0.8f );
	UICore::Color blue2( 0.45f, 0.3f, 0.95f, 0.7f );
	UICore::Color blue3( 0.25f, 0.2f, 0.6f, 0.6f );
	UICore::Color blue4( 0.15f, 0.1f, 0.3f, 0.5f );

	UICore::Color orange( 1.0f, 0.7f, 0.0f, 0.8f );
	UICore::Color white( 1.0f, 1.0f, 1.0f, 1.0f );
	UICore::Color transparent( 1.0f, 1.0f, 1.0f, 0.0f );
}

static UICore::Panel *panel;
static UICore::Label *label;
static UICore::Button *button;
static UICore::SwitchButton *switchbutton;
static UICore::CheckBox *checkBox;
static UICore::TextBox *textBox;
static UICore::Slider *slider;
static UICore::ListItem *listitem;
static UICore::ListBox *listBox;
static UICore::DropDownBox *dropDownBox;

void UIMenu::BuildTemplates( void )
{	
	UICore::Font font = UIWsw::Local::getFontSmall();

	if ( !font )
		return;

	panel = new UICore::Panel( NULL, 0, 0, 200, 200 );
	panel->setVisible( true );
	panel->setEnabled( true );
	panel->setDraggable( true );
	panel->setClickable( true );
	panel->setBackColor( blue4 );
	panel->setBorderColor( blue2 );
	panel->setDisabledColor( darkgray );
	panel->setBackgroundImage( NULL );
	panel->setDisabledImage( NULL );
	panel->setBorderWidth( 2 );
	UICore::Factory::setPanelTemplate( panel );

	label = new UICore::Label( NULL, 0, 0, 100, 20, "" );
	label->setVisible( true );
	label->setEnabled( true );
	label->setDraggable( false );
	label->setClickable( true );
	label->setBackColor( transparent );
	label->setBorderColor( blue2 );
	label->setDisabledColor( transparent );
	label->setBackgroundImage( NULL );
	label->setDisabledImage( NULL );
	label->setBorderWidth( 0 );
	label->setFont( font );
	label->setFontColor( blue1 );
	label->setDisabledFontColor( midgray );
	label->setAlign( UICore::ALIGN_MIDDLE_LEFT );
	label->setMultiline( false );
	UICore::Factory::setLabelTemplate( label );

	button = new UICore::Button( NULL, 0, 0, 100, 30, "" );
	button->setVisible( true );
	button->setEnabled( true );
	button->setDraggable( false );
	button->setClickable( true );
	button->setBackColor( blue4 );
	button->setBorderColor( blue2 );
	button->setDisabledColor( darkgray );
	button->setBackgroundImage( NULL );
	button->setDisabledImage( NULL );
	button->setBorderWidth( 1 );
	button->setFont( font );
	button->setFontColor( blue1 );
	button->setDisabledFontColor( midgray );
	button->setAlign( UICore::ALIGN_MIDDLE_CENTER );
	button->setHighlightColor( blue3 );
	button->setHighlightImage( NULL );
	UICore::Factory::setButtonTemplate( button );

	switchbutton = new UICore::SwitchButton( NULL, 0, 0, 100, 30, "" );
	switchbutton->setVisible( true );
	switchbutton->setEnabled( true );
	switchbutton->setDraggable( false );
	switchbutton->setClickable( true );
	switchbutton->setBackColor( blue4 );
	switchbutton->setBorderColor( blue2 );
	switchbutton->setDisabledColor( darkgray );
	switchbutton->setBackgroundImage( NULL );
	switchbutton->setDisabledImage( NULL );
	switchbutton->setBorderWidth( 1 );
	switchbutton->setFont( font );
	switchbutton->setFontColor( blue1 );
	switchbutton->setDisabledFontColor( midgray );
	switchbutton->setAlign( UICore::ALIGN_MIDDLE_CENTER );
	switchbutton->setHighlightColor( blue3 );
	switchbutton->setHighlightImage( NULL );
	switchbutton->setPressed( false );
	switchbutton->setPressedColor( orange );
	switchbutton->setPressedFontColor( blue3 );
	switchbutton->setPressedImage( NULL );
	UICore::Factory::setSwitchButtonTemplate( switchbutton );

	checkBox = new UICore::CheckBox( NULL, 0, 0, 100, 20, "" );
	checkBox->setVisible( true );
	checkBox->setEnabled( true );
	checkBox->setDraggable( false );
	checkBox->setClickable( true );
	checkBox->setBackColor( blue4 );
	checkBox->setBorderColor( blue2 );
	checkBox->setDisabledColor( darkgray );
	checkBox->setBackgroundImage( NULL );
	checkBox->setDisabledImage( NULL );
	checkBox->setBorderWidth( 1 );
	checkBox->setFont( font );
	checkBox->setFontColor( blue1 );
	checkBox->setDisabledFontColor( midgray );
	checkBox->setAlign( UICore::ALIGN_MIDDLE_LEFT );
	checkBox->setHighlightColor( blue3 );
	checkBox->setHighlightImage( NULL );
	checkBox->setPressed( false );
	checkBox->setPressedColor( orange );
	checkBox->setPressedImage( NULL );
	checkBox->setLabelBackColor( transparent );
	UICore::Factory::setCheckBoxTemplate( checkBox );

	textBox = new UICore::TextBox( NULL, 0, 0, 150, 20, 50 );
	textBox->setVisible( true );
	textBox->setEnabled( true );
	textBox->setDraggable( false );
	textBox->setClickable( true );
	textBox->setDisabledColor( darkgray );
	textBox->setBackgroundImage( NULL );
	textBox->setDisabledImage( NULL );
	textBox->setFont( font );
	textBox->setFontColor( blue1 );
	textBox->setBackColor( blue4 );
	textBox->setBorderColor( blue2 );
	textBox->setBorderWidth( 1 );
	textBox->setHighlightColor( blue3 );
	textBox->setEditingColor( orange );
	textBox->setEditingFontColor( blue3 );
	UICore::Factory::setTextBoxTemplate( textBox );

	slider = new UICore::Slider( NULL, 0, 0, 100, 20, true );
	slider->setBackColor( blue4 );
	slider->setBorderColor( blue2 );
	slider->setBorderWidth( 1 );
	slider->setDisabledColor( darkgray );
	slider->setButtonBackColor( blue3 );
	slider->setButtonHighlightColor( blue2 );
	slider->setButtonBorderColor( blue2 );
	slider->setButtonBorderWidth( 1 );
	slider->setButtonDisabledColor( darkgray );
	slider->setCursorBackColor( blue4 );
	slider->setCursorHighlightColor( blue2 );
	slider->setCursorBorderColor( blue2 );
	slider->setCursorBorderWidth( 1 );
	slider->setCursorDisabledColor( darkgray );
	UICore::Factory::setSliderTemplate( slider );

	listitem = new UICore::ListItem( 0, 0, 100, 30, "" );
	listitem->setBackColor( transparent );
	listitem->setBorderColor( transparent );
	listitem->setDisabledColor( transparent );
	listitem->setBackgroundImage( NULL );
	listitem->setDisabledImage( NULL );
	listitem->setBorderWidth( 0 );
	listitem->setFont( font );
	listitem->setFontColor( blue1 );
	listitem->setDisabledFontColor( midgray );
	listitem->setAlign( UICore::ALIGN_MIDDLE_CENTER );
	listitem->setHighlightColor( blue3 );
	listitem->setHighlightImage( NULL );
	listitem->setPressedColor( orange );
	listitem->setPressedFontColor( blue3 );
	listitem->setPressedImage( NULL );
	UICore::Factory::setListItemTemplate( listitem );

	listBox = new UICore::ListBox( NULL, 0, 0, 100, 100, false );
	listBox->setVisible( true );
	listBox->setEnabled( true );
	listBox->setDraggable( false );
	listBox->setClickable( true );
	listBox->setBackColor( blue4 );
	listBox->setBorderColor( blue2 );
	listBox->setDisabledColor( darkgray );
	listBox->setBackgroundImage( NULL );
	listBox->setDisabledImage( NULL );
	listBox->setBorderWidth( 1 );
	listBox->setMultipleSelection( false );
	listBox->setScrollbarSize( 15 );
	listBox->setItemSize( 15 );
	UICore::Factory::setListBoxTemplate( listBox );	

	dropDownBox = new UICore::DropDownBox( NULL, 0, 0, 150, 20 );
	dropDownBox->setVisible( true );
	dropDownBox->setEnabled( true );
	dropDownBox->setDraggable( false );
	dropDownBox->setClickable( true );
	dropDownBox->setBackColor( blue4 );
	dropDownBox->setBorderColor( blue2 );
	dropDownBox->setDisabledColor( darkgray );
	dropDownBox->setBackgroundImage( NULL );
	dropDownBox->setDisabledImage( NULL );
	dropDownBox->setBorderWidth( 1 );
	dropDownBox->setMultipleSelection( false );
	dropDownBox->setScrollbarSize( 15 );
	dropDownBox->setItemSize( 15 );
	UICore::Factory::setDropDownBoxTemplate( dropDownBox );
}

void UIMenu::FreeTemplates( void )
{
	delete panel;
	delete label;
	delete button;
	delete switchbutton;
	delete checkBox;
	delete textBox;
	delete slider;
	delete listitem;
	delete listBox;
	delete dropDownBox;
}

namespace UIMenu
{
	void setActiveMenu( MenuBase *menu )
	{
		if( activeMenu )
			activeMenu->Hide();
		activeMenu = menu;

		if( activeMenu )
			UIWsw::Trap::CL_SetKeyDest( key_menu );
		else
			UIWsw::Trap::CL_SetKeyDest( key_game );
	}
}
