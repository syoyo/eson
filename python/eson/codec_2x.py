# -*- coding: utf-8 -*-

# ESON codec
# Copyright (c) 2015 Light Transport Entertainment Inc.
#
# based on BSON python: https://github.com/martinkou/bson
# Copyright (c) 2010, Kou Man Tong. All rights reserved.
# For licensing, see LICENSE file included in the package.
"""
Base codec functions for eson.
"""
import struct
import cStringIO
import warnings
from abc import ABCMeta, abstractmethod

# {{{ Error Classes
class MissingClassDefinition(ValueError):
	def __init__(self, class_name):
		super(MissingClassDefinition, self).__init__(
		"No class definition for class %s" % (class_name,))
# }}}
# {{{ Warning Classes
class MissingTimezoneWarning(RuntimeWarning):
	def __init__(self, *args):
		args = list(args)
		if len(args) < 1:
			args.append("Input datetime object has no tzinfo, assuming UTC.")
		super(MissingTimezoneWarning, self).__init__(*args)
# }}}
# {{{ Traversal Step
class TraversalStep(object):
	def __init__(self, parent, key):
		self.parent = parent
		self.key = key
# }}}
# {{{ Custom Object Codec

class ESONCoding(object):
	__metaclass__ = ABCMeta

	@abstractmethod
	def eson_encode(self):
		pass

	@abstractmethod
	def eson_init(self, raw_values):
		pass

classes = {}

def import_class(cls):
	if not issubclass(cls, ESONCoding):
		return

	global classes
	classes[cls.__name__] = cls

def import_classes(*args):
	for cls in args:
		import_class(cls)

def import_classes_from_modules(*args):
	for module in args:
		for item in module.__dict__:
			if hasattr(item, "__new__") and hasattr(item, "__name__"):
				import_class(item)

def encode_object(obj, traversal_stack, generator_func):
	values = obj.eson_encode()
	class_name = obj.__class__.__name__
	values["$$__CLASS_NAME__$$"] = class_name
	return encode_document(values, traversal_stack, obj, generator_func)

def encode_object_element(name, value, traversal_stack, generator_func):
	return "\x07" + encode_cstring(name) + encode_object( value, traversal_stack, generator_func = generator_func )

class _EmptyClass(object):
	pass

def decode_object(raw_values):
	global classes
	class_name = raw_values["$$__CLASS_NAME__$$"]
	cls = None
	try:
		cls = classes[class_name]
	except KeyError, e:
		raise MissingClassDefinition(class_name)

	retval = _EmptyClass()
	retval.__class__ = cls
	alt_retval = retval.eson_init(raw_values)
	return alt_retval or retval

# }}}
# {{{ Codec Logic
def encode_string(value):
	value = value.encode("utf8")
	length = len(value)
	return struct.pack("<q%dsB" % (length,), length + 1, value, 0)

def decode_string(data, base):
	length = struct.unpack("<q", data[base:base + 8])[0]
	value = data[base + 8: base + 8 + length - 1]
	value = value.decode("utf8")
	return (base + 8 + length, value)

def encode_cstring(value):
	if isinstance(value, unicode):
		value = value.encode("utf8")
	return value + "\x00"

def decode_cstring(data, base):
	length = 0
	max_length = len(data) - base
	while length < max_length:
		character = data[base + length]
		length += 1
		if character == "\x00":
			break
	return (base + length, data[base:base + length - 1].decode("utf8"))

def encode_binary(value):
	length = len(value)
	return struct.pack("<q", length) + value

def decode_binary(data, base):
	length, binary_type = struct.unpack("<qB", data[base:base + 9])
	return (base + 8 + length, data[base + 8:base + 8 + length])

def encode_double(value):
	return struct.pack("<d", value)

def decode_double(data, base):
	return (base + 8, struct.unpack("<d", data[base: base + 8])[0])


ELEMENT_TYPES = {
		0x01 : "double",
		0x02 : "int64",
		0x03 : "byte",
		0x04 : "string",
		0x05 : "int64 document",
		0x06 : "binary",
        0x07 : "document",
	}

def encode_value(name, value, buf, traversal_stack, generator_func):
	if isinstance(value, ESONCoding):
		buf.write(encode_object_element(name, value, traversal_stack, generator_func))
	elif isinstance(value, float):
		buf.write(encode_double_element(name, value))
	elif isinstance(value, unicode):
		buf.write(encode_string_element(name, value))
	elif isinstance(value, dict):
		buf.write(encode_document_element(name, value,
			traversal_stack, generator_func))
	elif isinstance(value, list) or isinstance(value, tuple):
		buf.write(encode_array_element(name, value,
			traversal_stack, generator_func))
	elif isinstance(value, str):
		buf.write(encode_binary_element(name, value))
	elif isinstance(value, int):
		buf.write(encode_int64_element(name, value))
	elif isinstance(value, long):
		buf.write(encode_int64_element(name, value))

def encode_array(array, traversal_stack,
		traversal_parent = None,
		generator_func = None):
	buf = cStringIO.StringIO()
	for i in xrange(0, len(array)):
		value = array[i]
		traversal_stack.append(TraversalStep(traversal_parent or array, i))
		encode_value(unicode(i), value, buf, traversal_stack, generator_func)
		traversal_stack.pop()
	e_list = buf.getvalue()
	e_list_length = len(e_list)
	return struct.pack("<q%dsB" % (e_list_length,), e_list_length + 8 + 1, e_list, 0 )

def encode_array_element(name, value, traversal_stack, generator_func):
	return "\x05" + encode_cstring(name) + encode_array(value, traversal_stack, generator_func = generator_func)

def encode_double_element(name, value):
	return "\x01" + encode_cstring(name) + encode_double(value)

def decode_double_element(data, base):
	base, name = decode_cstring(data, base + 1)
	base, value = decode_double(data, base)
	return (base, name, value)

def encode_int64_element(name, value):
	return "\x02" + encode_cstring(name) + struct.pack("<q", value)

def decode_int64_element(data, base):
	base, name = decode_cstring(data, base + 1)
	value = struct.unpack("<q", data[base:base + 8])[0]
	return (base + 8, name, value)

def encode_byte_element(name, value):
	return "\x03" + encode_cstring(name) + struct.pack("<B", value)

def decode_byte_element(data, base):
	base, name = decode_cstring(data, base + 1)
	value = struct.unpack("<B", data[base:base + 1])[0]
	return (base + 1, name, value)

def encode_string(value):
	value = value.encode("utf8")
	length = len(value)
	return struct.pack("<q%dsB" % (length,), length + 1, value, 0)

def decode_string(data, base):
	length = struct.unpack("<q", data[base:base + 8])[0]
	value = data[base + 8: base + 8 + length - 1]
	value = value.decode("utf8")
	return (base + 8 + length, value)

def encode_string_element(name, value):
	return "\x04" + encode_cstring(name) + encode_string(value)

def decode_string_element(data, base):
	base, name = decode_cstring(data, base + 1)
	base, value = decode_string(data, base)
	return (base, name, value)

def encode_document(obj, traversal_stack,
		traversal_parent = None,
		generator_func = None):
	buf = cStringIO.StringIO()
	key_iter = obj.iterkeys()
	if generator_func is not None:
		key_iter = generator_func(obj, traversal_stack)
	for name in key_iter:
		value = obj[name]
		traversal_stack.append(TraversalStep(traversal_parent or obj, name))
		encode_value(name, value, buf, traversal_stack, generator_func)
		traversal_stack.pop()
	e_list = buf.getvalue()
	e_list_length = len(e_list)
	return struct.pack("<q%dsB" % (e_list_length,), e_list_length + 8, e_list, 0)

def decode_document(data, base):
	length = struct.unpack("<q", data[base:base + 8])[0]
	end_point = base + length
	base += 8
	retval = {}
	while base < end_point - 1:
		base, name, value = decode_element(data, base)
		retval[name] = value
	if "$$__CLASS_NAME__$$" in retval:
		retval = decode_object(retval)
	return (end_point, retval)

def decode_element(data, base):
	element_type = struct.unpack("<B", data[base:base + 1])[0]
	element_description = ELEMENT_TYPES[element_type]
	decode_func = globals()["decode_" + element_description + "_element"]
	return decode_func(data, base)


def encode_document_element(name, value, traversal_stack, generator_func):
	return "\x07" + encode_cstring(name) + \
			encode_document(value, traversal_stack,
					generator_func = generator_func)

def decode_document_element(data, base):
	base, name = decode_cstring(data, base + 1)
	base, value = decode_document(data, base)
	return (base, name, value)

def encode_array_element(name, value, traversal_stack, generator_func):
	return "\x05" + encode_cstring(name) + encode_array(value, traversal_stack, generator_func = generator_func)

def encode_binary_element(name, value):
	return "\x06" + encode_cstring(name) + encode_binary(value)

def decode_binary_element(data, base):
	base, name = decode_cstring(data, base + 1)
	base, value = decode_binary(data, base)
	return (base, name, value)

