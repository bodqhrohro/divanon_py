import logging
import numpy as np
from random import shuffle

from divanon.parse.ScrapyImport import ScrapyImport
import divanon.analytic.utils as utils
from divanon.textstats.Morphologic import Morphologic
from divanon.classifiers.SVM import SVM
import divanon.vis.pcaplot as pcaplot

def randomly_split_sets(filenames):
    authors_dict = {}
    for filename in filenames:
        #try:
            posts = ScrapyImport.texts_cleanup(ScrapyImport.file_to_array(filename))
            authors_dict[filename] = posts
        #except Error:
            #logging.error(Error)
            #die

    authors_numbers = { value: key for key, value in enumerate(authors_dict.keys()) }
    authors_pairs = []
    for author, posts in authors_dict.items():
        author_number = authors_numbers[author]
        for post in posts:
            authors_pairs.append((post, author_number))

    shuffle(authors_pairs)

    train_pairs, control_pairs = utils.randomly_split_list(authors_pairs, 0.63 * len(authors_pairs))

    svm = SVM()

    train_posts, train_authors_numbers = zip(*train_pairs)
    train_posts = np.array(list([utils.normalize_np_array(Morphologic.dict_to_np_array(Morphologic.get_PoS_distribution(post))) for post in train_posts]))
    train_authors_numbers = np.array(train_authors_numbers)

    pcaplot.plot_hyperplane(train_posts, train_authors_numbers, 'tist.png')

    svm.train(train_posts, train_authors_numbers)

    control_posts, control_authors_numbers = zip(*control_pairs)
    control_posts = np.array(list([utils.normalize_np_array(Morphologic.dict_to_np_array(Morphologic.get_PoS_distribution(post))) for post in control_posts]))
    control_authors_numbers = np.array(control_authors_numbers)

    estimated_author_numbers = svm.estimate(control_posts)
    print(estimated_author_numbers)

    success_count = 0
    for estimated_author, control_author in zip(control_authors_numbers, estimated_author_numbers):
        if estimated_author == control_author:
            success_count += 1

    print(success_count / len(control_authors_numbers))
