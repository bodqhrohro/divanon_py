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
import divanon.textstats.Morphologic as Morphologic

OUT_DIR = 'data/scrapy_out'

logging.basicConfig(level=logging.DEBUG)
class TestWordStats(unittest.TestCase):
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
                    for word in Morphologic._russian_words(Morphologic._text_to_words(post)):
                        self.assertNotEqual(word, "")
                except AssertionError:
                    logging.error(posts[post_i-1] + "\n" + post + "\n" + posts[post_i+1])
                    raise

    def test_PoS(self):
        dist = Morphologic.Morphologic.get_PoS_distribution("Частоты 500 наиболее частых слов из словаря Шарова, сглаженные методом Лапласа")
        expected_dict = {
            'NOUN': 6,
            'ADJF': 1,
            'PRTF': 1,
            'ADVB': 1,
            'PREP': 1,
        }
        accuracy = 0.5
        for grammem, score in dist.items():
            self.assertTrue(lambda: abs(score - expected_dict[grammem]) < accuracy)

    def test_russian_letters(self):
        self.assertTrue(Morphologic._is_letter_russian('б'))
        self.assertTrue(Morphologic._is_letter_russian('Ё'))
        self.assertFalse(Morphologic._is_letter_russian('Ї'))
        self.assertFalse(Morphologic._is_letter_russian('B'))
        self.assertFalse(Morphologic._is_letter_russian(' '))
        self.assertFalse(Morphologic._is_letter_russian('™'))
        self.assertFalse(Morphologic._is_letter_russian('-'))
        self.assertFalse(Morphologic._is_letter_russian('7'))

if __name__ == '__main__':
    unittest.main()
    logging.debug('tist')
