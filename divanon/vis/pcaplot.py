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

import matplotlib as mpl
mpl.use('Agg')
from matplotlib import pyplot as plot
from sklearn.decomposition import PCA
import numpy as np

def plot_hyperplane(X, y, filename):
    plot.figure()
    pca = PCA()
    y_len = np.amax(y) + 1
    X_r = pca.fit(X).transform(X)
    cmap = plot.cm.get_cmap('hsv', y_len)
    y_range = range(0, y_len)
    for color, i in zip([cmap(i) for i in y_range], y_range):
        plot.scatter(X_r[y==i, 0], X_r[y==i, 1], color=color, alpha=.8, lw=2)
    plot.savefig(filename)
