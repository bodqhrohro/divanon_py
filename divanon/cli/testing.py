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

import logging
import numpy as np
from random import shuffle

from divanon.parse.ScrapyImport import ScrapyImport
import divanon.analytic.utils as utils
from divanon.textstats.Morphologic import Morphologic
from divanon.classifiers.MLP import MLP
from divanon.classifiers.SVM import SVM
from divanon.classifiers.CCN import CCN
# import divanon.vis.pcaplot as pcaplot


def randomly_split_sets(filenames):
    authors_dict = {}
    for filename in filenames:
        try:
            posts = ScrapyImport.texts_cleanup(
                ScrapyImport.file_to_array(filename)
            )
            authors_dict[filename] = posts
        except IOError:
            logging.error(IOError)
            raise IOError

    authors_numbers = {
        value: key for key, value in enumerate(authors_dict.keys())
    }
    authors_pairs = []
    for author, posts in authors_dict.items():
        author_number = authors_numbers[author]
        for post in posts:
            authors_pairs.append((post, author_number))

    shuffle(authors_pairs)

    train_pairs, control_pairs = utils.randomly_split_list(
        authors_pairs, 0.63 * len(authors_pairs)
    )

    svm = SVM(verbose=False)
    mlp = MLP(verbose=False)
    ccn = CCN('tist', verbose=False)
    classifiers = [svm, mlp, ccn]

    characteristics = [
        lambda post: utils.normalize_np_array(Morphologic.PoS_dict_to_np_array(
            Morphologic.get_PoS_distribution(post)
        )),
        lambda post: utils.normalize_np_array(
            Morphologic.letter_dict_to_np_array(
                Morphologic.get_letter_distribution(post)
            )
        ),
    ]

    train_posts, train_authors_numbers = zip(*train_pairs)
    train_authors_numbers = np.array(train_authors_numbers)

    control_posts, control_authors_numbers = zip(*control_pairs)
    control_authors_numbers = np.array(control_authors_numbers)

    # pcaplot.plot_hyperplane(train_posts, train_authors_numbers, 'tist.png')

    for characteristic in characteristics:
        train_data = np.array(list([
            characteristic(post) for post in train_posts
        ]))
        for classifier in classifiers:
            classifier.train(train_data, train_authors_numbers)

        control_data = np.array(list([
            characteristic(post) for post in control_posts
        ]))
        for classifier in classifiers:
            estimated_author_numbers = classifier.estimate(control_data)
            print(estimated_author_numbers)

            success_count = 0
            for estimated_author, control_author in zip(
                control_authors_numbers, estimated_author_numbers
            ):
                if estimated_author == control_author:
                    success_count += 1

            print(success_count / len(control_authors_numbers))
    '''
    for classifier in classifiers:
        classifier.train(
            np.array([
                [0.1, 0.2, 0.3], [0.4, 0.5, 0.6]
            ]),
            np.array([0, 1])
        )
        print(classifier.estimate(
            np.array([
                [0.4, 0.5, 0.6],
                [0.1, 0.2, 0.3],
                [0.7, 0.8, 0.9],
                [0.0, 0.0, 0.3]
            ]))
        )
    '''
