#!/usr/bin/env python

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
	def _post_cleanup(post):
		global cleaner
		raw_html = cleaner.clean_html(post)
		if len(raw_html) > 0:
			unwrap_regex = re.compile(r'\A<div>(.*?)</div>\Z', flags=re.MULTILINE | re.DOTALL)
			cut_html = unwrap_regex.match(raw_html).group(1)
			return cut_html
		else:
			return None

	@staticmethod
	def posts_cleanup(posts):
		return list(filter(lambda s: len(s) > 0, map(ScrapyImport._post_cleanup, posts)))


cleaner = Cleaner(allow_tags=[''], remove_unknown_tags=None, kill_tags=['code', 'blockquote', 's', 'strike'])
