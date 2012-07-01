/*
Copyright (C) 2008 German Garcia
Copyright (C) 2011 Chasseur de bots

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

#include "../qas_local.h"
#include "addon_string.h"

static inline asstring_t *objectString_Alloc( void )
{
	static asstring_t *object;

	object = (asstring_t *)QAS_Malloc( sizeof( asstring_t ) );
	object->asRefCount = 1;
	return object;
}

asstring_t *objectString_FactoryBuffer( const char *buffer, unsigned int length )
{
	asstring_t *object;

	object = objectString_Alloc();
	object->buffer = (char *)QAS_Malloc( sizeof( char ) * ( length + 1 ) );
	object->len = length;
	object->buffer[length] = 0;
	object->size = length + 1;
	if( buffer )
		Q_strncpyz( object->buffer, buffer, object->size );

	return object;
}

asstring_t *objectString_AssignString( asstring_t *self, const char *string, size_t strlen )
{
	if( strlen >= self->size )
	{
		QAS_Free( self->buffer );

		self->size = strlen + 1;
		self->buffer = (char *)QAS_Malloc( self->size );
	}

	self->len = strlen;
	Q_strncpyz( self->buffer, string, self->size );

	return self;
}

static asstring_t *objectString_AssignPattern( asstring_t *self, const char *pattern, ... )
{
	va_list	argptr;
	static char buf[4096];

	va_start( argptr, pattern );
	Q_vsnprintfz( buf, sizeof( buf ), pattern, argptr );
	va_end( argptr );

	return objectString_AssignString( self, buf, strlen( buf ) );
}

static asstring_t *objectString_AddAssignString( asstring_t *self, const char *string, size_t strlen )
{
	if( strlen )
	{
		char *tem = self->buffer;

		self->len = strlen + self->len;
		self->size = self->len + 1;
		self->buffer = (char *)QAS_Malloc( self->size );

		Q_snprintfz( self->buffer, self->size, "%s%s", tem, string );
		QAS_Free( tem );
	}

	return self;
}

static asstring_t *objectString_AddAssignPattern( asstring_t *self, const char *pattern, ... )
{
	va_list	argptr;
	static char buf[4096];

	va_start( argptr, pattern );
	Q_vsnprintfz( buf, sizeof( buf ), pattern, argptr );
	va_end( argptr );

	return objectString_AddAssignString( self, buf, strlen( buf ) );
}

static asstring_t *objectString_AddString( asstring_t *first, const char *second, size_t seclen )
{
	asstring_t *self = objectString_FactoryBuffer( NULL, first->len + seclen );

	Q_snprintfz( self->buffer, self->size, "%s%s", first->buffer, second );

	return self;
}

static asstring_t *objectString_AddPattern( asstring_t *first, const char *pattern, ... )
{
	va_list	argptr;
	static char buf[4096];

	va_start( argptr, pattern );
	Q_vsnprintfz( buf, sizeof( buf ), pattern, argptr );
	va_end( argptr );

	return objectString_AddString( first, buf, strlen( buf ) );
}

static asstring_t *objectString_Factory( void )
{
	return objectString_FactoryBuffer( NULL, 0 );
}

static asstring_t *objectString_FactoryCopy( const asstring_t *other )
{
	return objectString_FactoryBuffer( other->buffer, other->len );
}

static asstring_t *objectString_FactoryFromInt( int other )
{
	asstring_t *obj;
	obj = objectString_FactoryBuffer( NULL, 0 );
	return objectString_AssignPattern( obj, "%i", other );
}

static asstring_t *objectString_FactoryFromFloat( float other )
{
	asstring_t *obj;
	obj = objectString_FactoryBuffer( NULL, 0 );
	return objectString_AssignPattern( obj, "%f", other );
}

static asstring_t *objectString_FactoryFromDouble( double other )
{
	asstring_t *obj;
	obj = objectString_FactoryBuffer( NULL, 0 );
	return objectString_AssignPattern( obj, "%g", other );
}

static void objectString_Addref( asstring_t *obj ) { obj->asRefCount++; }

void objectString_Release( asstring_t *obj )
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );

	if( !obj->asRefCount )
	{
		QAS_Free( obj->buffer );
		QAS_Free( obj );
	}
}

static asstring_t *StringFactory( unsigned int length, const char *s )
{
	return objectString_FactoryBuffer( s, length );
}

static char *objectString_Index( unsigned int i, asstring_t *self )
{
	if( i > self->len )
	{
		assert( i > self->len );
		return NULL;
	}

	return &self->buffer[i];
}

static asstring_t *objectString_AssignBehaviour( asstring_t *other, asstring_t *self )
{
	return objectString_AssignString( self, other->buffer, other->len );
}

static asstring_t *objectString_AssignBehaviourI( int other, asstring_t *self )
{
	return objectString_AssignPattern( self, "%i", other );
}

static asstring_t *objectString_AssignBehaviourD( double other, asstring_t *self )
{
	return objectString_AssignPattern( self, "%g", other );
}

static asstring_t *objectString_AssignBehaviourF( float other, asstring_t *self )
{
	return objectString_AssignPattern( self, "%f", other );
}

static asstring_t *objectString_AddAssignBehaviourSS( asstring_t *other, asstring_t *self )
{
	return objectString_AddAssignString( self, other->buffer, other->len );
}

static asstring_t *objectString_AddAssignBehaviourSI( int other, asstring_t *self )
{
	return objectString_AddAssignPattern( self, "%i", other );
}

static asstring_t *objectString_AddAssignBehaviourSD( double other, asstring_t *self )
{
	return objectString_AddAssignPattern( self, "%g", other );
}

static asstring_t *objectString_AddAssignBehaviourSF( float other, asstring_t *self )
{
	return objectString_AddAssignPattern( self, "%f", other );
}

static asstring_t *objectString_AddBehaviourSS( asstring_t *first, asstring_t *second )
{
	return objectString_AddString( first, second->buffer, second->len );
}

static asstring_t *objectString_AddBehaviourSI( asstring_t *first, int second )
{
	return objectString_AddPattern( first, "%i", second );
}

static asstring_t *objectString_AddBehaviourIS( int first, asstring_t *second )
{
	asstring_t *res = objectString_Factory();
	return objectString_AssignPattern( res, "%i%s", first, second->buffer );
}

static asstring_t *objectString_AddBehaviourSD( asstring_t *first, double second )
{
	return objectString_AddPattern( first, "%g", second );
}

static asstring_t *objectString_AddBehaviourDS( double first, asstring_t *second )
{
	asstring_t *res = objectString_FactoryBuffer( NULL, 0 );
	return objectString_AssignPattern( res, "%g%s", first, second->buffer );
}

static asstring_t *objectString_AddBehaviourSF( asstring_t *first, float second )
{
	return objectString_AddPattern( first, "%f", second );
}

static asstring_t *objectString_AddBehaviourFS( float first, asstring_t *second )
{
	asstring_t *res = objectString_FactoryBuffer( NULL, 0 );
	return objectString_AssignPattern( res, "%f%s", first, second->buffer );
}

static bool objectString_EqualBehaviour( asstring_t *first, asstring_t *second )
{
	if( !first->len && !second->len )
		return true;

	return ( Q_stricmp( first->buffer, second->buffer ) == 0 );
}

static int objectString_CastToInt( asstring_t *self )
{
	return atoi( self->buffer );
}

static float objectString_CastToFloat( asstring_t *self )
{
	return atof( self->buffer );
}

static double objectString_CastToDouble( asstring_t *self )
{
	return atof( self->buffer );
}

// ==================================================================================

static int objectString_Len( asstring_t *self )
{
	return self->len;
}

static bool objectString_Empty( asstring_t *self )
{
	return self->len == 0;
}

static asstring_t *objectString_ToLower( asstring_t *self )
{
	asstring_t *string = objectString_FactoryBuffer( self->buffer, self->len );
	if( string->len )
		Q_strlwr( string->buffer );
	return string;
}

static asstring_t *objectString_ToUpper( asstring_t *self )
{
	asstring_t *string = objectString_FactoryBuffer( self->buffer, self->len );
	if( string->len )
		Q_strupr( string->buffer );
	return string;
}

static asstring_t *objectString_Trim( asstring_t *self )
{
	asstring_t *string = objectString_FactoryBuffer( self->buffer, self->len );
	if( string->len )
		Q_trim( string->buffer );
	return string;
}

static unsigned int objectString_Locate( asstring_t *substr, unsigned int skip, asstring_t *self )
{
	unsigned int i;
	char *p, *s;

	if( !self->len ) {
		return 0;
	}
	if( !substr->len ) {
		return self->len;
	}

	p = NULL;
	for( i = 0, s = self->buffer; i <= skip; i++, s = p + substr->len )
	{
		if( !(p = strstr( s, substr->buffer )) )
			break;
	}

	if( p ) {
		return (int)( p - self->buffer );
	}
	return self->len;
}

static asstring_t *objectString_Substring( int start, int length, asstring_t *self )
{
	if( start < 0 || length <= 0 )
		return objectString_FactoryBuffer( NULL, 0 );
	if( start >= (int)self->len )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->buffer + start, min( length, (int)self->len - start ) );
}

static asstring_t *objectString_Substring2( int start, asstring_t *self )
{
	if( start < 0 || start >= (int)self->len )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->buffer + start, (int)self->len - start );
}

static bool objectString_IsAlpha( asstring_t *self )
{
	size_t i;

	for( i = 0; i < self->len; i++ )
	{
		if( !isalpha( self->buffer[i] ) )
			return false;
	}
	return true;
}

static bool objectString_IsNumeric( asstring_t *self )
{
	size_t i;

	if( !self->buffer[0] )
		return false;

	for( i = 0; i < self->len; i++ )
	{
		if( !isdigit( self->buffer[i] ) )
			return false;
	}
	return true;
}

static bool objectString_IsAlphaNumerical( asstring_t *self )
{
	size_t i;

	for( i = 0; i < self->len; i++ )
	{
		if( !isalnum( self->buffer[i] ) )
			return false;
	}
	return true;
}

static asstring_t *objectString_RemoveColorTokens( asstring_t *self )
{
	const char *s;

	if( !self->len )
		return objectString_FactoryBuffer( NULL, 0 );

	s = COM_RemoveColorTokens( self->buffer );
	return objectString_FactoryBuffer( s, strlen(s) );
}

static int objectString_toInt( asstring_t *self )
{
	return atoi( self->buffer );
}

static float objectString_toFloat( asstring_t *self )
{
	return atof( self->buffer );
}

static asstring_t *objectString_getToken( unsigned int index, asstring_t *self )
{
	unsigned int i;
	char *s;
	const char *token = "";

	s = self->buffer;

	for( i = 0; i <= index; i++ )
	{
		token = COM_Parse( &s );
		if( !token[0] ) // string finished before finding the token
			break;
	}

	return objectString_FactoryBuffer( token, strlen( token ) );
}

void PreRegisterStringAddon( asIScriptEngine *engine )
{
	int r;

	// register the string type
	r = engine->RegisterObjectType( "String", sizeof( asstring_t ), asOBJ_REF ); assert( r >= 0 );
}

void RegisterStringAddon( asIScriptEngine *engine )
{
	int r;

	// register the string factory
	r = engine->RegisterStringFactory( "String @", asFUNCTION( StringFactory ), asCALL_CDECL ); assert( r >= 0 );

	// register object behaviours
	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_FACTORY, "String @f()", asFUNCTION( objectString_Factory ), asCALL_CDECL ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_FACTORY, "String @f(const String &in)", asFUNCTION( objectString_FactoryCopy ), asCALL_CDECL ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_FACTORY, "String @f(int)", asFUNCTION( objectString_FactoryFromInt ), asCALL_CDECL ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_FACTORY, "String @f(float)", asFUNCTION( objectString_FactoryFromFloat ), asCALL_CDECL ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_FACTORY, "String @f(double)", asFUNCTION( objectString_FactoryFromDouble ), asCALL_CDECL ); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_ADDREF, "void f()", asFUNCTION( objectString_Addref ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_RELEASE, "void f()", asFUNCTION( objectString_Release ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_IMPLICIT_VALUE_CAST, "int f() const", asFUNCTION( objectString_CastToInt ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_IMPLICIT_VALUE_CAST, "float f() const", asFUNCTION( objectString_CastToFloat ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "String", asBEHAVE_IMPLICIT_VALUE_CAST, "double f() const", asFUNCTION( objectString_CastToDouble ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );

	// register object methods

	// assignments
	r = engine->RegisterObjectMethod( "String", "String &opAssign(const String &in)", asFUNCTION( objectString_AssignBehaviour ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String &opAssign(int)", asFUNCTION( objectString_AssignBehaviourI ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String &opAssign(double)", asFUNCTION( objectString_AssignBehaviourD ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String &opAssign(float)", asFUNCTION( objectString_AssignBehaviourF ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );

	// register the index operator, both as a mutator and as an inspector
	r = engine->RegisterObjectMethod( "String", "uint8 &opIndex(uint)", asFUNCTION( objectString_Index ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "const uint8 &opIndex(uint) const", asFUNCTION( objectString_Index ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );

	// +=
	r = engine->RegisterObjectMethod( "String", "String &opAddAssign(const String &in)", asFUNCTION( objectString_AddAssignBehaviourSS ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String &opAddAssign(int)", asFUNCTION( objectString_AddAssignBehaviourSI ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String &opAddAssign(double)", asFUNCTION( objectString_AddAssignBehaviourSD ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String &opAddAssign(float)", asFUNCTION( objectString_AddAssignBehaviourSF ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );

	// +
	r = engine->RegisterObjectMethod( "String", "String @opAdd(const String &in) const", asFUNCTION( objectString_AddBehaviourSS ), asCALL_CDECL_OBJFIRST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @opAdd(int) const", asFUNCTION( objectString_AddBehaviourSI ), asCALL_CDECL_OBJFIRST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @opAdd_r(int) const", asFUNCTION( objectString_AddBehaviourIS ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @opAdd(double) const", asFUNCTION( objectString_AddBehaviourSD ), asCALL_CDECL_OBJFIRST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @opAdd_r(double) const", asFUNCTION( objectString_AddBehaviourDS ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @opAdd(float) const", asFUNCTION( objectString_AddBehaviourSF ), asCALL_CDECL_OBJFIRST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @opAdd_r(float) const", asFUNCTION( objectString_AddBehaviourFS ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );

	// ==
	r = engine->RegisterObjectMethod( "String", "bool opEquals(const String &in) const", asFUNCTION( objectString_EqualBehaviour ), asCALL_CDECL_OBJFIRST ); assert( r >= 0 );

	r = engine->RegisterObjectMethod( "String", "uint len() const", asFUNCTION( objectString_Len ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "uint length() const", asFUNCTION( objectString_Len ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "bool empty() const", asFUNCTION( objectString_Empty ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @tolower() const", asFUNCTION( objectString_ToLower ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @toupper() const", asFUNCTION( objectString_ToUpper ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @trim() const", asFUNCTION( objectString_Trim ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @removeColorTokens() const", asFUNCTION( objectString_RemoveColorTokens ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @getToken(const uint) const", asFUNCTION( objectString_getToken ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );

	r = engine->RegisterObjectMethod( "String", "int toInt() const", asFUNCTION( objectString_toInt ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "float toFloat() const", asFUNCTION( objectString_toFloat ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );

	r = engine->RegisterObjectMethod( "String", "uint locate(String &, const uint) const", asFUNCTION( objectString_Locate ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @substr(const uint start, const uint length) const", asFUNCTION( objectString_Substring ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @subString(const uint start, const uint length) const", asFUNCTION( objectString_Substring ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @substr(const uint start) const", asFUNCTION( objectString_Substring2 ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "String @subString(const uint start) const", asFUNCTION( objectString_Substring2 ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );

	r = engine->RegisterObjectMethod( "String", "bool isAlpha() const", asFUNCTION( objectString_IsAlpha ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "bool isNumerical() const", asFUNCTION( objectString_IsNumeric ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "bool isNumeric() const", asFUNCTION( objectString_IsNumeric ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "String", "bool isAlphaNumerical() const", asFUNCTION( objectString_IsAlphaNumerical ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );
}
