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

from sklearn.neural_network import MLPClassifier

class MLP:
    def __init__(self, verbose = True):
        self.classifier = MLPClassifier(
                hidden_layer_sizes = (10),
                activation = 'logistic',
                solver = 'sgd',
                learning_rate = 'constant',
                learning_rate_init = 0.7,
                max_iter = 10000,
                tol = 0.00001,
                momentum = 0,
                verbose = verbose,
            )

    def train(self, X, y):
        self.classifier.fit(X, y)

    def estimate(self, X):
        return self.classifier.predict(X)
