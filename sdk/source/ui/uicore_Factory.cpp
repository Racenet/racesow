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


#include "uicore_Global.h"

namespace UICore
{
	const Panel *Factory::panelTemplate = NULL;
	const Label *Factory::labelTemplate = NULL;
	const Button *Factory::buttonTemplate = NULL;
	const SwitchButton *Factory::switchButtonTemplate = NULL;
	const TextBox *Factory::textBoxTemplate = NULL;
	const CheckBox *Factory::checkBoxTemplate = NULL;
	const Slider *Factory::sliderTemplate = NULL;
	const ListItem *Factory::listItemTemplate = NULL;
	const ListBox *Factory::listBoxTemplate = NULL;
	const DropDownBox *Factory::dropDownBoxTemplate = NULL;

	void Factory::setPanelTemplate( const Panel *panel )
	{
		panelTemplate = panel;
	}

	void Factory::setLabelTemplate( const Label *label )
	{
		labelTemplate = label;
	}

	void Factory::setButtonTemplate( const Button *button )
	{
		buttonTemplate = button;
	}

	void Factory::setSwitchButtonTemplate( const SwitchButton *switchButton )
	{
		switchButtonTemplate = switchButton;
	}

	void Factory::setTextBoxTemplate( const TextBox *textBox )
	{
		textBoxTemplate = textBox;
	}

	void Factory::setCheckBoxTemplate( const CheckBox *checkBox )
	{
		checkBoxTemplate = checkBox;
	}

	void Factory::setSliderTemplate( const Slider *slider )
	{
		sliderTemplate = slider;
	}

	void Factory::setListItemTemplate( const ListItem *listItem )
	{
		listItemTemplate = listItem;
	}

	void Factory::setListBoxTemplate( const ListBox *listBox )
	{
		listBoxTemplate = listBox;
	}

	void Factory::setDropDownBoxTemplate( const DropDownBox *dropDownBox )
	{
		dropDownBoxTemplate = dropDownBox;
	}

	Panel * Factory::newPanel( BaseObject *parent, float x, float y, float w, float h )
	{
		Panel *inst = new Panel( parent, x, y, w, h );
		if ( labelTemplate )
		{
			setBaseObjectProperties( inst, panelTemplate );
		}
		return inst;
	}

	Label * Factory::newLabel( BaseObject *parent, float x, float y, float w, float h, std::string caption )
	{
		Label *inst = new Label( parent, x, y, w, h, caption );
		if ( labelTemplate )
		{
			setBaseObjectProperties( inst, labelTemplate );
			setLabelProperties( inst, labelTemplate );
		}
		return inst;
	}

	Button * Factory::newButton( BaseObject *parent, float x, float y, float w, float h, std::string caption )
	{
		Button *inst = new Button( parent, x, y, w, h, caption );
		if ( buttonTemplate )
		{
			setBaseObjectProperties( inst, buttonTemplate );
			setLabelProperties( inst, buttonTemplate );
			setButtonProperties( inst, buttonTemplate );
		}
		return inst;
	}

	SwitchButton * Factory::newSwitchButton( BaseObject *parent, float x, float y, float w, float h, std::string caption )
	{
		SwitchButton *inst = new SwitchButton( parent, x, y, w, h, caption );
		if ( switchButtonTemplate )
		{
			setBaseObjectProperties( inst, switchButtonTemplate );
			setLabelProperties( inst, switchButtonTemplate );
			setButtonProperties( inst, switchButtonTemplate );
			setSwitchButtonProperties( inst, switchButtonTemplate );
		}
		return inst;
	}

	TextBox * Factory::newTextBox( BaseObject *parent, float x, float y, float w, float h, int bufferSize )
	{
		TextBox *inst = new TextBox( parent, x, y, w, h, bufferSize );
		if ( textBoxTemplate )
		{
			setBaseObjectProperties( inst, textBoxTemplate );
			setLabelProperties( inst, textBoxTemplate );
			setButtonProperties( inst, textBoxTemplate );
			setTextBoxProperties( inst, textBoxTemplate );
		}
		return inst;
	}
	
	CheckBox * Factory::newCheckBox( BaseObject *parent, float x, float y, float w, float h, std::string caption )
	{
		CheckBox *inst = new CheckBox( parent, x, y, w, h, caption );
		if ( checkBoxTemplate )
		{
			setBaseObjectProperties( inst, checkBoxTemplate );
			setCheckBoxProperties( inst, checkBoxTemplate );
		}
		return inst;
	}
	
	Slider * Factory::newSlider( BaseObject *parent, float x, float y, float w, float h, bool horizontal )
	{
		Slider *inst = new Slider( parent, x, y, w, h, horizontal );
		if ( sliderTemplate )
		{
			setBaseObjectProperties( inst, sliderTemplate );
			setSliderProperties( inst, sliderTemplate );
		}
		return inst;
	}

	ListItem * Factory::newListItem( ListBox *parent, std::string caption, int position )
	{
		ListItem *inst = new ListItem( 0, 0, 1, 1, caption );
		if ( listItemTemplate )
		{
			setBaseObjectProperties( inst, listItemTemplate );
			setLabelProperties( inst, listItemTemplate );
			setButtonProperties( inst, listItemTemplate );
			setSwitchButtonProperties( inst, listItemTemplate );
		}
		if ( parent )
			parent->addItem( inst, position );

		return inst;
	}

	ListBox * Factory::newListBox( BaseObject *parent, float x, float y, float w, float h, bool horizontal )
	{
		ListBox *inst = new ListBox( parent, x, y, w, h, horizontal );
		if ( listBoxTemplate )
		{
			setBaseObjectProperties( inst, listBoxTemplate );
			setListBoxProperties( inst, listBoxTemplate );
		}
		return inst;
	}

	DropDownBox * Factory::newDropDownBox( BaseObject *parent, float x, float y, float w, float h, float listh, std::string title )
	{
		DropDownBox *inst = new DropDownBox( parent, x, y, w, h, listh, title );
		if( dropDownBoxTemplate )
		{
			setBaseObjectProperties( inst, dropDownBoxTemplate );
			setListBoxProperties( inst, dropDownBoxTemplate );
		}
		return inst;
	}

	MessageBox * Factory::newMessageBox( std::string caption, int buttons, MessageBoxImage image )
	{
		MessageBox *inst = new MessageBox( caption, buttons, image );
		if( panelTemplate )
			setBaseObjectProperties( inst, panelTemplate );
		return inst;
	}

	MessageBox * Factory::newMessageBox( void )
	{
		MessageBox *inst = new MessageBox;
		if( panelTemplate )
			setBaseObjectProperties( inst, panelTemplate );
		return inst;
	}

	
	/****************************************/
	/*										*/
	/*			PRIVATE FUNCTIONS			*/
	/*										*/
	/****************************************/

	void Factory::setBaseObjectProperties( BaseObject *baseObject, const BaseObject *templateBaseObject )
	{
		baseObject->setBackColor( templateBaseObject->getBackColor() );
		baseObject->setBorderColor( templateBaseObject->getBorderColor() );
		baseObject->setDisabledColor( templateBaseObject->getDisabledColor() );
		baseObject->setBackgroundImage( templateBaseObject->getBackgroundImage() );
		//baseObject->setDisabledImage( templateBaseObject->getDisabledImage() );
		baseObject->setBorderWidth( templateBaseObject->getBorderWidth() );
	}

	void Factory::setLabelProperties( Label *label, const Label *templateLabel )
	{
		label->setFont( templateLabel->getFont() );
		label->setFontColor( templateLabel->getFontColor() );
		label->setDisabledFontColor( templateLabel->getDisabledFontColor() );
		label->setAlign( templateLabel->getAlign() );
	}

	void Factory::setButtonProperties( Button *button, const Button *templateButton )
	{
		button->setHighlightColor( templateButton->getHighlightColor() );
		button->setHighlightImage( templateButton->getHighlightImage() );
	}

	void Factory::setSwitchButtonProperties( SwitchButton *switchButton, const SwitchButton *templateSwitchButton )
	{
		switchButton->setPressed( templateSwitchButton->isPressed() );
		switchButton->setPressedColor( templateSwitchButton->getPressedColor() );
		switchButton->setPressedFontColor( templateSwitchButton->getPressedFontColor() );
		switchButton->setPressedImage( templateSwitchButton->getPressedImage() );
	}

	void Factory::setTextBoxProperties( TextBox *textBox, const TextBox *templateTextBox )
	{
		textBox->setText( templateTextBox->getText() );
		textBox->setEditingColor( templateTextBox->getEditingColor() );
		textBox->setEditingFontColor( templateTextBox->getEditingFontColor() );
	}

	void Factory::setCheckBoxProperties( CheckBox *checkBox, const CheckBox *templateCheckBox )
	{
		checkBox->setFont( templateCheckBox->getFont() );
		checkBox->setFontColor( templateCheckBox->getFontColor() );
		checkBox->setDisabledFontColor( templateCheckBox->getDisabledFontColor() );
		checkBox->setAlign( templateCheckBox->getAlign() );
		checkBox->setHighlightColor( templateCheckBox->getHighlightColor() );
		checkBox->setHighlightImage( templateCheckBox->getHighlightImage() );
		checkBox->setPressed( templateCheckBox->isPressed() );
		checkBox->setPressedColor( templateCheckBox->getPressedColor() );
		checkBox->setPressedImage( templateCheckBox->getPressedImage() );
		checkBox->setLabelBackColor( templateCheckBox->getLabelBackColor() );
	}

	void Factory::setSliderProperties( Slider *slider, const Slider *templateSlider )
	{
		slider->setMinValue( templateSlider->getMinValue() );
		slider->setMaxValue( templateSlider->getMaxValue() );
		slider->setCurValue( templateSlider->getCurValue() );
		slider->setButtonStepValue( templateSlider->getButtonStepValue() );
		slider->setBarClickStepValue( templateSlider->getBarClickStepValue() );

		// Buttons
		slider->setButtonBackColor( templateSlider->getButtonBackColor() );
		slider->setButtonBorderColor( templateSlider->getButtonBorderColor() );
		slider->setButtonDisabledColor( templateSlider->getButtonDisabledColor() );
		slider->setButtonBackgroundImage( templateSlider->getButtonBackgroundImage() );
		slider->setButtonDisabledImage( templateSlider->getDisabledImage() );
		slider->setButtonBorderWidth( templateSlider->getBorderWidth() );
		slider->setButtonHighlightColor( templateSlider->getButtonHighlightColor() );
		slider->setButtonHighlightImage( templateSlider->getButtonHighlightImage() );

		// Cursor
		slider->setCursorBackColor( templateSlider->getCursorBackColor() );
		slider->setCursorBorderColor( templateSlider->getCursorBorderColor() );
		slider->setCursorDisabledColor( templateSlider->getCursorDisabledColor() );
		slider->setCursorBackgroundImage( templateSlider->getCursorBackgroundImage() );
		slider->setCursorDisabledImage( templateSlider->getCursorDisabledImage() );
		slider->setCursorBorderWidth( templateSlider->getCursorBorderWidth() );
		slider->setCursorHighlightColor( templateSlider->getCursorHighlightColor() );
		slider->setCursorHighlightImage( templateSlider->getCursorHighlightImage() );
	}

	void Factory::setListBoxProperties( ListBox *listBox, const ListBox *templateListBox )
	{
		listBox->setMultipleSelection( templateListBox->getMultipleSelection() );
		listBox->setScrollbarSize( templateListBox->getScrollbarSize() );
		listBox->setItemSize( templateListBox->getItemSize() );
	}

	void Factory::setDropDownBoxProperties( DropDownBox *, const DropDownBox * )
	{

	}
}
