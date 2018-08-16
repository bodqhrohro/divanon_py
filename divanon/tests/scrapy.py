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


import unittest
import os,logging
from divanon.parse.ScrapyImport import ScrapyImport

OUT_DIR = 'data/scrapy_out'

logging.basicConfig(level=logging.DEBUG)
class TestScrapyImport(unittest.TestCase):
    def setUp(self):
        pass

    def test_habr_read(self):
        all_outputs = os.listdir(OUT_DIR)
        habra_outputs = filter(lambda name: name.startswith('habr'), all_outputs)
        for habrafile in habra_outputs:
            try:
                posts = ScrapyImport.texts_cleanup(ScrapyImport.file_to_array(OUT_DIR + '/' + habrafile))
            except IOError:
                logging.error(IOError)
                die
            for post_i, post in enumerate(posts):
                try:
                    self.assertNotEqual(post, "")
                    self.assertNotEqual(post, ">")
                except AssertionError:
                    logging.error(posts[post_i-1] + "\n" + post + "\n" + posts[post_i+1])
                    raise

if __name__ == '__main__':
    unittest.main()
    logging.debug('tist')
