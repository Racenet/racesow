#!/usr/bin/python 


# goofos at epruegel.de 
# 
# ***** BEGIN GPL LICENSE BLOCK ***** 
# 
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either version 2 
# of the License, or (at your option) any later version. 
# 
# This program is distributed in the hope that it will be useful, 
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details. 
# 
# You should have received a copy of the GNU General Public License 
# along with this program; if not, write to the Free Software Foundation, 
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 
# 
# ***** END GPL LICENCE BLOCK ***** 
#
# modified by torbenh

import string, time, sys 

from numpy import *
import cairo


def read_main(filename): 

   global counts 
   counts = {'verts': 0, 'tris': 0} 

   start = time.clock() 
   file = open(filename, "r") 

   print("----------------start-----------------") 
   print 'Import Patch: ', filename 


   lines= file.readlines() 
   retval = Ase(lines) 

   file.close() 
   print "----------------end-----------------" 
   end = time.clock() 
   seconds = " in %.2f %s" % (end-start, "seconds") 
   print "Successfully imported " + filename + seconds 

   return retval


class ase_obj: 

   def __init__(self): 
      self.name = 'Name' 
      self.objType = None 
      self.row0x = None 
      self.row0y = None 
      self.row0z = None 
      self.row1x = None 
      self.row1y = None 
      self.row1z = None 
      self.row2x = None 
      self.row2y = None 
      self.row2z = None 
      self.row3x = None 
      self.row3y = None 
      self.row3z = None 
      self.parent = None 
      self.obj = None 
      self.objName = 'Name' 

class ase_mesh: 

   def __init__(self): 
      self.name = '' 
      self.vCount = 0 
      self.fCount = 0 
      self.uvVCount = 0 
      self.uvFCount = 0 
      self.vcVCount = 0 
      self.vcFCount = 0 
      self.meVerts = [] 
      self.meFaces = [] 
      self.uvVerts = [] 
      self.uvFaces = [] 
      self.vcVerts = [] 
      self.vcFaces = [] 
      self.hasFUV = 0 
      self.hasVC = 0 

class mesh_face: 

   def __init__(self): 
      self.v1 = array(0.0,0.0,0.0) 
      self.v2 = array(0.0,0.0,0.0)  
      self.v3 = array(0.0,0.0,0.0)  
      self.mat = None 
        
class mesh_vert: 

   def __init__(self): 
      self.v = array(0.0, 0.0, 0.0)

class mesh_uvVert: 

   def __init__(self): 
      self.index = 0 
      self.u = 0.0 
      self.v = 0.0 

class mesh_uvFace: 

   def __init__(self): 
      self.index = 0 
      self.uv1 = 0 
      self.uv2 = 0 
      self.uv3 = 0 
        
class mesh_vcVert: 

   def __init__(self): 
      self.index = 0 
      self.r = 0 
      self.g = 0 
      self.b = 0 
      self.a = 255 
        
class mesh_vcFace: 

   def __init__(self): 
      self.index = 0 
      self.c1 = 0 
      self.c2 = 0 
      self.c3 = 0 


def minsfeed( mins, v ):
	mins[0] = min( mins[0], v[0] )
	mins[1] = min( mins[1], v[1] )
	mins[2] = min( mins[2], v[2] )

def maxsfeed( maxs, v ):
	maxs[0] = max( maxs[0], v[0] )
	maxs[1] = max( maxs[1], v[1] )
	maxs[2] = max( maxs[2], v[2] )

class Ase:

	def __init__(self, lines): 

		objects = [] 
		objIdx = 0 
		objCheck = -1 #needed to skip helper objects 
		PBidx = 0.0 
		lineCount = float(len(lines)) 
		self.maxs = array( [0,0,0] )
		self.mins = array( [0,0,0] )

		print 'Read file' 

		for line in lines: 
			words = string.split(line) 


			if not words: 
				continue 

			if words[0] == '*GEOMOBJECT': 
				objCheck = 0 
				newObj = ase_obj() 
				objects.append(newObj) 
				obj = objects[objIdx] 
				objIdx += 1 
			elif words[0] == '*NODE_NAME' and objCheck != -1: 
				if objCheck == 0: 
					obj.name = words[1] 
					objCheck = 1 
				elif objCheck == 1: 
					obj.objName = words[1] 
			elif words[0] == '*TM_ROW0' and objCheck != -1: 
				obj.row0x = float(words[1]) 
				obj.row0y = float(words[2]) 
				obj.row0z = float(words[3]) 
			elif words[0] == '*TM_ROW1' and objCheck != -1: 
				obj.row1x = float(words[1]) 
				obj.row1y = float(words[2]) 
				obj.row1z = float(words[3]) 
			elif words[0] == '*TM_ROW2' and objCheck != -1: 
				obj.row2x = float(words[1]) 
				obj.row2y = float(words[2]) 
				obj.row2z = float(words[3]) 
			elif words[0] == '*TM_ROW3' and objCheck != -1: 
				obj.row3x = float(words[1]) 
				obj.row3y = float(words[2]) 
				obj.row3z = float(words[3]) 
				objCheck = -1 
			elif words[0] == '*MESH': 
				obj.objType = 'Mesh' 
				obj.obj = ase_mesh() 
				me = obj.obj 
			elif words[0] == '*MESH_NUMVERTEX': 
				me.vCount = int(words[1]) 
			elif words[0] == '*MESH_NUMFACES': 
				me.fCount = int(words[1]) 
			elif words[0] == '*MESH_VERTEX': 
				v = array( [float(words[2]),float(words[3]),float(words[4])] )
				maxsfeed( self.maxs, v )
				minsfeed( self.mins, v )

				me.meVerts.append(v) 
			elif words[0] == '*MESH_FACE': 
				f = [me.meVerts[int(words[3])],me.meVerts[int(words[5])],me.meVerts[int(words[7])]] 
				me.meFaces.append(f) 
			elif words[0] == '*MESH_NUMTVERTEX': 
				me.uvVCount = int(words[1]) 
				if me.uvVCount > 0: 
					me.hasFUV = 1 
			elif words[0] == '*MESH_TVERT': 
				uv = mesh_uvVert() 
				uv.index = int(words[1]) 
				uv.u = float(words[2]) 
				uv.v = float(words[3]) 
				me.uvVerts.append(uv) 
			elif words[0] == '*MESH_NUMTVFACES': 
				me.uvFCount = int(words[1]) 
			elif words[0] == '*MESH_TFACE': 
				fUv = mesh_uvFace() 
				fUv.index = int(words[1]) 
				fUv.uv1 = int(words[2]) 
				fUv.uv2 = int(words[3]) 
				fUv.uv3 = int(words[4]) 
				me.uvFaces.append(fUv) 
			elif words[0] == '*MESH_NUMCVERTEX': 
				me.vcVCount = int(words[1]) 
				if me.uvVCount > 0: 
					me.hasVC = 1 
			elif words[0] == '*MESH_VERTCOL': 
				c = mesh_vcVert() 
				c.index = int(words[1]) 
				c.r = round(float(words[2])*256) 
				c.g = round(float(words[3])*256) 
				c.b = round(float(words[4])*256) 
				me.vcVerts.append(c) 
			elif words[0] == '*MESH_CFACE': 
				fc = mesh_vcFace() 
				fc.index = int(words[1]) 
				fc.c1 = int(words[2]) 
				fc.c2 = int(words[3]) 
				fc.c3 = int(words[4]) 
				me.vcFaces.append(fc) 

		self.objects = objects


ase= read_main( sys.argv[1]  )
print "maxs:",  ase.maxs
print "mins:",  ase.mins

extends=ase.maxs - ase.mins

w=256
h=256

surface = cairo.ImageSurface (cairo.FORMAT_ARGB32, w, h)
crx = cairo.Context( surface )

scale = array( [w,h,0] )

for o in ase.objects:
	for face in o.obj.meFaces:
		v1 = face[0]
		v2 = face[1]
		v3 = face[2]

		normal = cross( v2-v1, v3-v1 )
		normal /= linalg.norm( normal )
		
		if( normal[2] > 0.9 ):
			u1=(v1-ase.mins)/extends*scale
			u2=(v2-ase.mins)/extends*scale
			u3=(v3-ase.mins)/extends*scale

			crx.move_to( u1[0], h-u1[1] )
			crx.line_to( u2[0], h-u2[1] )
			crx.line_to( u3[0], h-u3[1] )
			crx.close_path()
			crx.set_source_rgba(1.0,1.0,1.0,0.5)
			crx.fill()



surface.write_to_png (sys.argv[2])




