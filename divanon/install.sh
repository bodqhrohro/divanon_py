#!/bin/sh
mkdir -p data/scrapy_out
pip3 install -r requirements.txt
python3 -m nltk.downloader punkt -d nltk_data
ln -fs "$(pwd)"/nltk_data/ /usr/local/share/nltk_data

cd vendor/cascor/v110
make
