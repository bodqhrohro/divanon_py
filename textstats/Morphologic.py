import nltk
import pymorphy2
from collections import defaultdict

morph = pymorphy2.MorphAnalyzer()

def _text_to_words(text):
    return nltk.word_tokenize(text)

def _russian_words(words):
    return filter(_is_word_russian, words)

def _is_word_russian(word):
    russian_letters_count = sum(_is_letter_russian(letter) for letter in word)
    return russian_letters_count > (len(word) / 2)

def _is_letter_russian(letter):
    cyrillic_list = [ 'а', 'б', 'в', 'г', 'д', 'е', 'ё', 'ж', 'з', 'и', 'й', 'к', 'л', 'м', 'н', 'о', 'п', 'р', 'с', 'т', 'у', 'ф', 'х', 'ц', 'ч', 'ш', 'щ', 'ъ', 'ы', 'ь', 'э', 'ю', 'я' ]
    return letter.lower() in cyrillic_list

class Morphologic:
    def get_PoS_distribution(text):
        PoS_dict = defaultdict(float)
        words = _text_to_words(text)
        russian_words = _russian_words(words)
        for word in russian_words:
            ps = morph.parse(word)
            for p in ps:
                if (p.tag.POS != None):
                    PoS_dict[p.tag.POS] += float(p.score)
        return PoS_dict
