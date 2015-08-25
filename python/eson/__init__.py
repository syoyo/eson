# -*- coding: utf-8 -*-

import sys

if( sys.version_info.major == 2 ):
	from .codec_2x import *
	reload( codec_2x )
elif( sys.version_info.major == 3 ):
	from .codec_3x import *
	import imp
	imp.reload( codec_3x )
	

__all__ = ['loads', 'dumps']

def dumps( obj, generator = None ):
	if isinstance( obj, ESONCoding ):
		return encode_object(obj, [], generator_func = generator)
	return encode_document(obj, [], generator_func = generator)

def load( data ):
	return decode_document( data, 0 )[1]

