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
#include "uiwsw_SysCalls.h"

namespace UIMenu
{
	MenuBase *activeMenu;

	UICore::Color black( 0.0f, 0.0f, 0.0f, 0.6f);
	UICore::Color darkgray( 0.25f, 0.25f, 0.25f, 0.6f );
	UICore::Color midgray( 0.5f, 0.5f, 0.5f, 0.6f );

	UICore::Color blue1( 0.6f, 0.45f, 0.8f, 0.8f );
	UICore::Color blue2( 0.45f, 0.3f, 0.95f, 0.7f );
	UICore::Color blue3( 0.25f, 0.2f, 0.6f, 1.0f );
	UICore::Color blue4( 0.13f, 0.1f, 0.3f, 1.0f );
	UICore::Color blue5( 0.29f, 0.25f, 0.43f, 1.0f );
	UICore::Color blue6( 0.125f, 0.11f, 0.176f, 1.0f );
	UICore::Color blue7( 0.13f, 0.12f, 0.18f, 0.9f );
	UICore::Color blue9( 0.164f, 0.135f, 0.27f, 0.9f );

	UICore::Color orange( 1.0f, 0.61f, 0.0f, 1.0f );
	UICore::Color white( 1.0f, 1.0f, 1.0f, 1.0f );
	UICore::Color transparent( 1.0f, 1.0f, 1.0f, 0.0f );
}

static UICore::Panel *panel;
static UICore::Panel *frame;
static UICore::Label *label;
static UICore::Button *button;
static UICore::SwitchButton *switchbutton;
static UICore::CheckBox *checkBox;
static UICore::TextBox *textBox;
static UICore::Slider *slider;
static UICore::ListItem *listitem;
static UICore::ListBox *listBox;
static UICore::DropDownBox *dropDownBox;
static UICore::TitleBar *titleBar;

void UIMenu::BuildTemplates( void )
{	
	UICore::Font font = UIWsw::Local::getFontSmall();

	if ( !font )
		return;

	UICore::Image panelBorders[UICore::BORDER_MAX_BORDERS];
	panelBorders[UICore::BORDER_TOP_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_panel_corner" );
	panelBorders[UICore::BORDER_TOP] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_panel_border_top" );
	panelBorders[UICore::BORDER_TOP_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_panel_corner_right" );
	panelBorders[UICore::BORDER_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_panel_border_right" );
	panelBorders[UICore::BORDER_BOTTOM_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_panel_corner_right_bottom" );
	panelBorders[UICore::BORDER_BOTTOM] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_panel_border_bottom" );
	panelBorders[UICore::BORDER_BOTTOM_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_panel_corner_left_bottom" );
	panelBorders[UICore::BORDER_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_panel_border_left" );

	UICore::Image frameBorders[UICore::BORDER_MAX_BORDERS];
	frameBorders[UICore::BORDER_TOP_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_frame_corner" );
	frameBorders[UICore::BORDER_TOP] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_frame_border_top" );
	frameBorders[UICore::BORDER_TOP_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_frame_corner_right" );
	frameBorders[UICore::BORDER_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_frame_border_right" );
	frameBorders[UICore::BORDER_BOTTOM_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_frame_corner_right_bottom" );
	frameBorders[UICore::BORDER_BOTTOM] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_frame_border_bottom" );
	frameBorders[UICore::BORDER_BOTTOM_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_frame_corner_left_bottom" );
	frameBorders[UICore::BORDER_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_frame_border_left" );

	UICore::Image buttonBorders[UICore::BORDER_MAX_BORDERS];
	buttonBorders[UICore::BORDER_TOP_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_corner_top" );
	buttonBorders[UICore::BORDER_TOP] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_border_top" );
	buttonBorders[UICore::BORDER_TOP_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_corner_top" );
	buttonBorders[UICore::BORDER_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_border_left" );
	buttonBorders[UICore::BORDER_BOTTOM_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_corner_bottom" );
	buttonBorders[UICore::BORDER_BOTTOM] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_border_bottom" );
	buttonBorders[UICore::BORDER_BOTTOM_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_corner_bottom" );
	buttonBorders[UICore::BORDER_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_border_left" );

	panel = new UICore::Panel( NULL, 0, 0, 200, 200 );
	panel->setVisible( true );
	panel->setEnabled( true );
	panel->setDraggable( true );
	panel->setClickable( true );
	panel->setBackColor( white );
	panel->setBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_panel_bg" ) );
	panel->setBorderWidth( 10 );
	panel->setBorderImages( panelBorders );
	panel->setBorderColor( white );
	panel->setDisabledColor( darkgray );
	panel->setDisabledImage( panel->getBackgroundImage() );
	UICore::Factory::setPanelTemplate( panel );

	frame = new UICore::Panel( NULL, 0, 0, 200, 200 );
	frame->setVisible( true );
	frame->setEnabled( true );
	frame->setDraggable( true );
	frame->setClickable( true );
	frame->setBackColor( white );
	frame->setBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_frame_bg" ) );
	frame->setBorderWidth( 10 );
	frame->setBorderImages( frameBorders );
	frame->setBorderColor( white );
	frame->setDisabledColor( darkgray );
	frame->setDisabledImage( frame->getBackgroundImage() );
	UICore::Factory::setFrameTemplate( frame );

	label = new UICore::Label( NULL, 0, 0, 100, 20, "" );
	label->setVisible( true );
	label->setEnabled( true );
	label->setDraggable( false );
	label->setClickable( true );
	label->setBackColor( transparent );
	label->setBorderColor( blue5 );
	label->setDisabledColor( transparent );
	label->setBackgroundImage( NULL );
	label->setDisabledImage( NULL );
	label->setBorderWidth( 0 );
	label->setFont( font );
	label->setFontColor( white );
	label->setDisabledFontColor( midgray );
	label->setMultiline( false );
	label->setAlign( UICore::ALIGN_MIDDLE_LEFT );
	UICore::Factory::setLabelTemplate( label );

	button = new UICore::Button( NULL, 0, 0, 100, 30, "" );
	button->setVisible( true );
	button->setEnabled( true );
	button->setDraggable( false );
	button->setClickable( true );
	button->setBackColor( white );
	button->setBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_background" ) );
	button->setDisabledColor( midgray );
	button->setDisabledImage( button->getBackgroundImage() );
	button->setHighlightColor( white );
	button->setHighlightImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_button_background_hover" ) );
	button->setBorderWidth( 1 );
	button->setBorderImages( buttonBorders );
	button->setBorderColor( white );
	button->setFont( font );
	button->setFontColor( white );
	button->setHighlightFontColor( white );
	button->setDisabledFontColor( midgray );
	button->setAlign( UICore::ALIGN_MIDDLE_CENTER );
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
	switchbutton->setBorderImages( buttonBorders );
	switchbutton->setBorderColor( white );
	switchbutton->setFont( font );
	switchbutton->setFontColor( white );
	switchbutton->setHighlightFontColor( orange );
	switchbutton->setDisabledFontColor( midgray );
	switchbutton->setAlign( UICore::ALIGN_MIDDLE_CENTER );
	switchbutton->setHighlightColor( blue3 );
	switchbutton->setHighlightImage( NULL );
	switchbutton->setPressed( false );
	switchbutton->setPressedColor( orange );
	switchbutton->setPressedFontColor( blue3 );
	switchbutton->setPressedImage( NULL );
	UICore::Factory::setSwitchButtonTemplate( switchbutton );

	checkBox = new UICore::CheckBox( NULL, 0, 0, 100, 16, "" );
	checkBox->setVisible( true );
	checkBox->setEnabled( true );
	checkBox->setDraggable( false );
	checkBox->setClickable( true );
	checkBox->setBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_checkbox" ) );
	checkBox->setBackColor( white );
	checkBox->setHighlightImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_checkbox_hover" ) );
	checkBox->setHighlightColor( checkBox->getBackColor() );
	checkBox->setPressedImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_checkbox_checked" ) );
	checkBox->setPressedColor( white );
	checkBox->setPressedHighlightImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_checkbox_checked_hover" ) );
	checkBox->setPressedHighlightColor( checkBox->getPressedColor() );
	checkBox->setDisabledImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_checkbox_disabled" ) );
	checkBox->setDisabledColor( midgray );
	checkBox->setBorderWidth( 1 );
	checkBox->setBorderImages( buttonBorders );
	checkBox->setBorderColor( white );
	checkBox->setFont( font );
	checkBox->setFontColor( white );
	checkBox->setDisabledFontColor( midgray );
	checkBox->setAlign( UICore::ALIGN_MIDDLE_LEFT );
	checkBox->setPressed( false );
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
	textBox->setFontColor( white );
	textBox->setHighlightFontColor( white );
	textBox->setBackColor( blue6 );
	textBox->setBorderImages( buttonBorders );
	textBox->setBorderColor( white );
	textBox->setBorderWidth( 1 );
	textBox->setHighlightColor( blue3 );
	textBox->setEditingColor( orange );
	textBox->setEditingFontColor( blue3 );
	UICore::Factory::setTextBoxTemplate( textBox );

	slider = new UICore::Slider( NULL, 0, 0, 100, 20, true );
	slider->setBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_slider_center" ) );
	slider->setDisabledImage( slider->getBackgroundImage() );
	slider->setHighlightImage( slider->getBackgroundImage() );
	slider->setBackColor( white );
	slider->setDisabledColor( midgray );
	slider->setHighlightColor( white );
	slider->setBorderWidth( 0 );
	slider->setButtonBorderWidth( 0 );
	slider->setButtonBackColor( white );
	slider->setButtonBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_slider_left" ) );
	slider->setButtonDisabledColor( midgray );
	slider->setButtonDisabledImage( slider->getButtonBackgroundImage() );
	slider->setButtonHighlightColor( white );
	slider->setButtonHighlightImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_slider_left_active" ) );
	slider->setCursorBorderWidth( 0 );
	slider->setCursorBackColor( white );
	slider->setCursorDisabledColor( transparent );
	slider->setCursorBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_slider_selector" ) );
	slider->setCursorDisabledImage( NULL );
	slider->setCursorHighlightColor( white );
	slider->setCursorHighlightImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_slider_selector_active" ) );
	UICore::Factory::setSliderTemplate( slider );

	listitem = new UICore::ListItem( 0, 0, 100, 30, "" );
	listitem->setBackColor( transparent );
	listitem->setBorderColor( transparent );
	listitem->setDisabledColor( transparent );
	listitem->setBackgroundImage( NULL );
	listitem->setDisabledImage( NULL );
	listitem->setBorderWidth( 0 );
	listitem->setFont( font );
	listitem->setFontColor( white );
	listitem->setHighlightFontColor( white );
	listitem->setDisabledFontColor( midgray );
	listitem->setAlign( UICore::ALIGN_MIDDLE_CENTER );
	listitem->setHighlightColor( blue9 );
	listitem->setHighlightImage( NULL );
	listitem->setHighlightFontColor( white );
	listitem->setPressedColor( orange );
	listitem->setPressedFontColor( blue3 );
	listitem->setPressedImage( NULL );
	UICore::Factory::setListItemTemplate( listitem );

	listBox = new UICore::ListBox( NULL, 0, 0, 100, 100, false );
	listBox->setVisible( true );
	listBox->setEnabled( true );
	listBox->setDraggable( false );
	listBox->setClickable( true );
	listBox->setBackColor( blue6 );
	listBox->setBorderColor( blue2 );
	listBox->setDisabledColor( darkgray );
	listBox->setBackgroundImage( NULL );
	listBox->setDisabledImage( NULL );
	listBox->setBorderWidth( 1 );
	listBox->setBorderColor( blue5 );
	listBox->setMultipleSelection( false );
	listBox->setScrollbarSize( 22 );
	listBox->setItemSize( 22 );
	UICore::Factory::setListBoxTemplate( listBox );	

	dropDownBox = new UICore::DropDownBox( NULL, 0, 0, 150, 22 );
	dropDownBox->setVisible( true );
	dropDownBox->setEnabled( true );
	dropDownBox->setDraggable( false );
	dropDownBox->setClickable( true );
	dropDownBox->setBackColor( blue6 );
	dropDownBox->setDisabledColor( midgray );
	dropDownBox->setBackgroundImage( NULL );
	dropDownBox->setDisabledImage( NULL );
	dropDownBox->setBorderImages( buttonBorders );
	dropDownBox->setBorderColor( white );
	dropDownBox->setBorderWidth( 1 );
	dropDownBox->setMultipleSelection( false );
	dropDownBox->setScrollbarSize( 22 );
	dropDownBox->setItemSize( 16 );
	dropDownBox->setButtonBorderWidth( 0 );
	dropDownBox->setButtonBackColor( white );
	dropDownBox->setButtonBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_dropdown_arrow" ) );
	dropDownBox->setButtonDisabledColor( midgray );
	dropDownBox->setButtonDisabledImage( dropDownBox->getButtonBackgroundImage() );
	dropDownBox->setButtonHighlightColor( white );
	dropDownBox->setButtonHighlightImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_dropdown_arrow_hover" ) );
	UICore::Factory::setDropDownBoxTemplate( dropDownBox );

	titleBar = new UICore::TitleBar();
	titleBar->setSize( 886, 24 );
	titleBar->setVisible( true );
	titleBar->setEnabled( true );
	titleBar->setDraggable( false );
	titleBar->setClickable( true );
	titleBar->setBackColor( white );
	titleBar->setBorderColor( blue5 );
	titleBar->setDisabledColor( midgray );
	titleBar->setPadLeft( 28 );
	titleBar->setBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_top_bg" ) );
	titleBar->setDisabledImage( titleBar->getBackgroundImage() );
	titleBar->setBorderWidth( 0 );
	titleBar->setFont( font );
	titleBar->setFontColor( white );
	titleBar->setDisabledFontColor( midgray );
	titleBar->setMultiline( false );
	titleBar->setAlign( UICore::ALIGN_MIDDLE_LEFT );
	UICore::Factory::setTitleBarTemplate( titleBar );
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
