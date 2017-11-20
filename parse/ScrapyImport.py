#!/usr/bin/env python

import xml.etree.ElementTree as ET

class ScrapyImport:
	@staticmethod
	def file_to_array(filename):
		tree = ET.parse(filename)
		root = tree.getroot()
		return [item[0].text for item in root]
