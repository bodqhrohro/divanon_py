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

import nltk
import pymorphy2
from collections import defaultdict
import numpy as np

morph = pymorphy2.MorphAnalyzer()

PARTS_OF_SPEECH = ['PRED', 'PREP', 'CONJ', 'PRCL', 'INTJ']
RUSSIAN_LETTERS = [
    'а', 'б', 'в', 'г', 'д', 'е', 'ё', 'ж', 'з', 'и', 'й', 'к',
    'л', 'м', 'н', 'о', 'п', 'р', 'с', 'т', 'у', 'ф', 'х', 'ц',
    'ч', 'ш', 'щ', 'ъ', 'ы', 'ь', 'э', 'ю', 'я'
]


def _text_to_words(text):
    return nltk.word_tokenize(text)


def _russian_words(words):
    return filter(_is_word_russian, words)


def _is_word_russian(word):
    russian_letters_count = sum(_is_letter_russian(letter) for letter in word)
    return russian_letters_count > (len(word) / 2)


def _is_letter_russian(letter):
    global RUSSIAN_LETTERS
    return letter.lower() in RUSSIAN_LETTERS


class Morphologic:
    def get_PoS_distribution(text):
        PoS_dict = defaultdict(float)
        words = _text_to_words(text)
        russian_words = _russian_words(words)
        total_score = 0
        for word in russian_words:
            ps = morph.parse(word)
            for p in ps:
                if (p.tag.POS is not None):
                    PoS_dict[p.tag.POS] += float(p.score)
                    total_score += float(p.score)
        if total_score > 0:
            for PoS in PoS_dict.keys():
                PoS_dict[PoS] /= total_score
        return PoS_dict

    def PoS_dict_to_np_array(PoS_dict):
        global PARTS_OF_SPEECH
        return np.array(list([
            PoS_dict[PoS] if PoS in PoS_dict else 0 for PoS in PARTS_OF_SPEECH
        ]))

    def get_letter_distribution(text):
        letter_dict = defaultdict(float)
        words = _text_to_words(text)
        russian_words = _russian_words(words)
        total_score = 0
        for word in russian_words:
            for letter in word:
                if letter in RUSSIAN_LETTERS:
                    letter_dict[letter] += 1
                    total_score += 1
        if total_score > 0:
            for letter in letter_dict.keys():
                letter_dict[letter] /= total_score
        return letter_dict

    def letter_dict_to_np_array(letter_dict):
        global RUSSIAN_LETTERS
        return np.array(list([
            letter_dict[letter] if letter in letter_dict else 0
            for letter in RUSSIAN_LETTERS
        ]))
