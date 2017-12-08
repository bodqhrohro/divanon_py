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
