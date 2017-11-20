#!/usr/bin/env python
import scrapy, sys

class HabraProfileParser(scrapy.Spider):
	name = 'profile'

	def __init__(self, url=''):
		self.start_urls = [
			url
		]

	def parse(self, response):
		for comment in response.css('.content-list__item_comment-plain'):
			messages = comment.css('.comment__message::text').extract()
			if messages:
				yield { 'comment': "\n".join(messages).encode('utf-8').strip() }

		next_page = response.css('.arrows-pagination__item-link_next::attr("href")').extract_first()
		if next_page is not None:
			next_page = response.urljoin(next_page)
			yield scrapy.Request(next_page, callback = self.parse)