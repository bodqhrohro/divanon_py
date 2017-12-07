#!/bin/sh
# usage: habraparser.sh {habrahabr|geektimes} username
~/.local/bin/scrapy runspider -a url=https://$1.ru/users/$2/comments/ -o data/scrapy_out/$1_$2.xml -t xml parse/parsers/habraparser.py 2>>error.log
