#!/usr/bin/env python

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
            except Error:
                logging.error(Error)
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
