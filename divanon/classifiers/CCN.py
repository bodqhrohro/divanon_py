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

# from divanon.vendor.cascade_net import CascadeNet, logistic, d_logistic
from divanon.dblink.cmu_wrapper import CMUWrapper
import numpy as np


class CCN:
    def __init__(self, name: str, verbose=True):
        self.filename = name
        self.full_filename = name + '.data'
        self.classifier = CMUWrapper()
        self.verbose = verbose

    def train(self, X, y):
        self.classifier.generateTrainSet(
            self.full_filename, X, y, np.max(y) + 1
        )

    def estimate(self, X):
        return self.classifier.estimateByTrainFile(
            self.full_filename,
            X,
            self.verbose
        )
