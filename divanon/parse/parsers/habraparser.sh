#!/bin/sh
# usage: habraparser.sh {habrahabr|geektimes} username
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

PATH="$PATH:~/.local/bin"
scrapy runspider -a url=https://$1.ru/users/$2/comments/ -o data/scrapy_out/$1_$2.xml -t xml parse/parsers/habraparser.py 2>>error.log
