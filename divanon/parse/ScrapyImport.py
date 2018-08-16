#!/usr/bin/env python
#
#    divanon: the deanonymizer
#    Copyright (C) 2018  Bohdan "bodqhrohro" Horbeshko
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
#


import xml.etree.ElementTree as ET
from lxml import html
from lxml.html.clean import Cleaner
import re

class ScrapyImport:
	@staticmethod
	def file_to_array(filename):
		tree = ET.parse(filename)
		root = tree.getroot()
		return [item[0].text for item in root]

	@staticmethod
	def _text_cleanup(text):
		global cleaner
		raw_html = cleaner.clean_html(text)
		# TODO: remove links
		# TODO: remove code
		# TODO: remove Latin
		# TODO: remove garbage
		# TODO: replace Latin omographs with Cyrillic
		# TODO: unify qmarks
		if len(raw_html) > 0:
			unwrap_regex = re.compile(r'\A<div>(.*?)</div>\Z', flags=re.MULTILINE | re.DOTALL)
			cut_html = unwrap_regex.match(raw_html).group(1)
			return cut_html
		else:
			return None

	@staticmethod
	def texts_cleanup(texts):
		return list(filter(lambda s: len(s) > 0, map(ScrapyImport._text_cleanup, texts)))

cleaner = Cleaner(allow_tags=[''], remove_unknown_tags=None, kill_tags=['code', 'blockquote', 's', 'strike'])
