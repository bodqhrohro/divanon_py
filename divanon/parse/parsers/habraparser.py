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

import scrapy, sys
from lxml import html

class HabraProfileParser(scrapy.Spider):
	name = 'profile'

	def __init__(self, url=''):
		self.start_urls = [
			url
		]

	def parse(self, response):
		for comment in response.css('.content-list__item_comment-plain'):
			messages = comment.css('.comment__message')
			if len(messages) > 0:
				message = messages[0]._root
				yield { 'comment': (message.text or '') + str("\n".join([str(html.tostring(child)) for child in message.iterchildren()]).encode('utf-8').strip()) }

		next_page = response.css('.arrows-pagination__item-link_next::attr("href")').extract_first()
		if next_page is not None:
			next_page = response.urljoin(next_page)
			yield scrapy.Request(next_page, callback = self.parse)
